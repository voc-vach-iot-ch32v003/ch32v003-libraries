#include "ch32fun.h"
#include "ch32v003_timer.h"
#include "ch32v003_ws2812.h"
#include "hil.h"
#include <string.h>

// Test HIL cho ch32v003_ws2812: ESP32 bắt khung bằng RMT (WSCAP, 12.5ns) trên chân DIN, runner so byte với
// mô hình Python (thứ tự màu, độ sáng) và độ rộng xung với datasheet WS2812B.
// Bit-bang trên PC4 (cả 3 chip); SPI DMA trên PC6 (A4M6, F4P6). Mẫu màu LED i dùng chung công thức với runner.
// Mỗi phép bắt: "capture <net> <case> <idleUs>" -> runner bật WSCAP rồi trả lời -> chip phát -> "verify <case>".

#define PIN_C(n) ((MCU_PORT_INDEX_C << 4) | (n))
#define PATTERN_LEDS 8
#define LONG_LEDS    16  // Nhiều lần nạp lại bộ đệm DMA (4 LED)
#define MANY_LEDS    100 // ~3ms bit-bang: kiểm tra millis() không đứng

typedef struct
{
    const char* tag;
    const char* net;
    WS2812_DriverMode_t mode;
    uint8_t pin;
} TestConfiguration_t;

static const TestConfiguration_t configurations[] = {
    {"bb", "PC4", WS2812_MODE_BITBANG, PIN_C(4)},
#if !defined(CH32V003_J4M6)
    {"dma", "PC6", WS2812_MODE_SPI_DMA, PIN_C(6)},
#endif
};

static WS2812_DriverHandle_t strip;
static uint8_t pixels[(MANY_LEDS + 1) * WS2812_BYTES_PER_LED]; // +1 LED làm vùng canh ghi tràn
static char text[48];
static char name[32];
static char detail[48];

static void check(const char* what, const char* tag, const int passed, const char* info)
{
    mini_snprintf(name, sizeof(name), "%s_%s", what, tag);
    hilCheck(name, passed, info);
}

static uint32_t patternColor(const uint16_t i)
{
    return WS2812_COLOR((i * 37 + 5) & 0xFF, (i * 91 + 200) & 0xFF, (255 - i * 29) & 0xFF);
}

static void loadPattern(const uint16_t count)
{
    for (uint16_t i = 0; i < count; i++) ws2812SetPixel(&strip, i, patternColor(i));
}

static void showAndWait(void)
{
    ws2812Show(&strip);
    while (ws2812IsBusy(&strip))
    {
    }
}

// Runner bật bắt tín hiệu trước, chip phát, rồi runner so kết quả
static void captured(const TestConfiguration_t* cfg, const char* testCase, const uint16_t idleUs,
                     void (*action)(void))
{
    mini_snprintf(text, sizeof(text), "capture %s %s_%s %u", cfg->net, cfg->tag, testCase, idleUs);
    hilWait(text);
    action();
    mini_snprintf(text, sizeof(text), "verify %s_%s", cfg->tag, testCase);
    hilWait(text);
}

static uint8_t initStrip(const TestConfiguration_t* cfg, const WS2812_ColorOrder_t order, const uint16_t count)
{
    strip.driverMode = cfg->mode;
    strip.dataPin = cfg->pin;
    strip.colorOrder = order;
    return ws2812Init(&strip, pixels, count);
}

// ============================================================================
// CÁC CẢNH
// ============================================================================

static void showOnce(void)
{
    showAndWait();
}

static void showTwice(void)
{
    ws2812Show(&strip);
    ws2812Show(&strip); // Phải chờ đủ thời gian chốt sau khung đầu
    while (ws2812IsBusy(&strip))
    {
    }
}

// ============================================================================

static void testConfiguration(const TestConfiguration_t* cfg)
{
    const char* tag = cfg->tag;
    mini_snprintf(text, sizeof(text), "setup %s", tag);
    hilWait(text);

    // Tham số sai
    strip.driverMode = cfg->mode;
    strip.dataPin = cfg->pin;
    strip.colorOrder = (WS2812_ColorOrder_t)6;
    const uint8_t badOrder = ws2812Init(&strip, pixels, PATTERN_LEDS);
    strip.colorOrder = WS2812_ORDER_GRB;
    check("init_reject", tag,
          ws2812Init(NULL, pixels, 1) == 0 && ws2812Init(&strip, NULL, 1) == 0 && ws2812Init(&strip, pixels, 0) == 0 &&
              badOrder == 0, "NULL / 0 LED / thứ tự sai");

    // Init xóa bộ đệm; Set/Get; ngoài dải không ghi tràn
    memset(pixels, 0xAA, sizeof(pixels));
    check("init", tag, initStrip(cfg, WS2812_ORDER_GRB, PATTERN_LEDS), "");
    uint8_t cleared = 1;
    for (uint16_t i = 0; i < PATTERN_LEDS * WS2812_BYTES_PER_LED; i++) cleared &= pixels[i] == 0;
    const uint8_t guard = pixels[PATTERN_LEDS * WS2812_BYTES_PER_LED];
    ws2812SetPixel(&strip, PATTERN_LEDS, 0xFFFFFF);
    ws2812SetPixel(&strip, 3, 0x123456);
    check("pixels", tag, cleared && guard == 0xAA && pixels[PATTERN_LEDS * WS2812_BYTES_PER_LED] == 0xAA &&
                             ws2812GetPixel(&strip, 3) == 0x123456 && ws2812GetPixel(&strip, PATTERN_LEDS) == 0 &&
                             pixels[9] == 0x12 && pixels[10] == 0x34 && pixels[11] == 0x56, "");

    // Thứ tự màu
    loadPattern(PATTERN_LEDS);
    captured(cfg, "grb", 100, showOnce);
    initStrip(cfg, WS2812_ORDER_RGB, PATTERN_LEDS);
    loadPattern(PATTERN_LEDS);
    captured(cfg, "rgb", 100, showOnce);
    initStrip(cfg, WS2812_ORDER_BGR, PATTERN_LEDS);
    loadPattern(PATTERN_LEDS);
    captured(cfg, "bgr", 100, showOnce);

    // Độ sáng áp khi phát, bộ đệm giữ màu gốc
    initStrip(cfg, WS2812_ORDER_GRB, PATTERN_LEDS);
    loadPattern(PATTERN_LEDS);
    ws2812SetBrightness(&strip, 128);
    captured(cfg, "bright128", 100, showOnce);
    check("bright_keeps_buffer", tag, ws2812GetPixel(&strip, 5) == patternColor(5), "");
    ws2812SetBrightness(&strip, 0);
    captured(cfg, "bright0", 100, showOnce);
    ws2812SetBrightness(&strip, 255);
    ws2812Fill(&strip, 0x123456);
    captured(cfg, "fill", 100, showOnce);

    // 2 lần show liền nhau: khoảng chốt >= 280us (bắt cả 2 khung với idle 400us)
    initStrip(cfg, WS2812_ORDER_GRB, 4);
    loadPattern(4);
    captured(cfg, "twice", 400, showTwice);

    // Dải dài hơn bộ đệm DMA
    initStrip(cfg, WS2812_ORDER_GRB, LONG_LEDS);
    loadPattern(LONG_LEDS);
    captured(cfg, "long", 100, showOnce);

    // Không chặn CPU (DMA) / chặn đúng thời lượng (bit-bang)
    initStrip(cfg, WS2812_ORDER_GRB, MANY_LEDS);
    loadPattern(MANY_LEDS);
    showAndWait(); // Qua thời gian chốt của lần init
    const uint32_t ticksPerUs = DELAY_US_TIME;
    uint32_t start = SysTick->CNT;
    ws2812Show(&strip);
    const uint32_t returnUs = (SysTick->CNT - start) / ticksPerUs;
    const uint8_t busyAfter = ws2812IsBusy(&strip);
    while (ws2812IsBusy(&strip))
    {
    }
    const uint32_t doneUs = (SysTick->CNT - start) / ticksPerUs;
    mini_snprintf(detail, sizeof(detail), "trả về %luus, xong %luus", returnUs, doneUs);
    if (cfg->mode == WS2812_MODE_SPI_DMA)
    {
        // 300us chờ chốt + mã hóa 4 LED đầu; phát 100 LED × 32us = 3.2ms
        check("dma_nonblocking", tag, busyAfter && returnUs < 600 && doneUs > 3200 && doneUs < 4500, detail);
    }
    else
    {
        // 100 LED × 1.3us × 24 bit = 3.1ms (+ khoảng giữa LED)
        check("bitbang_duration", tag, !busyAfter && returnUs > 3100 && returnUs < 4500, detail);
    }

    // millis() vẫn chạy đúng trong lúc phát dài (ngắt chỉ tắt từng LED)
    const uint32_t millisStart = millis();
    start = SysTick->CNT;
    for (uint8_t i = 0; i < 10; i++) showAndWait();
    const uint32_t elapsedMs = (SysTick->CNT - start) / DELAY_MS_TIME;
    const uint32_t millisElapsed = millis() - millisStart;
    mini_snprintf(detail, sizeof(detail), "millis %lu, SysTick %lums", millisElapsed, elapsedMs);
    check("millis_running", tag, millisElapsed + 1 >= elapsedMs && millisElapsed <= elapsedMs + 1, detail);
}

int main(void)
{
    SystemInit();
    Delay_Ms(1000); // Cửa sổ nạp
    timerInit();

    hilBegin("ws2812");
#if defined(CH32V003_J4M6)
    strip.driverMode = WS2812_MODE_SPI_DMA;
    strip.colorOrder = WS2812_ORDER_GRB;
    hilCheck("dma_unsupported_j4m6", ws2812Init(&strip, pixels, 1) == 0, "J4M6 không có PC6");
#endif
    for (uint8_t i = 0; i < sizeof(configurations) / sizeof(configurations[0]); i++)
    {
        testConfiguration(&configurations[i]);
    }
    hilWait("done");
    hilEnd();
}

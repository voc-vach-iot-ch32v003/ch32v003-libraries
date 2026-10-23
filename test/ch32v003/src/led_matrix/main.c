#include "ch32fun.h"
#include "ch32v003_led_matrix.h"
#include "sniff.h"
#include <string.h>

// Test HIL cho ch32v003_led_matrix: chuỗi 2 module (16x8), ESP32 nghe lén SPI (SNIFF) và so 8 khung của
// ledMatrixUpdate() với Frame Buffer mà runner dựng theo hợp đồng (bit 7 = cột trái, 90/270 xoay từng module,
// 180 lật cả dải). Chân như suite max7219: phần cứng SCK = PC5, MOSI = PC6 (F4P6); phần mềm SCK = PC4, MOSI = PA2;
// CS = PC2. Mỗi cảnh vẽ lại từ đầu nên làm lại (khi runner báo bắt hỏng) cho cùng khung SPI.

#define PIN_A(n) ((MCU_PORT_INDEX_A << 4) | (n))
#define PIN_C(n) ((MCU_PORT_INDEX_C << 4) | (n))
#define MODULES 2
#define WIDTH (MODULES * 8)

typedef struct
{
    const char* tag;
    uint8_t csPin;
    SPI_DriverHandle_t bus;
} TestConfiguration_t;

static TestConfiguration_t configurations[] = {
#if defined(CH32V003_F4P6)
    {"hw", PIN_C(2), {.driverMode = SPI_MODE_HARDWARE, .config.hardware = {SPI_CLOCK_DIVIDER_64}}},
#endif
    {"sw", PIN_C(2), {.driverMode = SPI_MODE_SOFTWARE, .config.software = {PIN_C(4), PIN_A(2), PIN_A(1)}}},
};

static LED_MATRIX_DriverHandle_t matrix;
static LED_MATRIX_DriverHandle_t other;
static uint8_t buffer[WIDTH];
static uint8_t otherBuffer[WIDTH];
static char text[40];
static char name[32];

static void check(const char* what, const char* tag, const int passed, const char* detail)
{
    mini_snprintf(name, sizeof(name), "%s_%s", what, tag);
    hilCheck(name, passed, detail);
}

static uint8_t bufferEmpty(const uint8_t* data)
{
    for (uint8_t i = 0; i < WIDTH; i++)
    {
        if (data[i]) return 0;
    }
    return 1;
}

// ============================================================================
// CÁC CẢNH (mỗi cảnh xóa rồi vẽ lại từ đầu, kết thúc bằng ledMatrixUpdate)
// ============================================================================

static void sceneDraw(void)
{
    ledMatrixSetRotation(&matrix, LED_MATRIX_ROTATE_0);
    ledMatrixClearBuffer(&matrix);
    ledMatrixDrawPixel(&matrix, 0, 0, true);
    ledMatrixDrawPixel(&matrix, WIDTH - 1, 7, true);
    ledMatrixDrawLine(&matrix, 0, 7, 7, 0, true);
    ledMatrixDrawString(&matrix, 8, 0, "Hi", true); // 'i' bị cắt ở mép phải
    ledMatrixDrawPixel(&matrix, 9, 0, false); // Xóa 1 điểm của 'H'
    ledMatrixUpdate(&matrix);
}

static void sceneRotate(const LED_MATRIX_Rotation_t rotation)
{
    ledMatrixSetRotation(&matrix, rotation);
    ledMatrixClearBuffer(&matrix);
    ledMatrixDrawPixel(&matrix, 1, 0, true);
    ledMatrixDrawPixel(&matrix, 9, 3, true);
    ledMatrixUpdate(&matrix);
    ledMatrixSetRotation(&matrix, LED_MATRIX_ROTATE_0);
}

static void sceneScroll(const LED_MATRIX_ScrollDirection_t direction, const uint8_t steps)
{
    ledMatrixSetScrollDirection(&matrix, direction); // Đặt lại vị trí cuộn
    for (uint8_t i = 0; i < steps; i++)
    {
        ledMatrixScrollTextAnimation(&matrix, "I", 0, true);
    }
    ledMatrixUpdate(&matrix);
}

static void sceneClear(void)
{
    ledMatrixClearBuffer(&matrix);
    ledMatrixUpdate(&matrix);
}

// ============================================================================

static void testConfiguration(const TestConfiguration_t* cfg)
{
    const char* tag = cfg->tag;
    sniffSlowClock = (cfg->bus.driverMode == SPI_MODE_SOFTWARE);
    mini_snprintf(text, sizeof(text), "setup %s", tag);
    hilWait(text);
    check("spi_init", tag, spiInit((SPI_DriverHandle_t*)&cfg->bus), "");
    spiInitCS(cfg->csPin); // CS nghỉ HIGH trước khi bộ nghe lén bắt đầu

    // Tham số sai: từ chối, không phát khung nào
    check("init_reject", tag,
          ledMatrixInit(NULL, &cfg->bus, cfg->csPin, buffer, MODULES) == 0 &&
              ledMatrixInit(&matrix, &cfg->bus, cfg->csPin, NULL, MODULES) == 0 &&
              ledMatrixInit(&matrix, NULL, cfg->csPin, buffer, MODULES) == 0 &&
              ledMatrixInit(&matrix, &cfg->bus, cfg->csPin, buffer, 0) == 0,
          "NULL / thiếu bus / 0 module");

    memset(otherBuffer, 0, sizeof(otherBuffer)); // Bộ đệm kề bên: dùng để phát hiện ghi tràn
    memset(buffer, 0xFF, sizeof(buffer)); // Init phải tự xóa bộ đệm
    uint8_t ok = 0;
    SNIFFED(tag, "init", ok = ledMatrixInit(&matrix, &cfg->bus, cfg->csPin, buffer, MODULES));
    check("init_ok", tag, ok == 1 && bufferEmpty(buffer), "");

    SNIFFED(tag, "draw", sceneDraw());
    SNIFFED(tag, "rotate90", sceneRotate(LED_MATRIX_ROTATE_90));
    SNIFFED(tag, "rotate180", sceneRotate(LED_MATRIX_ROTATE_180));
    SNIFFED(tag, "rotate270", sceneRotate(LED_MATRIX_ROTATE_270));
    SNIFFED(tag, "scroll_left", sceneScroll(LED_MATRIX_SCROLL_LEFT, 13)); // Lần thứ 13 vẽ tại x = 16 - 12 = 4
    SNIFFED(tag, "scroll_right", sceneScroll(LED_MATRIX_SCROLL_RIGHT, 9)); // Lần thứ 9 vẽ tại x = -6 + 8 = 2
    SNIFFED(tag, "scroll_up", sceneScroll(LED_MATRIX_SCROLL_UP, 3)); // Lần thứ 3 vẽ tại y = 8 - 2 = 6
    SNIFFED(tag, "scroll_down", sceneScroll(LED_MATRIX_SCROLL_DOWN, 5)); // Lần thứ 5 vẽ tại y = -8 + 4 = -4
    SNIFFED(tag, "brightness", ledMatrixSetBrightness(&matrix, 9));
    SNIFFED(tag, "brightness_clamp", ledMatrixSetBrightness(&matrix, 30));
    SNIFFED(tag, "clear", sceneClear());

    // Điểm ngoài màn hình bị bỏ qua, không ghi tràn bộ đệm
    ledMatrixClearBuffer(&matrix);
    ledMatrixDrawPixel(&matrix, -1, 0, true);
    ledMatrixDrawPixel(&matrix, WIDTH, 0, true);
    ledMatrixDrawPixel(&matrix, 0, -1, true);
    ledMatrixDrawPixel(&matrix, 0, 8, true);
    ledMatrixSetRotation(&matrix, LED_MATRIX_ROTATE_90);
    ledMatrixDrawPixel(&matrix, WIDTH, 3, true);
    ledMatrixSetRotation(&matrix, LED_MATRIX_ROTATE_0);
    check("pixel_out_of_range", tag, bufferEmpty(buffer) && bufferEmpty(otherBuffer), "");

    // Hai màn hình cuộn độc lập: A cuộn 5 bước, B cuộn 2 + 3 bước -> cùng nội dung
    ledMatrixInit(&other, &cfg->bus, cfg->csPin, otherBuffer, MODULES);
    ledMatrixSetScrollDirection(&matrix, LED_MATRIX_SCROLL_LEFT);
    ledMatrixSetScrollDirection(&other, LED_MATRIX_SCROLL_LEFT);
    for (uint8_t i = 0; i < 5; i++) ledMatrixScrollTextAnimation(&matrix, "AB", 0, true);
    for (uint8_t i = 0; i < 2; i++) ledMatrixScrollTextAnimation(&other, "AB", 0, true);
    const uint8_t differentMidway = memcmp(buffer, otherBuffer, WIDTH) != 0;
    for (uint8_t i = 0; i < 3; i++) ledMatrixScrollTextAnimation(&other, "AB", 0, true);
    check("scroll_independent", tag,
          differentMidway && memcmp(buffer, otherBuffer, WIDTH) == 0 && !bufferEmpty(buffer),
          "A 5 bước == B 2 + 3 bước");
}

int main(void)
{
    SystemInit();
    Delay_Ms(1000); // Cửa sổ nạp

    hilBegin("led_matrix");
    for (uint8_t i = 0; i < sizeof(configurations) / sizeof(configurations[0]); i++)
    {
        testConfiguration(&configurations[i]);
    }
    hilWait("done");
    hilEnd();
}

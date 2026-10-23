#include "ch32fun.h"
#include "ch32v003_max7219.h"
#include "hil.h"

// Test HIL cho ch32v003_max7219: chuỗi 3 IC giả định, ESP32 nghe lén SPI (SNIFF) và so từng khung CS.
// - Phần cứng SPI1 (chỉ F4P6): SCK = PC5, MOSI = PC6 (chia 64 = 750kHz để bộ nghe lén theo kịp).
// - Phần mềm (cả 3 gói vỏ):    SCK = PC4, MOSI = PA2. Bit-bang ở 48MHz có xung SCK ~200ns, ngắn hơn 1 lượt đọc GPIO
//   của ESP32, nên lúc gọi hàm max7219 hạ clock lõi xuống /8 (xung ~1.6us). Test kiểm tra logic khung; tốc độ
//   bit-bang đã có suite spi kiểm tra.
// CS = PC2 cho cả 2 (có trên cả 3 gói vỏ): 3 chân nằm cùng thanh ghi GPIO.in của ESP32 nên bộ nghe lén đọc 1 lần / lượt.
// Bước "sniff <tag> <case>": runner bật SNIFF, trả lời, chip thực hiện thao tác; bước "verify <tag> <case>": runner
// so với khung mong đợi rồi trả "ok", hoặc "retry" nếu lần bắt không đáng tin (ESP32 bị dừng tạm giữa chừng) để chip
// làm lại thao tác (mọi thao tác chỉ ghi thanh ghi nên làm lại không đổi kết quả).

#define PIN_A(n) ((MCU_PORT_INDEX_A << 4) | (n))
#define PIN_C(n) ((MCU_PORT_INDEX_C << 4) | (n))
#define CASCADE 3

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

static char text[40];
static char name[32];

static uint8_t slowClock; // 1 = hạ clock lõi trong lúc gọi hàm max7219 (cấu hình phần mềm)

static void sniff(const char* tag, const char* testCase)
{
    mini_snprintf(text, sizeof(text), "sniff %s %s", tag, testCase);
    hilWait(text);
    if (slowClock) RCC->CFGR0 = (RCC->CFGR0 & ~RCC_HPRE) | RCC_HPRE_DIV8;
}

// Gọi sau mỗi thao tác được nghe lén: trả clock về 48MHz trước khi in qua SWIO, hỏi runner có cần làm lại không
static uint8_t done(const char* tag, const char* testCase)
{
    RCC->CFGR0 = (RCC->CFGR0 & ~RCC_HPRE) | RCC_HPRE_DIV1;
    mini_snprintf(text, sizeof(text), "verify %s %s", tag, testCase);
    return hilWait(text)[0] == 'o';
}

// Nghe lén một thao tác, làm lại tới khi runner bắt được trọn vẹn
#define SNIFFED(tag, testCase, statement) \
    do                                    \
    {                                     \
        sniff(tag, testCase);             \
        statement;                        \
    } while (!done(tag, testCase))

static void testConfiguration(TestConfiguration_t* cfg)
{
    slowClock = (cfg->bus.driverMode == SPI_MODE_SOFTWARE);
    mini_snprintf(text, sizeof(text), "setup %s", cfg->tag);
    hilWait(text);
    mini_snprintf(name, sizeof(name), "spi_init_%s", cfg->tag);
    hilCheck(name, spiInit(&cfg->bus), "");
    spiInitCS(cfg->csPin); // CS nghỉ HIGH trước khi bộ nghe lén bắt đầu (chân thả nổi có thể đang LOW)

    // Handle không hợp lệ: từ chối, không phát khung nào
    const MAX7219_DriverHandle_t empty = {.spiHandle = &cfg->bus, .csPin = cfg->csPin, .cascadeCount = 0};
    mini_snprintf(name, sizeof(name), "init_reject_%s", cfg->tag);
    hilCheck(name, max7219Init(&empty) == 0 && max7219Init(NULL) == 0, "cascadeCount = 0 / NULL");

    const MAX7219_DriverHandle_t dev = {.spiHandle = &cfg->bus, .csPin = cfg->csPin, .cascadeCount = CASCADE};

    uint8_t ok = 0;
    SNIFFED(cfg->tag, "init", ok = max7219Init(&dev));
    mini_snprintf(name, sizeof(name), "init_ok_%s", cfg->tag);
    hilCheck(name, ok == 1, "");

    SNIFFED(cfg->tag, "write_register_1", max7219WriteRegister(&dev, 1, MAX7219_REG_DIGIT2, 0xA5));

    SNIFFED(cfg->tag, "write_register_0", max7219WriteRegister(&dev, 0, MAX7219_REG_DIGIT0, 0x81));

    // Ngoài chuỗi: không phát gì
    SNIFFED(cfg->tag, "write_register_out", max7219WriteRegister(&dev, CASCADE, MAX7219_REG_DIGIT0, 0xFF));

    SNIFFED(cfg->tag, "write_all", max7219WriteAll(&dev, MAX7219_REG_DIGIT7, 0x3C));

    static const uint8_t chain[CASCADE] = {0x11, 0x22, 0x33};
    SNIFFED(cfg->tag, "write_chain", max7219WriteChain(&dev, MAX7219_REG_DIGIT4, chain, 1));

    // Frame Buffer 8 byte / IC: hàng 0 của IC i nằm ở frameBuffer[i * 8]
    uint8_t frameBuffer[CASCADE * 8];
    for (uint8_t i = 0; i < sizeof(frameBuffer); i++) frameBuffer[i] = (uint8_t)(0xA0 + i);
    SNIFFED(cfg->tag, "write_chain_stride", max7219WriteChain(&dev, MAX7219_REG_DIGIT4, frameBuffer, 8));

    SNIFFED(cfg->tag, "intensity_clamp", max7219SetIntensity(&dev, 20));

    SNIFFED(cfg->tag, "power_off", max7219SetPower(&dev, false));

    SNIFFED(cfg->tag, "power_on", max7219SetPower(&dev, true));

    SNIFFED(cfg->tag, "clear_all", max7219ClearAll(&dev));
}

int main(void)
{
    SystemInit();
    Delay_Ms(1000); // Cửa sổ nạp

    hilBegin("max7219");
    for (uint8_t i = 0; i < sizeof(configurations) / sizeof(configurations[0]); i++)
    {
        testConfiguration(&configurations[i]);
    }
    hilWait("done");
    hilEnd();
}

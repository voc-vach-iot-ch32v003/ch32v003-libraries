#include "ch32fun.h"
#include "ch32v003_delay.h"
#include "ch32v003_spi.h"
#include "hil.h"

// Test HIL cho ch32v003_spi: ESP32 làm SPI slave Mode 0.
// - Phần cứng SPI1 (chỉ F4P6): SCK = PC5, MOSI = PC6, MISO = PC7, CS = PC0.
// - Phần mềm (cả 3 gói vỏ):    SCK = PC4, MOSI = PA2, MISO = PA1, CS = PC1.
// Mỗi cấu hình: truyền-nhận song công, spiWrite liên tiếp, spiTransfer ngay sau spiWrite, đo tốc độ qua độ dài CS.
// Runner nạp sẵn dữ liệu slave sẽ trả (SPIS LOAD) trước mỗi giao dịch.

#define PIN_A(n) ((MCU_PORT_INDEX_A << 4) | (n))
#define PIN_C(n) ((MCU_PORT_INDEX_C << 4) | (n))

typedef struct
{
    const char* tag;
    uint8_t csPin;
    SPI_DriverHandle_t handle;
} TestConfiguration_t;

static TestConfiguration_t configurations[] = {
#if defined(CH32V003_F4P6)
    {"hw16", PIN_C(0), {.driverMode = SPI_MODE_HARDWARE, .config.hardware = {SPI_CLOCK_DIVIDER_16}}},
    {"hw256", PIN_C(0), {.driverMode = SPI_MODE_HARDWARE, .config.hardware = {SPI_CLOCK_DIVIDER_256}}},
#endif
    {"sw", PIN_C(1), {.driverMode = SPI_MODE_SOFTWARE, .config.software = {PIN_C(4), PIN_A(2), PIN_A(1)}}},
};

static char text[48];
static char name[32];

static void step(const char* kind, const char* tag, const char* argument)
{
    mini_snprintf(text, sizeof(text), "%s %s %s", kind, tag, argument);
    hilWait(text);
}

static void check(const char* what, const char* tag, const int passed, const char* detail)
{
    mini_snprintf(name, sizeof(name), "%s_%s", what, tag);
    hilCheck(name, passed, detail);
}

static void toHex(const uint8_t* data, const uint8_t length, char* out)
{
    static const char digits[] = "0123456789ABCDEF";
    for (uint8_t i = 0; i < length; i++)
    {
        out[2 * i] = digits[data[i] >> 4];
        out[2 * i + 1] = digits[data[i] & 0x0F];
    }
    out[2 * length] = '\0';
}

static void testConfiguration(TestConfiguration_t* cfg)
{
    SPI_DriverHandle_t* bus = &cfg->handle;
    char hex[2 * 12 + 1];
    char detail[32];

    step("slave_on", cfg->tag, "-");
    check("init", cfg->tag, spiInit(bus), "");
    spiInitCS(cfg->csPin);

    // 1. Song công 8 byte: chip gửi 00 11 22 ... 77, slave trả mẫu nạp sẵn
    static const uint8_t expected[] = {0xC3, 0xA5, 0x5A, 0x3C, 0x0F, 0x1E, 0x2D, 0x4B};
    uint8_t received[8];
    toHex(expected, sizeof(expected), hex);
    step("load", cfg->tag, hex);
    spiSelect(cfg->csPin);
    for (uint8_t i = 0; i < sizeof(received); i++)
    {
        received[i] = spiTransfer(bus, i * 0x11);
    }
    spiWaitIdle(bus);
    spiDeselect(cfg->csPin);
    toHex(received, sizeof(received), hex);
    mini_snprintf(detail, sizeof(detail), "doc=%s", hex);
    check("transfer_miso", cfg->tag, memcmp(received, expected, sizeof(expected)) == 0, detail);
    step("expect_rx", cfg->tag, "0011223344556677");

    // 2. spiWrite 12 byte liên tiếp
    static const uint8_t burst[] = {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB};
    step("load", cfg->tag, "-");
    spiSelect(cfg->csPin);
    spiWrite(bus, burst, sizeof(burst));
    spiWaitIdle(bus);
    spiDeselect(cfg->csPin);
    step("expect_rx", cfg->tag, "A0A1A2A3A4A5A6A7A8A9AAAB");

    // 3. spiTransfer ngay sau spiWrite: byte nhận phải là của giao dịch mới (không dính byte cũ / cờ OVR)
    step("load", cfg->tag, "5A");
    spiSelect(cfg->csPin);
    const uint8_t after = spiTransfer(bus, 0x99);
    spiWaitIdle(bus);
    spiDeselect(cfg->csPin);
    mini_snprintf(detail, sizeof(detail), "doc=%02X", after);
    check("transfer_after_write", cfg->tag, after == 0x5A, detail);
    step("expect_rx", cfg->tag, "99");

    // 4. Tốc độ: runner bắt độ dài mức LOW của CS trong lúc ghi 32 byte 00 01 02 ... 1F (đủ dài để sai số đo nhỏ)
    uint8_t ramp[32];
    for (uint8_t i = 0; i < sizeof(ramp); i++) ramp[i] = i;
    step("cs_time", cfg->tag, "32");
    spiSelect(cfg->csPin);
    spiWrite(bus, ramp, sizeof(ramp));
    spiWaitIdle(bus);
    spiDeselect(cfg->csPin);
    step("check_ramp", cfg->tag, "32");
}

int main(void)
{
    SystemInit();
    delayMs(1000); // Cửa sổ nạp

    hilBegin("spi");
#if !defined(CH32V003_F4P6)
    // A4M6 / J4M6 không có chân PC5 (SCK của SPI1): spiInit phải từ chối chế độ phần cứng
    SPI_DriverHandle_t hardware = {.driverMode = SPI_MODE_HARDWARE};
    hilCheck("hw_unavailable", spiInit(&hardware) == 0, "");
#endif
    for (uint8_t i = 0; i < sizeof(configurations) / sizeof(configurations[0]); i++)
    {
        testConfiguration(&configurations[i]);
    }
    hilEnd();
}

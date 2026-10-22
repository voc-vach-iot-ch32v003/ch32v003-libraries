#include "ch32fun.h"
#include "ch32v003_delay.h"
#include "ch32v003_i2c.h"
#include "hil.h"

// Test HIL cho ch32v003_i2c: ESP32 làm I2C slave (địa chỉ 0x42) trên PC1 (SDA) / PC2 (SCL), có trên cả 3 gói vỏ.
// Mỗi cấu hình (phần cứng 100kHz, phần cứng 400kHz, phần mềm) chạy: ghi burst, đọc burst 1/2/4 byte,
// đọc từng byte bằng API rời, NACK địa chỉ vắng mặt rồi giao dịch tiếp vẫn chạy, đo tần số SCL.
// ESP32 bản gốc không kéo giãn SCL: runner nạp sẵn dữ liệu trả lời (I2CS LOAD) trước mỗi lần đọc.

#define SLAVE_ADDRESS  0x42
#define ABSENT_ADDRESS 0x43

typedef struct
{
    const char* tag;
    I2C_DriverHandle_t handle;
} TestConfiguration_t;

static const TestConfiguration_t configurations[] = {
    {"hw100", {.driverMode = I2C_MODE_HARDWARE, .config.hardware = {I2C_REMAP_DEFAULT, I2C_SPEED_STANDARD}}},
    {"hw400", {.driverMode = I2C_MODE_HARDWARE, .config.hardware = {I2C_REMAP_DEFAULT, I2C_SPEED_FAST}}},
    {"sw", {.driverMode = I2C_MODE_SOFTWARE, .config.software = {MCU_I2C_SDA, MCU_I2C_SCL}}},
};

static char text[48];
static char name[24];

static void step(const char* format, const char* tag, const char* argument)
{
    mini_snprintf(text, sizeof(text), format, tag, argument);
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

// Đọc burst sau khi runner nạp sẵn dữ liệu, so khớp trên chip + nhờ runner xác nhận slave nhận đúng địa chỉ thanh ghi
static void testReadBurst(const TestConfiguration_t* cfg, const char* what, const uint8_t reg,
                          const uint8_t* expected, const uint8_t length)
{
    char hex[2 * 8 + 1];
    char detail[24];
    uint8_t buffer[8] = {0};

    toHex(expected, length, hex);
    step("load %s %s", cfg->tag, hex);

    const uint8_t ok = i2cReadBurst(&cfg->handle, SLAVE_ADDRESS, reg, buffer, length);
    toHex(buffer, length, hex);
    mini_snprintf(detail, sizeof(detail), "ret=%u doc=%s", ok, hex);
    check(what, cfg->tag, ok && memcmp(buffer, expected, length) == 0, detail);

    mini_snprintf(hex, sizeof(hex), "%02X", reg);
    step("expect_rx %s %s", cfg->tag, hex);
}

static void testConfiguration(const TestConfiguration_t* cfg)
{
    const I2C_DriverHandle_t* bus = &cfg->handle;
    char detail[24];

    step("slave_on %s%s", cfg->tag, "");
    check("init", cfg->tag, i2cInit(bus), "");

    // 1. Ghi burst 5 byte vào thanh ghi 0x10
    static const uint8_t writeData[] = {0x11, 0x22, 0x33, 0x44, 0x55};
    check("write_burst", cfg->tag, i2cWriteBurst(bus, SLAVE_ADDRESS, 0x10, writeData, sizeof(writeData)), "");
    step("expect_rx %s %s", cfg->tag, "101122334455");

    // 2. Đọc burst: 4 byte, 1 byte (nhánh riêng NACK + STOP trước khi xóa ADDR), 2 byte
    static const uint8_t read4[] = {0xA5, 0x5A, 0x3C, 0xC3};
    static const uint8_t read1[] = {0x7E};
    static const uint8_t read2[] = {0xBE, 0xEF};
    testReadBurst(cfg, "read_burst4", 0x20, read4, sizeof(read4));
    testReadBurst(cfg, "read_burst1", 0x21, read1, sizeof(read1));
    testReadBurst(cfg, "read_burst2", 0x22, read2, sizeof(read2));

    // 3. API rời: START đọc + 2 byte ACK + 1 byte NACK, rồi i2cStop thừa (không được làm kẹt bus)
    step("load %s %s", cfg->tag, "010203");
    uint8_t bytes[3] = {0};
    const uint8_t started = i2cStart(bus, SLAVE_ADDRESS, I2C_DIRECTION_READ);
    if (started)
    {
        bytes[0] = i2cRead(bus, 1);
        bytes[1] = i2cRead(bus, 1);
        bytes[2] = i2cRead(bus, 0);
    }
    i2cStop(bus);
    mini_snprintf(detail, sizeof(detail), "start=%u %02X%02X%02X", started, bytes[0], bytes[1], bytes[2]);
    check("read_single", cfg->tag, started && bytes[0] == 1 && bytes[1] == 2 && bytes[2] == 3, detail);

    // 4. NACK địa chỉ vắng mặt: phải trả 0, và giao dịch kế tiếp vẫn chạy (bus không kẹt)
    check("nack_write", cfg->tag, !i2cWriteBurst(bus, ABSENT_ADDRESS, 0x00, writeData, 1), "");
    check("nack_read", cfg->tag, !i2cStart(bus, ABSENT_ADDRESS, I2C_DIRECTION_READ), "");
    static const uint8_t afterNack[] = {0x99};
    check("after_nack", cfg->tag, i2cWriteBurst(bus, SLAVE_ADDRESS, 0x30, afterNack, 1), "");
    step("expect_rx %s %s", cfg->tag, "3099");

    // 5. Tần số SCL: runner bật bắt sườn CH14 rồi mới trả lời -> ghi 8 byte
    static const uint8_t burst[] = {0, 1, 2, 3, 4, 5, 6, 7};
    step("scl %s%s", cfg->tag, "");
    i2cWriteBurst(bus, SLAVE_ADDRESS, 0x40, burst, sizeof(burst));
    step("expect_rx %s %s", cfg->tag, "400001020304050607");
}

int main(void)
{
    SystemInit();
    delayMs(1000); // Cửa sổ nạp

    hilBegin("i2c");
    for (uint8_t i = 0; i < sizeof(configurations) / sizeof(configurations[0]); i++)
    {
        testConfiguration(&configurations[i]);
    }
    hilEnd();
}

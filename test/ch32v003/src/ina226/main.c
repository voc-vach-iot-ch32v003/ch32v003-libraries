#include "ch32fun.h"
#include "ch32v003_ina226.h"
#include "hil.h"

// Test HIL cho ch32v003_ina226: ESP32 làm I2C slave 0x40 trên PC1 (SDA) / PC2 (SCL) giả lập INA226.
// Cơ chế giống suite bmi160: firmware link với -Wl,--wrap=i2cReadBurst, mỗi lần thư viện đọc thanh ghi thì báo
// "read <reg>" để runner nạp giá trị từ bảng khai báo bởi "regs <reg>=<hex>[,<hex>...]" (nhiều giá trị = lần lượt
// cho từng lần đọc). Runner so chuỗi byte chip đã ghi. Giá trị mong đợi (CAL, LSB dòng, uV, mV, uW...) được tính
// độc lập bằng Python (xem suites/ina226.py) và ghi cứng ở đây.
// Handle chính: shunt 0.1 ohm, dòng tối đa 1A -> LSB dòng 30518 nA, CAL 1677 (0x068D).

static const I2C_DriverHandle_t bus = {
    .driverMode = I2C_MODE_HARDWARE,
    .config.hardware = {I2C_REMAP_DEFAULT, I2C_SPEED_FAST},
};

static char text[80];
static char detail[48];
static char readText[16];

uint8_t __real_i2cReadBurst(const I2C_DriverHandle_t* handle, uint8_t slaveAddress, uint8_t startRegister,
                            uint8_t* buffer, uint8_t length);

uint8_t __wrap_i2cReadBurst(const I2C_DriverHandle_t* handle, const uint8_t slaveAddress, const uint8_t startRegister,
                            uint8_t* buffer, const uint8_t length)
{
    if (slaveAddress == 0x40)
    {
        mini_snprintf(readText, sizeof(readText), "read %02X", startRegister);
        hilWait(readText);
    }
    return __real_i2cReadBurst(handle, slaveAddress, startRegister, buffer, length);
}

static void step(const char* format, const char* argument)
{
    mini_snprintf(text, sizeof(text), format, argument);
    hilWait(text);
}

#define IDS_OK "FE=5449 FF=2260"

static void testInit(INA226_DriverHandle_t* main)
{
    // Tham số sai: từ chối trước khi phát giao dịch nào
    INA226_DriverHandle_t noBus = {0};
    INA226_DriverHandle_t noShunt = {.i2c = &bus, .maxCurrentUa = 1000000};
    INA226_DriverHandle_t noCurrent = {.i2c = &bus, .shuntResistanceUohm = 100000};
    INA226_DriverHandle_t tooBig = {.i2c = &bus, .shuntResistanceUohm = 100000, .maxCurrentUa = 2000001};
    INA226_DriverHandle_t calHigh = {.i2c = &bus, .shuntResistanceUohm = 1, .maxCurrentUa = 1000000}; // CAL > 0x7FFF
    INA226_DriverHandle_t calZero = {.i2c = &bus, .shuntResistanceUohm = 0xFFFFFFFF, .maxCurrentUa = 2000000}; // CAL = 0
    INA226_DriverHandle_t badAddress = {.i2c = &bus, .slaveAddress = (INA226_I2CAddress_t)0x44,
                                        .shuntResistanceUohm = 100000, .maxCurrentUa = 1000000};
    INA226_DriverHandle_t badMode = {.i2c = &bus, .shuntResistanceUohm = 100000, .maxCurrentUa = 1000000,
                                     .configuration.mode = (INA226_Mode_t)5};
    INA226_DriverHandle_t badAverage = {.i2c = &bus, .shuntResistanceUohm = 100000, .maxCurrentUa = 1000000,
                                        .configuration.averagingMode = (INA226_AveragingMode_t)9};
    const uint8_t rejected = !ina226Init(NULL) && !ina226Init(&noBus) && !ina226Init(&noShunt) &&
                             !ina226Init(&noCurrent) && !ina226Init(&tooBig) && !ina226Init(&calHigh) &&
                             !ina226Init(&calZero) && !ina226Init(&badAddress) && !ina226Init(&badMode) &&
                             !ina226Init(&badAverage);
    hilCheck("init_reject", rejected, "NULL / thiếu tham số / CAL ngoài khoảng / enum sai");
    step("expect_rx reject %s", "-");

    // Sai Manufacturer ID: chỉ đọc 0xFE rồi dừng
    INA226_DriverHandle_t dev = {.i2c = &bus, .shuntResistanceUohm = 100000, .maxCurrentUa = 1000000};
    step("regs %s", "FE=5448 FF=2260");
    hilCheck("init_wrong_manufacturer", ina226Init(&dev) == 0, "");
    step("expect_rx wrong_manufacturer %s", "FE");

    // Sai Die ID: đọc 0xFE, 0xFF rồi dừng, chưa ghi gì
    step("regs %s", "FE=5449 FF=2261");
    hilCheck("init_wrong_die", ina226Init(&dev) == 0 && dev.currentLsbNa == 0, "");
    step("expect_rx wrong_die %s", "FEFF");

    // Địa chỉ 0x41 không có thiết bị: NACK -> false
    INA226_DriverHandle_t absent = {.i2c = &bus, .slaveAddress = INA226_ADDRESS_0X41,
                                    .shuntResistanceUohm = 100000, .maxCurrentUa = 1000000};
    hilCheck("init_absent", ina226Init(&absent) == 0, "");

    // Cấu hình đầy đủ: AVG 16 (mã 2), bus 588us (mã 3), shunt 204us (mã 1), shunt+bus triggered (mã 3) -> 0x44CB
    INA226_DriverHandle_t custom = {
        .i2c = &bus,
        .slaveAddress = INA226_ADDRESS_0X40,
        .configuration = {INA226_AVG_16, INA226_BUS_CONVERSION_TIME_588US, INA226_SHUNT_CONVERSION_TIME_204US,
                          INA226_MODE_SHUNT_BUS_TRIGGERED},
        .shuntResistanceUohm = 100000,
        .maxCurrentUa = 1000000,
    };
    step("regs %s", IDS_OK);
    const uint8_t ok = ina226Init(&custom);
    mini_snprintf(detail, sizeof(detail), "ret=%u lsb=%lu cal=%u", ok, custom.currentLsbNa, custom.calibration);
    hilCheck("init_custom", ok == 1 && custom.currentLsbNa == 30518 && custom.calibration == 1677, detail);
    step("expect_rx custom %s", "FEFF0044CB05068D");

    // Handle để trống: 0x40, AVG 1, 1.1ms, liên tục -> Config 0x4127
    INA226_DriverHandle_t defaults = {.i2c = &bus, .shuntResistanceUohm = 100000, .maxCurrentUa = 1000000};
    step("regs %s", IDS_OK);
    const uint8_t okDefault = ina226Init(&defaults);
    hilCheck("init_default", okDefault == 1 && defaults.slaveAddress == INA226_ADDRESS_0X40 &&
                                 defaults.configuration.averagingMode == INA226_AVG_1 &&
                                 defaults.configuration.busConversionTime == INA226_BUS_CONVERSION_TIME_1100US &&
                                 defaults.configuration.shuntConversionTime == INA226_SHUNT_CONVERSION_TIME_1100US &&
                                 defaults.configuration.mode == INA226_MODE_SHUNT_BUS_CONTINUOUS, "");
    step("expect_rx default %s", "FEFF00412705068D");

    // Handle chính dùng cho các test sau
    *main = defaults;

    // Dòng tối đa 2A, shunt 10 mohm: LSB 61036 nA, CAL 8388 (0x20C4)
    uint32_t lsb = 0;
    uint16_t cal = 0;
    const uint8_t okCal = ina226CalculateCalibration(10000, 2000000, &lsb, &cal);
    mini_snprintf(detail, sizeof(detail), "lsb=%lu cal=%u", lsb, cal);
    hilCheck("calibration_2A", okCal && lsb == 61036 && cal == 8388, detail);
    hilCheck("calibration_800mA", ina226CalculateCalibration(100000, 800000, &lsb, &cal) && lsb == 24415 && cal == 2097,
             "");
    // Dòng nhỏ: LSB làm tròn lên thành 3052 (không phải 3051)
    hilCheck("calibration_100mA", ina226CalculateCalibration(100000, 100000, &lsb, &cal) && lsb == 3052 && cal == 16775,
             "");
}

static void testConfiguration(INA226_DriverHandle_t* dev)
{
    // AVG 64 (mã 3), bus 2.116ms (mã 5), shunt 8.244ms (mã 7), bus liên tục (mã 6) -> 0x477E
    const INA226_Configuration_t slow = {INA226_AVG_64, INA226_BUS_CONVERSION_TIME_2116US,
                                         INA226_SHUNT_CONVERSION_TIME_8244US, INA226_MODE_BUS_CONTINUOUS};
    hilCheck("set_configuration", ina226SetConfiguration(dev, &slow) && dev->configuration.averagingMode == INA226_AVG_64,
             "");
    const INA226_Configuration_t bad = {.mode = (INA226_Mode_t)9};
    hilCheck("set_configuration_bad", !ina226SetConfiguration(dev, &bad) && dev->configuration.mode == INA226_MODE_BUS_CONTINUOUS,
             "");
    step("expect_rx config %s", "00477E");

    // Trigger không dùng được khi đang đo liên tục; sang triggered (0x4123) thì ghi lại Config
    hilCheck("trigger_continuous_rejected", !ina226TriggerMeasurement(dev), "");
    const INA226_Configuration_t triggered = {.mode = INA226_MODE_SHUNT_BUS_TRIGGERED};
    hilCheck("trigger", ina226SetConfiguration(dev, &triggered) && ina226TriggerMeasurement(dev), "");
    step("expect_rx trigger %s", "004123004123");

    hilCheck("reset", ina226Reset(dev), "");
    step("expect_rx reset %s", "008000");

    const INA226_Configuration_t back = {0};
    ina226SetConfiguration(dev, &back);
    step("expect_rx back %s", "004127");
}

static void testIds(const INA226_DriverHandle_t* dev)
{
    uint16_t manufacturer = 0, die = 0;
    step("regs %s", IDS_OK);
    const uint8_t ok = ina226ReadIds(dev, &manufacturer, &die);
    mini_snprintf(detail, sizeof(detail), "%04X %04X", manufacturer, die);
    hilCheck("read_ids", ok && manufacturer == 0x5449 && die == 0x2260, detail);
    step("expect_rx ids %s", "FEFF");
}

typedef struct
{
    int32_t raw;
    int32_t expected;
} Vector_t;

static const Vector_t shuntVectors[] = {{1000, 2500}, {-1000, -2500}, {-32768, -81920}, {32767, 81917}, {1, 2}, {-1, -3}};
static const Vector_t busVectors[] = {{0, 0}, {1, 1}, {32767, 40958}, {12000, 15000}, {65535, 81918}};
static const Vector_t currentNaVectors[] = {{1000, 30518000}, {-1000, -30518000}, {32767, 999983306},
                                            {-32768, -1000013824}, {1, 30518}, {-1, -30518}, {0, 0}};
static const Vector_t currentUaVectors[] = {{1000, 30518}, {-1000, -30518}, {32767, 999983},
                                            {-32768, -1000013}, {1, 30}, {-1, -30}, {0, 0}};
static const Vector_t powerVectors[] = {{0, 0}, {1, 762}, {1000, 762950}, {65535, 49999928}};

static void testReadings(const INA226_DriverHandle_t* dev)
{
    uint8_t passed = 1;

    step("regs %s", "01=03E8,FC18,8000,7FFF,0001,FFFF");
    for (uint8_t i = 0; i < 6; i++)
    {
        int32_t value = 0;
        passed &= ina226ReadShuntVoltageUv(dev, &value) && value == shuntVectors[i].expected;
    }
    hilCheck("read_shunt_uv", passed, "dương / âm / biên / làm tròn xuống");
    step("expect_rx shunt %s", "010101010101");

    passed = 1;
    step("regs %s", "02=0000,0001,7FFF,2EE0,FFFF");
    for (uint8_t i = 0; i < 5; i++)
    {
        uint32_t value = 0;
        passed &= ina226ReadBusVoltageMv(dev, &value) && (int32_t)value == busVectors[i].expected;
    }
    hilCheck("read_bus_mv", passed, "");
    step("expect_rx bus %s", "0202020202");

    passed = 1;
    step("regs %s", "04=03E8,FC18,7FFF,8000,0001,FFFF,0000");
    for (uint8_t i = 0; i < 7; i++)
    {
        int32_t value = 0;
        passed &= ina226ReadCurrentNa(dev, &value) && value == currentNaVectors[i].expected;
    }
    hilCheck("read_current_na", passed, "");
    step("expect_rx current_na %s", "04040404040404");

    passed = 1;
    step("regs %s", "04=03E8,FC18,7FFF,8000,0001,FFFF,0000");
    for (uint8_t i = 0; i < 7; i++)
    {
        int32_t value = 0;
        passed &= ina226ReadCurrentUa(dev, &value) && value == currentUaVectors[i].expected;
    }
    hilCheck("read_current_ua", passed, "cắt về 0 cho số âm");
    step("expect_rx current_ua %s", "04040404040404");

    passed = 1;
    step("regs %s", "03=0000,0001,03E8,FFFF");
    for (uint8_t i = 0; i < 4; i++)
    {
        uint32_t value = 0;
        passed &= ina226ReadPowerUw(dev, &value) && (int32_t)value == powerVectors[i].expected;
    }
    hilCheck("read_power_uw", passed, "");
    step("expect_rx power %s", "03030303");

    // Giá trị thô
    int16_t shuntRaw = 0, currentRaw = 0;
    uint16_t busRaw = 0, powerRaw = 0;
    step("regs %s", "01=8000");
    passed = ina226ReadShuntVoltageRaw(dev, &shuntRaw) && shuntRaw == -32768;
    step("regs %s", "02=7FFF");
    passed &= ina226ReadBusVoltageRaw(dev, &busRaw) && busRaw == 0x7FFF;
    step("regs %s", "04=FC18");
    passed &= ina226ReadCurrentRaw(dev, &currentRaw) && currentRaw == -1000;
    step("regs %s", "03=FFFF");
    passed &= ina226ReadPowerRaw(dev, &powerRaw) && powerRaw == 0xFFFF;
    hilCheck("read_raw", passed, "");
    step("expect_rx raw %s", "01020403");

    // Thiết bị không trả lời (0x42): false, đầu ra giữ nguyên
    INA226_DriverHandle_t absent = *dev;
    absent.slaveAddress = INA226_ADDRESS_0X42;
    int32_t keep = 12345;
    uint32_t keepUnsigned = 54321;
    hilCheck("read_nack", !ina226ReadCurrentNa(&absent, &keep) && keep == 12345 &&
                              !ina226ReadBusVoltageMv(&absent, &keepUnsigned) && keepUnsigned == 54321 &&
                              !ina226ReadPowerUw(&absent, &keepUnsigned), "");
}

static void testAlert(const INA226_DriverHandle_t* dev)
{
    // Đổi đơn vị -> thô, làm tròn gần nhất
    hilCheck("limit_shunt", ina226ShuntVoltageUvToRaw(40000) == 16000 && ina226ShuntVoltageUvToRaw(-40000) == 0xC180 &&
                                ina226ShuntVoltageUvToRaw(1) == 0 && ina226ShuntVoltageUvToRaw(2) == 1 &&
                                ina226ShuntVoltageUvToRaw(3) == 1 && ina226ShuntVoltageUvToRaw(-2) == 0xFFFF &&
                                ina226ShuntVoltageUvToRaw(100000) == 0x7FFF && ina226ShuntVoltageUvToRaw(-100000) == 0x8000,
             "");
    hilCheck("limit_bus", ina226BusVoltageMvToRaw(5000) == 4000 && ina226BusVoltageMvToRaw(1) == 1 &&
                              ina226BusVoltageMvToRaw(2) == 2 && ina226BusVoltageMvToRaw(3) == 2 &&
                              ina226BusVoltageMvToRaw(50000) == 0x7FFF, "");
    hilCheck("limit_power", ina226PowerUwToRaw(dev, 500000) == 655 && ina226PowerUwToRaw(dev, 0) == 0 &&
                                ina226PowerUwToRaw(dev, 4000000000UL) == 0xFFFF, "");

    // Ghi ngưỡng rồi Mask/Enable: SOL (latch, cao), SUL (thấp), BUL (latch), POL, CNVR (không ghi ngưỡng), NONE
    uint8_t passed = ina226SetAlert(dev, INA226_ALERT_SHUNT_OVER_LIMIT, ina226ShuntVoltageUvToRaw(40000), true,
                                    INA226_ALERT_POLARITY_ACTIVE_HIGH);
    passed &= ina226SetAlert(dev, INA226_ALERT_SHUNT_UNDER_LIMIT, ina226ShuntVoltageUvToRaw(-40000), false,
                             INA226_ALERT_POLARITY_ACTIVE_LOW);
    passed &= ina226SetAlert(dev, INA226_ALERT_BUS_UNDER_LIMIT, ina226BusVoltageMvToRaw(5000), true,
                             INA226_ALERT_POLARITY_ACTIVE_LOW);
    passed &= ina226SetAlert(dev, INA226_ALERT_POWER_OVER_LIMIT, ina226PowerUwToRaw(dev, 500000), false,
                             INA226_ALERT_POLARITY_ACTIVE_LOW);
    passed &= ina226SetAlert(dev, INA226_ALERT_CONVERSION_READY, 0, false, INA226_ALERT_POLARITY_ACTIVE_LOW);
    passed &= ina226SetAlert(dev, INA226_ALERT_NONE, 0, false, INA226_ALERT_POLARITY_ACTIVE_LOW);
    hilCheck("set_alert", passed, "");
    step("expect_rx alert %s", "073E80068003" "07C180064000" "070FA0061001" "07028F060800" "060400" "060000");

    hilCheck("set_alert_bad", !ina226SetAlert(dev, (INA226_AlertFunction_t)0x0001, 0, false,
                                              INA226_ALERT_POLARITY_ACTIVE_LOW), "");
    step("expect_rx alert_bad %s", "-");

    // Cờ trạng thái lần lượt: AFF + OVF (0x0014), CVRF (0x0008), OVF (0x0004), cả ba (0x001C)
    INA226_AlertFlags_t f[4] = {0};
    passed = 1;
    step("regs %s", "06=0014,0008,0004,001C");
    for (uint8_t i = 0; i < 4; i++) passed &= ina226ReadAlertFlags(dev, &f[i]);
    passed &= f[0].alertFunction && !f[0].conversionReady && f[0].overflow;
    passed &= !f[1].alertFunction && f[1].conversionReady && !f[1].overflow;
    passed &= !f[2].alertFunction && !f[2].conversionReady && f[2].overflow;
    passed &= f[3].alertFunction && f[3].conversionReady && f[3].overflow;
    hilCheck("alert_flags", passed, "");
    step("expect_rx flags %s", "06060606");

    // ina226ClearAlert đọc Mask/Enable một lần
    step("regs %s", "06=0010");
    hilCheck("clear_alert", ina226ClearAlert(dev), "");
    step("expect_rx clear %s", "06");

    // ina226IsConversionReady: có cờ CVRF -> true, không -> false
    step("regs %s", "06=0008,0000");
    passed = ina226IsConversionReady(dev);
    passed &= !ina226IsConversionReady(dev);
    hilCheck("conversion_ready", passed, "");
    step("expect_rx ready %s", "0606");

    // ina226WaitConversion: cờ lên ở lần kiểm tra thứ 3 -> true, dừng đúng lúc
    step("regs %s", "06=0000,0000,0008");
    hilCheck("wait_ok", ina226WaitConversion(dev, 10), "");
    step("expect_rx wait_ok %s", "060606");

    // Không bao giờ có cờ: kiểm tra (timeout + 1) lần rồi báo hết giờ
    step("regs %s", "06=0000");
    hilCheck("wait_timeout", !ina226WaitConversion(dev, 3), "");
    step("expect_rx wait_timeout %s", "06060606");
}

int main(void)
{
    SystemInit();
    Delay_Ms(1000); // Cửa sổ nạp

    hilBegin("ina226");
    step("slave_on%s", "");
    hilCheck("i2c_init", i2cInit(&bus), "");

    INA226_DriverHandle_t dev = {0};
    testInit(&dev);
    testConfiguration(&dev);
    testIds(&dev);
    testReadings(&dev);
    testAlert(&dev);

    hilWait("done");
    hilEnd();
}

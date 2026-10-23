#include "ch32fun.h"
#include "ch32v003_bmi160.h"
#include "hil.h"

// Test HIL cho ch32v003_bmi160: ESP32 làm I2C slave 0x69 trên PC1 (SDA) / PC2 (SCL) giả lập BMI160.
// ESP32 không kéo giãn SCL nên phải nạp sẵn byte trả lời ngay trước từng lần đọc (byte thừa nằm lại FIFO làm hỏng
// các lần ghi sau): firmware link với -Wl,--wrap=i2cReadBurst, mỗi lần thư viện đọc thanh ghi thì báo "read <reg>"
// để runner nạp giá trị của thanh ghi đó từ bảng do bước "regs <reg>=<hex>[,<hex>...]" khai báo (nhiều giá trị =
// lần lượt cho các lần đọc). Runner so chuỗi byte chip đã ghi (địa chỉ thanh ghi + giá trị cấu hình).
// Phần toán (góc, bộ lọc bù, đổi trục, vector trọng lực) kiểm tra trên chip với giá trị mong đợi tính bằng
// mô hình số thực (atan2, Tau lý tưởng).

#define ANGLE_TOLERANCE 40 // centi-độ

static const I2C_DriverHandle_t bus = {
    .driverMode = I2C_MODE_HARDWARE,
    .config.hardware = {I2C_REMAP_DEFAULT, I2C_SPEED_FAST},
};

static char text[80];
static char detail[48];
static char readText[16];

uint8_t __real_i2cReadBurst(const I2C_DriverHandle_t* handle, uint8_t slaveAddress, uint8_t startRegister,
                            uint8_t* buffer, uint8_t length);

// Mọi lần đọc tới cảm biến giả lập: runner nạp sẵn byte của thanh ghi rồi mới phát giao dịch
uint8_t __wrap_i2cReadBurst(const I2C_DriverHandle_t* handle, const uint8_t slaveAddress, const uint8_t startRegister,
                            uint8_t* buffer, const uint8_t length)
{
    if (slaveAddress == BMI160_ADDRESS_VCC)
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

static int32_t absolute(const int32_t value)
{
    return value < 0 ? -value : value;
}

static int near(const int32_t value, const int32_t expected)
{
    return absolute(value - expected) <= ANGLE_TOLERANCE;
}

static BMI160_SensorData_t sample(const int16_t ax, const int16_t ay, const int16_t az,
                                  const int16_t gx, const int16_t gy, const int16_t gz)
{
    const BMI160_SensorData_t data = {{ax, ay, az}, {gx, gy, gz}};
    return data;
}

// ============================================================================
// GIAO TIẾP I2C
// ============================================================================

static void testInit(void)
{
    // Tham số sai: từ chối, không phát giao dịch nào
    BMI160_DriverHandle_t noBus = {0};
    BMI160_DriverHandle_t badGyro = {.i2cHandle = &bus, .gyroRange = (BMI160_GyroscopeRange_t)6};
    BMI160_DriverHandle_t badFilter = {.i2cHandle = &bus, .filterMode = (BMI160_FilterMode_t)6};
    hilCheck("init_reject", bmi160Init(NULL) == 0 && bmi160Init(&noBus) == 0 && bmi160Init(&badGyro) == 0 &&
                                bmi160Init(&badFilter) == 0, "NULL / thiếu bus / enum sai");
    step("expect_rx reject %s", "-");

    // Sai Chip ID: chỉ đọc thanh ghi 0x00 rồi dừng
    BMI160_DriverHandle_t dev = {.i2cHandle = &bus};
    step("regs %s", "00=D0");
    hilCheck("init_wrong_id", bmi160Init(&dev) == 0, "");
    step("expect_rx wrong_id %s", "00");

    // Địa chỉ 0x68 không có thiết bị: NACK -> 0
    BMI160_DriverHandle_t absent = {.i2cHandle = &bus, .slaveAddress = BMI160_ADDRESS_GND};
    hilCheck("init_absent", bmi160Init(&absent) == 0, "");

    // Cấu hình đầy đủ: thứ tự lệnh nguồn, mã dải đo (Gyro 2000 dps = 0x00), ODR, thời gian chờ Gyro khởi động
    BMI160_DriverHandle_t custom = {
        .i2cHandle = &bus,
        .slaveAddress = BMI160_ADDRESS_VCC,
        .accelRange = BMI160_ACCEL_RANGE_4G,
        .gyroRange = BMI160_GYRO_RANGE_2000_DPS,
        .outputDataRate = BMI160_ODR_200_HZ,
        .gravityFilterReady = 1,
    };
    step("regs %s", "00=D1 03=14");
    const uint32_t start = SysTick->CNT;
    const uint8_t ok = bmi160Init(&custom);
    const uint32_t elapsedMs = (SysTick->CNT - start) / DELAY_MS_TIME;
    mini_snprintf(detail, sizeof(detail), "ret=%u %lums", ok, elapsedMs);
    hilCheck("init_custom", ok == 1 && custom.gyroRange == BMI160_GYRO_RANGE_2000_DPS && custom.gravityFilterReady == 0,
             detail);
    hilCheck("init_startup_wait", elapsedMs >= BMI160_ACCEL_STARTUP_MS + BMI160_GYRO_STARTUP_MS, detail);
    step("expect_rx custom %s", "007E117E15410543004029422903");

    // Handle để trống: điền mặc định 0x69, ±2g, ±250 dps, 100Hz
    BMI160_DriverHandle_t defaults = {.i2cHandle = &bus};
    step("regs %s", "00=D1 03=14");
    const uint8_t okDefault = bmi160Init(&defaults);
    hilCheck("init_default", okDefault == 1 && defaults.slaveAddress == BMI160_ADDRESS_VCC &&
                                 defaults.accelRange == BMI160_ACCEL_RANGE_2G &&
                                 defaults.gyroRange == BMI160_GYRO_RANGE_250_DPS &&
                                 defaults.outputDataRate == BMI160_ODR_100_HZ, "");
    step("expect_rx default %s", "007E117E15410343034028422803");

    // Gyro không vào Normal (PMU_STATUS = 0x04): báo lỗi
    BMI160_DriverHandle_t pmu = {.i2cHandle = &bus};
    step("regs %s", "00=D1 03=10");
    hilCheck("init_pmu_fail", bmi160Init(&pmu) == 0, "");
    step("expect_rx pmu %s", "007E117E15410343034028422803");
}

static void testReadData(BMI160_DriverHandle_t* dev)
{
    // Gyro X/Y/Z = 0x1234, -2, 32767; Accel X/Y/Z = -32768, 0x0102, 0x4000 (byte thấp trước)
    BMI160_SensorData_t data = {0};
    step("regs %s", "0C=3412FEFFFF7F008002010040");
    const uint8_t ok = bmi160ReadData(dev, &data);
    mini_snprintf(detail, sizeof(detail), "g=%d,%d,%d a=%d,%d,%d", data.gyroscope.x, data.gyroscope.y,
                  data.gyroscope.z, data.accelerometer.x, data.accelerometer.y, data.accelerometer.z);
    hilCheck("read_data", ok && data.gyroscope.x == 0x1234 && data.gyroscope.y == -2 && data.gyroscope.z == 32767 &&
                              data.accelerometer.x == -32768 && data.accelerometer.y == 0x0102 &&
                              data.accelerometer.z == 0x4000, detail);
    step("expect_rx read %s", "0C");
}

static void testCalibrate(BMI160_DriverHandle_t* dev)
{
    // 2 mẫu nằm yên nghiêng ~10° (roll 10.02° và 10.19°): offset = trung bình
    step("regs %s", "0C=0000000000000000220B073F,0000000000000000540B023F");
    const uint8_t ok = bmi160CalibrateZeroOffset(dev, 2);
    mini_snprintf(detail, sizeof(detail), "ret=%u roll=%ld pitch=%ld ay=%d", ok, dev->offsetRollCenti,
                  dev->offsetPitchCenti, dev->offsetAccelY);
    hilCheck("calibrate", ok && absolute(dev->offsetRollCenti - 1010) <= 3 && dev->offsetPitchCenti == 0 &&
                              dev->offsetAccelX == 0 && dev->offsetAccelY == 2875, detail);
    step("expect_rx calibrate %s", "0C0C");

    // Đang chuyển động (0° rồi 10°): hủy, giữ offset cũ
    step("regs %s", "0C=000000000000000000000040,0000000000000000220B073F");
    const uint8_t moving = bmi160CalibrateZeroOffset(dev, 2);
    mini_snprintf(detail, sizeof(detail), "ret=%u roll=%ld", moving, dev->offsetRollCenti);
    hilCheck("calibrate_moving", moving == 0 && absolute(dev->offsetRollCenti - 1010) <= 3 &&
                                     dev->offsetAccelY == 2875, detail);
    step("expect_rx moving %s", "0C0C");

    // Lắp xoay 180°: mẫu được đổi trục trước khi lấy trung bình (Y -> -Y)
    BMI160_DriverHandle_t rotated = {.i2cHandle = &bus, .slaveAddress = BMI160_ADDRESS_VCC,
                                     .mountOrientation = BMI160_MOUNT_ROTATE_180};
    step("regs %s", "0C=0000000000000000220B073F,0000000000000000540B023F");
    const uint8_t okRotated = bmi160CalibrateZeroOffset(&rotated, 2);
    mini_snprintf(detail, sizeof(detail), "ret=%u roll=%ld ay=%d", okRotated, rotated.offsetRollCenti,
                  rotated.offsetAccelY);
    hilCheck("calibrate_mount", okRotated && absolute(rotated.offsetRollCenti + 1010) <= 3 &&
                                    rotated.offsetAccelY == -2875, detail);
    step("expect_rx mount %s", "0C0C");
}

// ============================================================================
// TOÁN (KHÔNG CẦN I2C)
// ============================================================================

// Chạy bộ lọc n bước với cùng một mẫu, trả roll sau bước cuối (angles bắt đầu từ góc phẳng)
static int32_t runFilter(const BMI160_DriverHandle_t* dev, const BMI160_SensorData_t* data,
                         const uint32_t deltaTime, const uint16_t steps)
{
    BMI160_EulerAngles_t angles = {0};
    const BMI160_SensorData_t flat = sample(0, 0, 16384, 0, 0, 0);
    bmi160GetTiltAngles(dev, &flat, &angles, deltaTime);
    for (uint16_t i = 0; i < steps; i++) bmi160GetTiltAngles(dev, data, &angles, deltaTime);
    return angles.roll;
}

static void testTiltAngles(void)
{
    BMI160_DriverHandle_t dev = {.gyroRange = BMI160_GYRO_RANGE_250_DPS, .filterMode = BMI160_FILTER_MODE_BALANCED};
    BMI160_EulerAngles_t angles = {0};

    // Lần gọi đầu lấy thẳng góc gia tốc: roll = atan2(Y, √(X² + Z²)), pitch = atan2(-X, √(Y² + Z²))
    BMI160_SensorData_t data = sample(0, 8192, 14189, 0, 0, 0);
    bmi160GetTiltAngles(&dev, &data, &angles, 10);
    mini_snprintf(detail, sizeof(detail), "roll=%ld pitch=%ld", angles.roll, angles.pitch);
    hilCheck("angle_roll30", near(angles.roll, 3000) && near(angles.pitch, 0), detail);

    angles = (BMI160_EulerAngles_t){0};
    data = sample(-8192, 0, 14189, 0, 0, 0);
    bmi160GetTiltAngles(&dev, &data, &angles, 10);
    mini_snprintf(detail, sizeof(detail), "roll=%ld pitch=%ld", angles.roll, angles.pitch);
    hilCheck("angle_pitch30", near(angles.roll, 0) && near(angles.pitch, 3000), detail);

    angles = (BMI160_EulerAngles_t){0};
    data = sample(5000, -7000, 12000, 0, 0, 0);
    bmi160GetTiltAngles(&dev, &data, &angles, 10);
    mini_snprintf(detail, sizeof(detail), "roll=%ld pitch=%ld", angles.roll, angles.pitch);
    hilCheck("angle_mixed", near(angles.roll, -2830) && near(angles.pitch, -1979), detail);

    // Offset hiệu chuẩn trừ một lần ở đầu ra, không cộng dồn qua các lần gọi
    dev.offsetRollCenti = 1000;
    angles = (BMI160_EulerAngles_t){0};
    data = sample(0, 8192, 14189, 0, 0, 0);
    for (uint8_t i = 0; i < 20; i++) bmi160GetTiltAngles(&dev, &data, &angles, 10);
    mini_snprintf(detail, sizeof(detail), "roll=%ld", angles.roll);
    hilCheck("angle_offset", near(angles.roll, 2000), detail);
    dev.offsetRollCenti = 0;

    // Tau: góc nhảy bậc 0 -> 30°, sau 1 Tau đạt 30 × (1 - (Tau / (Tau + dt))^n) bất kể chu kỳ quét
    const BMI160_SensorData_t tilted = sample(0, 8192, 14189, 0, 0, 0);
    int32_t roll = runFilter(&dev, &tilted, 10, 50);
    mini_snprintf(detail, sizeof(detail), "roll=%ld (1885)", roll);
    hilCheck("tau500_dt10", near(roll, 1885), detail);
    roll = runFilter(&dev, &tilted, 100, 5);
    mini_snprintf(detail, sizeof(detail), "roll=%ld (1794)", roll);
    hilCheck("tau500_dt100", near(roll, 1794), detail);
    roll = runFilter(&dev, &tilted, 5, 100);
    mini_snprintf(detail, sizeof(detail), "roll=%ld (1891)", roll);
    hilCheck("tau500_dt5", near(roll, 1891), detail);
    dev.filterMode = BMI160_FILTER_MODE_ULTRA_SMOOTH;
    roll = runFilter(&dev, &tilted, 5, 200);
    mini_snprintf(detail, sizeof(detail), "roll=%ld (1894)", roll);
    hilCheck("tau1000_dt5", near(roll, 1894), detail);

    // Con quay 100°/s quanh X, gia tốc phẳng, Tau 1000ms, 10 bước 10ms: 9.47° (cùng kết quả ở 2 dải đo)
    const BMI160_SensorData_t spin250 = sample(0, 0, 16384, 13120, 0, 0);
    roll = runFilter(&dev, &spin250, 10, 10);
    mini_snprintf(detail, sizeof(detail), "roll=%ld (947)", roll);
    hilCheck("gyro_250dps", near(roll, 947), detail);
    const BMI160_SensorData_t spinBack = sample(0, 0, 16384, -13120, 0, 0);
    roll = runFilter(&dev, &spinBack, 10, 10);
    mini_snprintf(detail, sizeof(detail), "roll=%ld (-947)", roll);
    hilCheck("gyro_250dps_negative", near(roll, -947), detail);
    dev.gyroRange = BMI160_GYRO_RANGE_2000_DPS;
    const BMI160_SensorData_t spin2000 = sample(0, 0, 16384, 1640, 0, 0);
    roll = runFilter(&dev, &spin2000, 10, 10);
    mini_snprintf(detail, sizeof(detail), "roll=%ld (947)", roll);
    hilCheck("gyro_2000dps", near(roll, 947), detail);

    // Pitch theo con quay Y
    BMI160_EulerAngles_t pitchAngles = {0};
    const BMI160_SensorData_t flat = sample(0, 0, 16384, 0, 0, 0);
    const BMI160_SensorData_t spinY = sample(0, 0, 16384, 0, 1640, 0);
    bmi160GetTiltAngles(&dev, &flat, &pitchAngles, 10);
    for (uint8_t i = 0; i < 10; i++) bmi160GetTiltAngles(&dev, &spinY, &pitchAngles, 10);
    mini_snprintf(detail, sizeof(detail), "pitch=%ld roll=%ld", pitchAngles.pitch, pitchAngles.roll);
    hilCheck("gyro_pitch", near(pitchAngles.pitch, 947) && near(pitchAngles.roll, 0), detail);
}

static int axesEqual(const BMI160_SensorData_t* d, const int16_t ax, const int16_t ay, const int16_t az,
                     const int16_t gx, const int16_t gy, const int16_t gz)
{
    return d->accelerometer.x == ax && d->accelerometer.y == ay && d->accelerometer.z == az &&
           d->gyroscope.x == gx && d->gyroscope.y == gy && d->gyroscope.z == gz;
}

static void testRemap(void)
{
    const BMI160_SensorData_t raw = sample(1, 2, 3, 4, 5, 6);
    BMI160_SensorData_t out;
    uint8_t passed = 1;
    bmi160RemapAxes(&raw, &out, BMI160_MOUNT_NORMAL);
    passed &= axesEqual(&out, 1, 2, 3, 4, 5, 6);
    bmi160RemapAxes(&raw, &out, BMI160_MOUNT_ROTATE_90);
    passed &= axesEqual(&out, -2, 1, 3, -5, 4, 6);
    bmi160RemapAxes(&raw, &out, BMI160_MOUNT_ROTATE_180);
    passed &= axesEqual(&out, -1, -2, 3, -4, -5, 6);
    bmi160RemapAxes(&raw, &out, BMI160_MOUNT_ROTATE_270);
    passed &= axesEqual(&out, 2, -1, 3, 5, -4, 6);
    bmi160RemapAxes(&raw, &out, BMI160_MOUNT_SWAP_XY);
    passed &= axesEqual(&out, 2, 1, 3, 5, 4, 6);
    bmi160RemapAxes(&raw, &out, BMI160_MOUNT_FLIP_Z);
    passed &= axesEqual(&out, 1, -2, -3, 4, -5, -6);
    hilCheck("remap_axes", passed, "");
}

static void testGravity2D(void)
{
    BMI160_DriverHandle_t a = {0};
    BMI160_DriverHandle_t b = {0};
    int16_t x = 0, y = 0;

    // Mẫu đầu nạp thẳng vào bộ lọc: đã đúng độ dài ngay
    BMI160_SensorData_t data = sample(10000, 0, 0, 0, 0, 0);
    uint8_t ok = bmi160GetNormalizedGravity2D(&a, &data, 256, &x, &y);
    mini_snprintf(detail, sizeof(detail), "ret=%u x=%d y=%d", ok, x, y);
    hilCheck("gravity_first", ok && x == 256 && y == 0, detail);

    // Cảm biến B không ảnh hưởng bộ lọc của A
    data = sample(0, -8000, 0, 0, 0, 0);
    ok = bmi160GetNormalizedGravity2D(&b, &data, 256, &x, &y);
    const uint8_t bOk = ok && x == 0 && y == -256;
    data = sample(10000, 0, 0, 0, 0, 0);
    ok = bmi160GetNormalizedGravity2D(&a, &data, 256, &x, &y);
    mini_snprintf(detail, sizeof(detail), "B ok=%u, A x=%d y=%d", bOk, x, y);
    hilCheck("gravity_independent", bOk && ok && x == 256 && y == 0, detail);

    // Lọc alpha 1/2: A từ (10000, 0) sang (0, 10000) -> (5000, 5000), hướng 45°
    data = sample(0, 10000, 0, 0, 0, 0);
    ok = bmi160GetNormalizedGravity2D(&a, &data, 256, &x, &y);
    mini_snprintf(detail, sizeof(detail), "x=%d y=%d", x, y);
    hilCheck("gravity_filter", ok && x == y && x > 170 && x < 190, detail);

    // Dưới ngưỡng (nằm phẳng): trả 0, giữ nguyên đầu ra
    BMI160_DriverHandle_t c = {0};
    data = sample(100, -100, 16384, 0, 0, 0);
    x = 7;
    y = 7;
    ok = bmi160GetNormalizedGravity2D(&c, &data, 256, &x, &y);
    hilCheck("gravity_flat", ok == 0 && x == 7 && y == 7, "");

    // Offset hiệu chuẩn: tư thế đã hiệu chuẩn cho vector (0, 0)
    BMI160_DriverHandle_t d = {.offsetAccelX = 3000, .offsetAccelY = -2000};
    data = sample(3000, -2000, 15000, 0, 0, 0);
    hilCheck("gravity_offset", bmi160GetNormalizedGravity2D(&d, &data, 256, &x, &y) == 0, "");
    data = sample(3000, 6000, 15000, 0, 0, 0); // Lọc: (3000, 2000) - offset = (0, 4000)
    ok = bmi160GetNormalizedGravity2D(&d, &data, 100, &x, &y);
    mini_snprintf(detail, sizeof(detail), "ret=%u x=%d y=%d", ok, x, y);
    hilCheck("gravity_offset_dir", ok && x == 0 && y == 100, detail);
}

int main(void)
{
    SystemInit();
    Delay_Ms(1000); // Cửa sổ nạp

    hilBegin("bmi160");
    step("slave_on%s", "");
    hilCheck("i2c_init", i2cInit(&bus), "");

    testInit();
    BMI160_DriverHandle_t dev = {.i2cHandle = &bus, .slaveAddress = BMI160_ADDRESS_VCC};
    testReadData(&dev);
    testCalibrate(&dev);

    testTiltAngles();
    testRemap();
    testGravity2D();

    hilWait("done");
    hilEnd();
}

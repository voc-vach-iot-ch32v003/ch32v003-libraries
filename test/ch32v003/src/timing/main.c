#include "ch32fun.h"
#include "ch32v003_delay.h"
#include "ch32v003_gpio.h"
#include "ch32v003_timer.h"
#include "hil.h"

// Test HIL cho ch32v003_delay + ch32v003_timer: chip phát xung trên PA2 (CH11, có trên cả 3 gói vỏ),
// ESP32 đo độ dài từng đoạn mức logic (CAP) để đối chiếu với thời gian mong muốn.

#define SIGNAL_PIN ((MCU_PORT_INDEX_A << 4) | 2) // PA2

static char text[48];

// Báo runner chuẩn bị bắt sườn: <tên> <số đoạn> <độ dài mong muốn us> <sai số cho phép us>
static void armCapture(const char* name, const int segments, const uint32_t expectUs, const uint32_t toleranceUs)
{
    digitalWrite(SIGNAL_PIN, LOW);
    mini_snprintf(text, sizeof(text), "cap %s %d %lu %lu", name, segments, expectUs, toleranceUs);
    hilWait(text);
}

// 5 đoạn xen kẽ HIGH/LOW, mỗi đoạn dùng hàm chờ cần kiểm tra
static void pulsesWithDelayMs(const uint32_t ms)
{
    for (uint8_t i = 0; i < 5; i++)
    {
        digitalToggle(SIGNAL_PIN);
        delayMs(ms);
    }
    digitalWrite(SIGNAL_PIN, LOW);
}

static void pulsesWithDelayUs(const uint32_t us)
{
    for (uint8_t i = 0; i < 5; i++)
    {
        digitalToggle(SIGNAL_PIN);
        delayUs(us);
    }
    digitalWrite(SIGNAL_PIN, LOW);
}

static void pulsesWithMillis(const uint32_t ms)
{
    for (uint8_t i = 0; i < 5; i++)
    {
        digitalToggle(SIGNAL_PIN);
        const uint32_t start = millis();
        while (elapsedMs(millis(), start) < ms)
        {
        }
    }
    digitalWrite(SIGNAL_PIN, LOW);
}

static void pulsesWithMicros(const uint32_t us)
{
    for (uint8_t i = 0; i < 5; i++)
    {
        digitalToggle(SIGNAL_PIN);
        const uint32_t start = micros();
        while (elapsedUs(micros(), start) < us)
        {
        }
    }
    digitalWrite(SIGNAL_PIN, LOW);
}

int main(void)
{
    SystemInit();
    delayMs(1000); // Cửa sổ nạp

    hilBegin("timing");
    pinMode(SIGNAL_PIN, OUTPUT);
    timerInit();

    // delayMs dùng làm thước đo clock của chip (HSI lệch tới ±2%), các phép đo sau so theo clock này
    armCapture("delayMs_20", 5, 20000, 400);
    pulsesWithDelayMs(20);

    armCapture("delayUs_200", 5, 200, 6);
    pulsesWithDelayUs(200);

    // millis() nhảy theo bước 1ms: đoạn đầu ngắn hơn tùy pha bắt đầu, runner bỏ qua đoạn đầu
    armCapture("millis_20", 5, 20000, 60);
    pulsesWithMillis(20);

    armCapture("micros_500", 5, 500, 8);
    pulsesWithMicros(500);

    // Chi phí 1 lần gọi micros() tính bằng chu kỳ máy (SysTick chạy ở HCLK)
    const uint32_t costStart = ticks();
    for (uint8_t i = 0; i < 100; i++)
    {
        (void)micros();
    }
    const uint32_t costCycles = (ticks() - costStart) / 100;
    mini_snprintf(text, sizeof(text), "%lu_chu_ky", costCycles);
    hilCheck("micros_cost", costCycles < 200, text); // Ngưỡng chống hồi quy (bản cũ dùng nhân/chia mềm ~480 chu kỳ)

    // micros() phải tăng đơn điệu và khớp millis() (sai lệch < 2ms) trong 3 giây
    uint32_t lastUs = micros();
    uint8_t monotonic = 1;
    const uint32_t startMs = millis();
    while (elapsedMs(millis(), startMs) < 3000)
    {
        const uint32_t nowUs = micros();
        if ((int32_t)(nowUs - lastUs) < 0) monotonic = 0;
        lastUs = nowUs;
    }
    hilCheck("micros_monotonic", monotonic, "");
    const int32_t driftUs = (int32_t)(micros() - millis() * 1000);
    mini_snprintf(text, sizeof(text), "lech=%ldus", driftUs);
    hilCheck("micros_matches_millis", driftUs > -2000 && driftUs < 2000, text);

    hilEnd();
}

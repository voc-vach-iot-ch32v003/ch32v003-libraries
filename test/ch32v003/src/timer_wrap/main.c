#include "ch32fun.h"
#include "ch32v003_delay.h"
#include "ch32v003_gpio.h"
#include "ch32v003_timer.h"
#include "hil.h"

// Test HIL dài cho ch32v003_timer: chạy QUA mốc tràn của SysTick 32-bit (2^32 / 48MHz = 89.48 giây).
// Bản cũ chia thẳng SysTick->CNT nên millis() quay về 0 tại mốc này.
//   1. Thước đo clock: 2 sườn lên cách nhau delayMs(5000), ESP32 đo bằng thạch anh của nó.
//   2. 2 sườn lên cách nhau đúng 95000ms theo millis() -> ESP32 đo, phải khớp 95s x tỉ lệ clock.
//   3. Trong lúc chờ: millis()/micros() không được giảm, và phải khớp số tick SysTick đếm được.

#define SIGNAL_PIN ((MCU_PORT_INDEX_A << 4) | 2) // PA2 (CH11)
#define WRAP_TEST_MS 95000UL

static char text[64];

static void risingEdge(void)
{
    digitalWrite(SIGNAL_PIN, HIGH);
    delayUs(200);
    digitalWrite(SIGNAL_PIN, LOW);
}

int main(void)
{
    SystemInit();
    delayMs(1000); // Cửa sổ nạp

    hilBegin("timer_wrap");
    pinMode(SIGNAL_PIN, OUTPUT);
    digitalWrite(SIGNAL_PIN, LOW);
    timerInit();

    // 1. Thước đo clock của chip
    hilWait("interval ref 5000");
    risingEdge();
    delayMs(5000);
    risingEdge();

    // 2. Chạy qua mốc tràn SysTick
    mini_snprintf(text, sizeof(text), "SysTick.CNT=%lu se_tran_sau_%lus", SysTick->CNT,
                  (0xFFFFFFFFUL - SysTick->CNT) / DELAY_MS_TIME / 1000);
    hilCheck("systick_start", 1, text);
    hilWait("interval wrap 95000");

    risingEdge();
    const uint32_t startMs = millis();
    const uint32_t startUs = micros();
    uint32_t lastMs = startMs;
    uint32_t lastUs = startUs;
    uint32_t lastCnt = SysTick->CNT;
    uint64_t tickSum = 0;
    uint16_t msBackward = 0;
    uint16_t usBackward = 0;
    uint8_t wraps = 0;

    while (elapsedMs(lastMs, startMs) < WRAP_TEST_MS)
    {
        const uint32_t nowMs = millis();
        const uint32_t nowUs = micros();
        const uint32_t nowCnt = SysTick->CNT;
        if ((int32_t)(nowMs - lastMs) < 0) msBackward++;
        if ((int32_t)(nowUs - lastUs) < 0) usBackward++;
        if (nowCnt < lastCnt) wraps++;
        tickSum += (uint32_t)(nowCnt - lastCnt); // Hiệu không dấu: đúng cả khi CNT tràn
        lastMs = nowMs;
        lastUs = nowUs;
        lastCnt = nowCnt;
    }
    risingEdge();

    const uint32_t elapsedMillis = elapsedMs(lastMs, startMs);
    const uint32_t tickMillis = (uint32_t)(tickSum / DELAY_MS_TIME);
    const uint32_t microsAsMillis = elapsedUs(lastUs, startUs) / 1000;

    mini_snprintf(text, sizeof(text), "so_lan_tran=%u", wraps);
    hilCheck("systick_wrapped", wraps >= 1, text);
    mini_snprintf(text, sizeof(text), "so_lan_giam=%u", msBackward);
    hilCheck("millis_monotonic", msBackward == 0, text);
    mini_snprintf(text, sizeof(text), "so_lan_giam=%u", usBackward);
    hilCheck("micros_monotonic", usBackward == 0, text);
    mini_snprintf(text, sizeof(text), "millis=%lu ticks=%lu", elapsedMillis, tickMillis);
    hilCheck("millis_matches_systick", (int32_t)(elapsedMillis - tickMillis) <= 2 && (int32_t)(tickMillis - elapsedMillis) <= 2, text);
    mini_snprintf(text, sizeof(text), "micros/1000=%lu millis=%lu", microsAsMillis, elapsedMillis);
    hilCheck("micros_matches_millis", (int32_t)(microsAsMillis - elapsedMillis) <= 2 && (int32_t)(elapsedMillis - microsAsMillis) <= 2, text);

    hilEnd();
}

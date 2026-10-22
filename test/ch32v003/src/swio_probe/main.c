#include "ch32fun.h"
#include "ch32v003_delay.h"
#include "ch32v003_gpio.h"

// Thí nghiệm: đổi chân nào sang ANALOG thì SWIO mất kết nối? (chạy bằng test/runner/swio_probe.py)
// 5 giây đầu giữ nguyên mọi thứ (vẫn nạp lại được), sau đó đổi nhóm chân PROBE_PINS sang ANALOG rồi đứng yên.
//   PROBE_PINS = 0: không đổi gì (đối chứng) | 1: PD1 (SWIO) | 2: PD4 + PD5 (cùng chân 8 với PD1 trên J4M6)
//              | 3: mọi chân của GPIOA / C / D (giống bản standby_hold đã làm J4M6 mất kết nối)

#ifndef PROBE_PINS
#define PROBE_PINS 0
#endif

int main(void)
{
    SystemInit();
    delayMs(5000);

#if PROBE_PINS == 1
    pinModePort(GPIOD, 1, ANALOG);
#elif PROBE_PINS == 2
    pinModePort(GPIOD, 4, ANALOG);
    pinModePort(GPIOD, 5, ANALOG);
#elif PROBE_PINS == 3
    for (uint8_t pin = 0; pin < 8; pin++)
    {
        pinModePort(GPIOA, pin, ANALOG);
        pinModePort(GPIOC, pin, ANALOG);
        pinModePort(GPIOD, pin, ANALOG);
    }
#endif

    // ReSharper disable once CppDFAEndlessLoop
    while (1)
    {
    }
}

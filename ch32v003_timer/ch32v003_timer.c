#include "ch32v003_timer.h"

#if (TIMER_ARDUINO_STYLE_ENABLE)

uint32_t ticks(void)
{
    return funSysTick32();
}

/**
 * @brief Triển khai hàm đọc micro-giây hệ thống
 */
uint32_t micros(void)
{
    // Quy đổi sang đơn vị us (Ví dụ: 48MHz thì chia cho 48) [ch32fun]
    return ticks() / (FUNCONF_SYSTEM_CORE_CLOCK / 1000000);
}

/**
 * @brief Triển khai hàm đọc mili-giây hệ thống
 */
uint32_t millis(void)
{
    return ticks() / (FUNCONF_SYSTEM_CORE_CLOCK / 1000);
}

/**
 * @brief Tính chênh lệch micro-giây
 */
uint32_t elapsedUs(const uint32_t current, const uint32_t start)
{
    return (current - start);
}

/**
 * @brief Tính chênh lệch mili-giây
 */
uint32_t elapsedMs(const uint32_t current, const uint32_t start)
{
    return (current - start);
}

#endif // TIMER_ARDUINO_STYLE_ENABLE

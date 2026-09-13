#include "ch32v003_delay.h"

#if (DELAY_ARDUINO_STYLE_ENABLE)
void delayMs(const uint32_t ms)
{
    Delay_Ms(ms); // Gọi hàm delay gốc siêu chính xác của ch32v003fun
}

void delayUs(const uint32_t us)
{
    Delay_Us(us); // Gọi hàm delay gốc siêu chính xác của ch32v003fun
}

#endif // DELAY_ENABLE_ARDUINO_STYLE

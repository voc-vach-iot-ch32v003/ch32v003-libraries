#include "ch32fun.h"
#include "ch32v003_delay.h"
#include "ch32v003_intmath.h"
#include "ch32v003_timer.h"
#include "hil.h"
#include "vectors.h"

// Test HIL cho ch32v003_intmath: bản build RV32EC phải cho kết quả GIỐNG HỆT bản build trên máy tính
// (bảng vectors.h sinh từ test/host), đồng thời đo chi phí từng hàm bằng chu kỳ máy.

#define COUNT(a) (sizeof(a) / sizeof((a)[0]))

static char text[48];

// Đo số chu kỳ trung bình của 1 lần gọi (SysTick chạy ở HCLK = chu kỳ máy)
#define MEASURE_CYCLES(result, expression)          \
    do                                              \
    {                                               \
        const uint32_t start = ticks();             \
        for (uint8_t n = 0; n < 32; n++)            \
        {                                           \
            volatile int32_t sink = (expression);   \
            (void)sink;                             \
        }                                           \
        (result) = (ticks() - start) >> 5;          \
    } while (0)

int main(void)
{
    SystemInit();
    delayMs(1000); // Cửa sổ nạp

    hilBegin("intmath");

    uint8_t bad = 0;
    for (uint8_t i = 0; i < COUNT(atanVectors); i++)
    {
        if (intMathAtan2Centi(atanVectors[i][0], atanVectors[i][1]) != atanVectors[i][2]) bad++;
    }
    mini_snprintf(text, sizeof(text), "sai=%u/%u", bad, (unsigned)COUNT(atanVectors));
    hilCheck("atan2_matches_host", bad == 0, text);

    bad = 0;
    for (uint8_t i = 0; i < COUNT(magVectors); i++)
    {
        if (intMathMagnitude2D((int16_t)magVectors[i][0], (int16_t)magVectors[i][1]) != (uint16_t)magVectors[i][2]) bad++;
    }
    mini_snprintf(text, sizeof(text), "sai=%u/%u", bad, (unsigned)COUNT(magVectors));
    hilCheck("magnitude_matches_host", bad == 0, text);

    bad = 0;
    for (uint8_t i = 0; i < COUNT(sqrtVectors); i++)
    {
        if (intMathSqrt32(sqrtVectors[i][0]) != sqrtVectors[i][1]) bad++;
    }
    mini_snprintf(text, sizeof(text), "sai=%u/%u", bad, (unsigned)COUNT(sqrtVectors));
    hilCheck("sqrt_matches_host", bad == 0, text);

    // _Generic phải chọn đúng hàm theo kiểu (int32_t trên RISC-V là long)
    const int16_t minus16 = -32768;
    const int32_t minus32 = -100000;
    hilCheck("abs_generic", intMathAbs(minus16) == 32768u && intMathAbs(minus32) == 100000u, "");

    // Chi phí (chu kỳ máy / lần gọi) để theo dõi hiệu năng giữa các phiên bản
    uint32_t atanCycles, magCycles, sqrtCycles;
    volatile int32_t y = -12345, x = 6789;
    volatile uint32_t v = 4000000000UL;
    MEASURE_CYCLES(atanCycles, intMathAtan2Centi(y, x));
    MEASURE_CYCLES(magCycles, intMathMagnitude2D((int16_t)y, (int16_t)x));
    MEASURE_CYCLES(sqrtCycles, intMathSqrt32(v));
    mini_snprintf(text, sizeof(text), "atan2=%lu mag=%lu sqrt=%lu", atanCycles, magCycles, sqrtCycles);
    hilCheck("cost_cycles", atanCycles < 2000 && magCycles < 200 && sqrtCycles < 1500, text);

    hilEnd();
}

// Kiểm thử thuần logic của ch32v003_intmath trên máy tính (gcc), so sánh với math.h.
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "ch32v003_intmath.h"

static int failures = 0;

#define CHECK(cond, ...) do { if (!(cond)) { failures++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

static double wrapCenti(double e)
{
    while (e > 18000) e -= 36000;
    while (e < -18000) e += 36000;
    return e;
}

int main(void)
{
    // atan2: quét lưới lớn + vùng giá trị nhỏ, sai số phải <= 3 centi-độ
    double worst = 0;
    for (int x = -20000; x <= 20000; x += 37)
    {
        for (int y = -20000; y <= 20000; y += 41)
        {
            if (x == 0 && y == 0) continue;
            const double e = wrapCenti(intMathAtan2Centi(y, x) - atan2(y, x) * 18000.0 / M_PI);
            if (fabs(e) > fabs(worst)) worst = e;
        }
    }
    for (int x = -5; x <= 5; x++)
    {
        for (int y = -5; y <= 5; y++)
        {
            if (x == 0 && y == 0) continue;
            const double e = wrapCenti(intMathAtan2Centi(y, x) - atan2(y, x) * 18000.0 / M_PI);
            if (fabs(e) > fabs(worst)) worst = e;
        }
    }
    CHECK(fabs(worst) <= 3.0, "atan2 sai số %.1f centi-độ", worst);
    CHECK(intMathAtan2Centi(0, 0) == 0, "atan2(0,0)");
    CHECK(abs(intMathAtan2Centi(0, -100) - 18000) <= 2, "atan2(0,-100) = %ld", (long)intMathAtan2Centi(0, -100));

    // magnitude2D: sai số tương đối trong [-3.5%, +1.5%]
    double lo = 0, hi = 0;
    for (int a = -32767; a <= 32767; a += 97)
    {
        for (int b = -32767; b <= 32767; b += 89)
        {
            const double t = hypot(a, b);
            if (t < 100) continue;
            const double e = (intMathMagnitude2D(a, b) - t) / t;
            if (e < lo) lo = e;
            if (e > hi) hi = e;
        }
    }
    CHECK(lo >= -0.035 && hi <= 0.015, "magnitude2D sai số %.2f%% .. %.2f%%", lo * 100, hi * 100);

    // sqrt32: đúng tuyệt đối (floor) tại các điểm biên
    const uint32_t values[] = {0, 1, 2, 3, 4, 15, 16, 99, 100, 65535, 65536, 1000000, 4294836225u, 4294967295u};
    for (unsigned i = 0; i < sizeof(values) / sizeof(values[0]); i++)
    {
        const uint32_t expected = (uint32_t)floor(sqrt((double)values[i]));
        CHECK(intMathSqrt32(values[i]) == expected, "sqrt(%lu) = %u, mong đợi %lu",
              (unsigned long)values[i], intMathSqrt32(values[i]), (unsigned long)expected);
    }

    // clamp + abs + xorshift (chu kỳ không rơi về 0)
    CHECK(intMathClamp(5, 0, 3) == 3 && intMathClamp(-5, 0, 3) == 0 && intMathClamp(2, 0, 3) == 2, "clamp");
    CHECK(intMathAbs((int16_t)-32768) == 32768u, "abs16(-32768)");
    uint16_t state16 = 0;
    for (int i = 0; i < 1000; i++) CHECK(intMathXorshift(&state16) != 0, "xorshift16 = 0");

    printf("intmath: %s (%d lỗi)\n", failures ? "KHÔNG ĐẠT" : "ĐẠT", failures);
    return failures ? 1 : 0;
}

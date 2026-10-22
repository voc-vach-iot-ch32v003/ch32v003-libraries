#include "ch32fun.h"
#include "ch32v003_debug.h"
#include "ch32v003_delay.h"
#include "ch32v003_gpio.h"

// Máy đo dòng nhỏ bằng thời gian nạp tụ (A4M6 trên bàn test). Dòng GND của chip cần đo nạp vào tụ 104 ở nút N.
// OPA tích hợp chạy hở vòng làm bộ so sánh: OPP0 (PA2) = nút N, OPN0 (PA1) = ngưỡng từ cầu phân áp, ngõ ra OPO (PD4).
// Mỗi lần đo: tắt 8050 xả tụ (PC3) -> đếm SysTick (48MHz) tới khi OPO lên HIGH -> bật lại 8050.
//   I = Q / t, với Q = C * V_ngưỡng (pC). Kết quả in qua SWIO mỗi ~0.5 giây.

#define DISCHARGE_PIN 3 // PC3 (chân 3 A4M6) -> trở 1kΩ -> base 8050 (C ở nút N, E ở GND)
#define OPA_OUT_PIN 4 // PD4 (chân 8 A4M6) = OPO

#ifndef CAP_CHARGE_PC
#define CAP_CHARGE_PC 16890ULL // Hiệu chuẩn bằng trở mẫu 197.8kΩ từ 3.349V (16.55µA): tụ 102.9nF, ngưỡng hiệu dụng ~164mV
#endif

#define TICKS_PER_US 48U
#define TIMEOUT_TICKS (50000U * TICKS_PER_US) // 50ms: dòng < ~0.3µA coi như quá nhỏ
#define FAST_TICKS (10U * TICKS_PER_US) // < 10µs: dòng > ~1.5mA, ngoài dải (tụ 104)
#define WINDOW_MS 500U

static inline uint8_t comparatorHigh(void)
{
    return (GPIOD->INDR >> OPA_OUT_PIN) & 1;
}

static inline void discharge(const uint8_t on)
{
    if (on) GPIOC->BSHR = 1U << DISCHARGE_PIN;
    else GPIOC->BCR = 1U << DISCHARGE_PIN;
}

// Một lần nạp: trả về số tick SysTick từ lúc nhả tụ tới lúc vượt ngưỡng (TIMEOUT_TICKS nếu không tới)
static uint32_t chargeOnce(void)
{
    discharge(1);
    delayUs(30); // Xả hết tụ, OPO về LOW
    const uint32_t start = SysTick->CNT;
    discharge(0);
    uint32_t elapsed;
    do
    {
        elapsed = SysTick->CNT - start;
    } while (!comparatorHigh() && elapsed < TIMEOUT_TICKS);
    discharge(1); // Nối tắt tụ lại ngay: chip được đo không bị nâng GND lâu
    return elapsed;
}

static void setup(void)
{
    // Ngõ vào OPA ở chế độ analog, ngõ ra PD4 đọc như input số
    pinModePort(GPIOA, 1, ANALOG);
    pinModePort(GPIOA, 2, ANALOG);
    pinModePort(GPIOD, OPA_OUT_PIN, INPUT);
    pinModePort(GPIOC, DISCHARGE_PIN, OUTPUT);
    discharge(1);

    // OPA: NSEL = 0 (OPN0 = PA1), PSEL = 0 (OPP0 = PA2), bật
    EXTEN->EXTEN_CTR &= ~(EXTEN_OPA_NSEL | EXTEN_OPA_PSEL);
    EXTEN->EXTEN_CTR |= EXTEN_OPA_EN;
    delayMs(1);

    printf("CAPMETER READY Q=%lu pC\n", (unsigned long)CAP_CHARGE_PC);
}

static void loop(void)
{
    uint64_t sumTicks = 0;
    uint32_t count = 0, timeouts = 0, fast = 0;
    uint8_t stuckHigh = 0;

    const uint32_t windowStart = SysTick->CNT;
    while (SysTick->CNT - windowStart < WINDOW_MS * 1000U * TICKS_PER_US)
    {
        discharge(1);
        delayUs(30);
        if (comparatorHigh())
        {
            stuckHigh = 1; // Tụ đã xả mà OPO vẫn HIGH: sai dây ngưỡng / nút N
            break;
        }
        const uint32_t ticks = chargeOnce();
        if (ticks >= TIMEOUT_TICKS)
        {
            timeouts++;
            continue;
        }
        if (ticks < FAST_TICKS) fast++;
        sumTicks += ticks;
        count++;
    }

    if (stuckHigh)
    {
        printf("CAPMETER ERR OPO luon HIGH (kiem tra nguong PA1 / nut N PA2)\n");
        delayMs(500);
        return;
    }
    if (count == 0)
    {
        printf("CAPMETER I<0.3uA timeout=%lu\n", (unsigned long)timeouts);
        return;
    }

    // t trung bình (ns) và I = Q / t (nA)
    const uint64_t avgTicks = sumTicks / count;
    const uint32_t tNs = (uint32_t)(avgTicks * 1000ULL / TICKS_PER_US);
    const uint32_t iNa = (uint32_t)(CAP_CHARGE_PC * 1000000ULL / (tNs ? tNs : 1));
    printf("CAPMETER I=%lu.%02lu uA t=%lu ns n=%lu fast=%lu timeout=%lu\n", (unsigned long)(iNa / 1000),
           (unsigned long)(iNa % 1000 / 10), (unsigned long)tNs, (unsigned long)count, (unsigned long)fast,
           (unsigned long)timeouts);
}

int main(void)
{
    SystemInit();
    delayMs(1000); // Cửa sổ nạp

    setup();
    while (1)
    {
        loop();
    }
}

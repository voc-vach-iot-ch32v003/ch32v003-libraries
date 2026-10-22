#include "ch32fun.h"
#include "ch32v003_delay.h"
#include "ch32v003_gpio.h"
#include "ch32v003_power.h"
#include "hil.h"

// Test HIL cho ch32v003_power (cả 3 gói vỏ):
// - MARKER (PA1, CH10): HIGH ngay trước khi ngủ, LOW + 1 xung ngay sau khi thức -> ESP32 đo / đọc trạng thái.
// - WAKE_A (PA2, CH11) / WAKE_B (PC4, CH16): ESP32 tạo sườn để đánh thức.
// Kiểm tra: từ chối ngủ khi không có nguồn đánh thức, thức bằng chân (3 kiểu sườn, 2 chân), thức bằng hẹn giờ
// (độ dài + công thức AWU), chân cắt ngang hẹn giờ, RAM giữ nguyên + chạy tiếp (không reset), giữ chế độ SWIO.
// Standby thật hay không (không cần đo dòng): nhánh Standby của thư viện KHÔNG tắt PLL, nên nếu sau khi thức
// cờ PLLRDYF bật (PLL phải khóa lại) thì chính phần cứng đã tắt PLL = chip đã vào chế độ tắt clock thật.
// Với Sleep, thư viện tự tắt/bật PLL -> PLLRDYF = 1 là đối chứng cho thấy cơ chế cờ hoạt động.
// (HSIRDYF không bật khi phần cứng tự khởi động lại HSI nên không dùng làm bằng chứng, chỉ in ra để tham khảo.)

#define MARKER_PIN ((MCU_PORT_INDEX_A << 4) | 1)
#define WAKE_A_PIN ((MCU_PORT_INDEX_A << 4) | 2)
#define WAKE_B_PIN ((MCU_PORT_INDEX_C << 4) | 4)

typedef uint8_t (*PowerEnter_t)(void);

static char text[48];
static volatile uint32_t ramMagic = 0;

static void markerPulse(void)
{
    digitalWrite(MARKER_PIN, HIGH);
    digitalWrite(MARKER_PIN, LOW);
}

// Ngủ một lần: MARKER lên trước khi ngủ, xuống + xung sau khi thức. Trả về giá trị trả về của hàm ngủ
static uint8_t sleepOnce(const PowerEnter_t enter, const char* mode, const char* what)
{
    ramMagic = 0xC0FFEE00;
    RCC->INTR = RCC_HSIRDYIE | RCC_PLLRDYIE | RCC_HSIRDYC | RCC_PLLRDYC; // Xóa cờ sẵn sàng (IE bật để cờ được ghi; NVIC không bật)
    digitalWrite(MARKER_PIN, HIGH);
    const uint8_t slept = enter();
    digitalWrite(MARKER_PIN, LOW);
    markerPulse();
    const uint32_t flags = RCC->INTR;
    const uint8_t hsiRestarted = (flags & RCC_HSIRDYF) != 0;
    const uint8_t pllRelocked = (flags & RCC_PLLRDYF) != 0;
    RCC->INTR = RCC_HSIRDYC | RCC_PLLRDYC;
    delayMs(50); // SWD vừa bật lại: chờ mạch nạp đồng bộ SWIO rồi mới in, tránh mất dòng kết quả

    mini_snprintf(text, sizeof(text), "ram_kept_%s_%s", what, mode);
    hilCheck(text, ramMagic == 0xC0FFEE00, "");

    const uint8_t deep = mode[1] == 't'; // "standby"
    mini_snprintf(text, sizeof(text), "%s_%s_%s", deep ? "pll_off_by_hw" : "pll_flag_ctrl", what, mode);
    char detail[24];
    mini_snprintf(detail, sizeof(detail), "PLLRDYF=%u HSIRDYF=%u", pllRelocked, hsiRestarted);
    hilCheck(text, pllRelocked, detail);
    return slept;
}

static void testPin(const PowerEnter_t enter, const char* mode, const char* what,
                    const POWER_WakeupPin_t* pins, const uint8_t count, const uint32_t timeoutMs)
{
    const POWER_WakeupConfiguration_t config = {pins, count, timeoutMs};
    powerConfigureWakeup(&config);
    mini_snprintf(text, sizeof(text), "pin %s %s", what, mode);
    hilWait(text);
    sleepOnce(enter, mode, what);
}

static void testTimer(const PowerEnter_t enter, const char* mode, const uint32_t timeoutMs)
{
    const POWER_WakeupConfiguration_t config = {NULL, 0, timeoutMs};
    powerConfigureWakeup(&config);
    mini_snprintf(text, sizeof(text), "timer %lu %s", timeoutMs, mode);
    hilWait(text);
    sleepOnce(enter, mode, "timer");
    mini_snprintf(text, sizeof(text), "timer_done %lu %s", timeoutMs, mode);
    hilWait(text);
}

static void testMode(const PowerEnter_t enter, const char* mode)
{
    static const POWER_WakeupPin_t falling[] = {{WAKE_A_PIN, POWER_WAKEUP_FALLING}};
    static const POWER_WakeupPin_t rising[] = {{WAKE_A_PIN, POWER_WAKEUP_RISING}};
    static const POWER_WakeupPin_t change[] = {{WAKE_A_PIN, POWER_WAKEUP_CHANGE}};
    static const POWER_WakeupPin_t two[] = {{WAKE_A_PIN, POWER_WAKEUP_FALLING}, {WAKE_B_PIN, POWER_WAKEUP_RISING}};

    // 1. Không có nguồn đánh thức: phải từ chối ngủ
    powerConfigureWakeup(NULL);
    mini_snprintf(text, sizeof(text), "refuse_%s", mode);
    hilCheck(text, enter() == 0, "");

    // 2. Thức bằng chân
    testPin(enter, mode, "falling", falling, 1, 0);
    testPin(enter, mode, "rising", rising, 1, 0);
    testPin(enter, mode, "change_up", change, 1, 0);
    testPin(enter, mode, "change_down", change, 1, 0);
    testPin(enter, mode, "second_pin", two, 2, 0);

    // 3. Hẹn giờ: 2000ms (cửa sổ AWU 63) và 5200ms (cửa sổ 10) -> kiểm tra cả độ dài lẫn công thức
    testTimer(enter, mode, 2000);
    testTimer(enter, mode, 5200);

    // 4. Chân cắt ngang hẹn giờ dài
    testPin(enter, mode, "interrupt_timer", falling, 1, 20000);
}

int main(void)
{
    SystemInit();
    delayMs(1000); // Cửa sổ nạp

    pinMode(MARKER_PIN, OUTPUT);
    digitalWrite(MARKER_PIN, LOW);

    hilBegin("power");
    testMode(powerSleep, "sleep");
    testMode(powerStandby, "standby");
    testTimer(powerStandby, "standby", 32000); // > 30.7s: 1 lần AWU dài nhất + 1 lần phần lẻ

    // 5. SWIO đang ở chế độ GPIO trước khi ngủ thì sau khi thức vẫn phải là GPIO (không còn printf được):
    //    báo kết quả bằng mức chân MARKER, rồi trả SWIO về chế độ nạp để runner đọc tiếp
    static const POWER_WakeupConfiguration_t shortTimer = {NULL, 0, 50};
    powerConfigureWakeup(&shortTimer);
    hilWait("swio_gpio");
    setSwioMode(SWIO_MODE_GPIO);
    const uint8_t readBack = getSwioMode() == SWIO_MODE_GPIO;
    powerStandby();
    const uint8_t kept = getSwioMode() == SWIO_MODE_GPIO;
    digitalWrite(MARKER_PIN, readBack && kept ? HIGH : LOW);
    delayMs(300); // runner đọc MARKER trong khoảng này
    setSwioMode(SWIO_MODE_DEBUG);
    digitalWrite(MARKER_PIN, LOW);
    delayMs(200); // Mạch nạp cần chút thời gian đồng bộ lại SWIO, in ngay sẽ dính rác vào dòng kết quả
    mini_snprintf(text, sizeof(text), "doc_lai=%u giu=%u", readBack, kept);
    hilCheck("swio_gpio_kept", readBack && kept, text);
    hilEnd();
}

#include "ch32fun.h"
#include "ch32v003_debug.h"
#include "ch32v003_delay.h"
#include "ch32v003_gpio.h"
#include "ch32v003_power.h"

// Firmware đo tay dòng Standby (nạp bằng test/runner/standby_hold.py):
//   1. Sau cửa sổ nạp: mọi chân về ANALOG (tắt bộ đệm ngõ vào số). KHÔNG để Input thả nổi: net trôi ở mức
//      lưng chừng làm bộ đệm CMOS dẫn xuyên, đo thực tế ≥ 50µA thay vì 6.7µA. Chân đánh thức PA2 do thư viện
//      đặt Pull-up. Chip ngủ vĩnh viễn với SWD tắt: muốn nạp lại phải tắt/bật nguồn (RST qua ESP32, nên VDD
//      phải đi qua GPIO của ESP32), nếu không thì dùng rvprog -e + cấp nguồn tay.
//   2. Sau cửa sổ nạp 1 giây giữ mãi một trạng thái (chọn bằng HOLD_MODE lúc build):
//      0 = Standby, chỉ thức bằng chân PA2 (LSI tắt) | 1 = Sleep | 2 = chạy liên tục 48MHz
//      3 = Standby thức bằng hẹn giờ 30s rồi ngủ lại ngay (LSI + AWU chạy)
//   3. Kéo PA2 xuống GND (ESP32: OFF PA2) -> chip thức và chạy liên tục (đối chứng dòng lúc thức).

#define WAKE_PIN ((MCU_PORT_INDEX_A << 4) | 2) // PA2 (CH11)

#ifndef HOLD_MODE
#define HOLD_MODE 0
#endif

#if HOLD_MODE == 1
#define HOLD_ENTER powerSleep
#define HOLD_NAME "Sleep"
#elif HOLD_MODE == 2
#define HOLD_NAME "Run"
#elif HOLD_MODE == 3
#define HOLD_ENTER powerStandby
#define HOLD_NAME "Standby (hen gio 30s)"
#else
#define HOLD_ENTER powerStandby
#define HOLD_NAME "Standby"
#endif

static void setPort(GPIO_TypeDef* GPIOx, const GPIO_PinMode_t mode)
{
    for (uint8_t pinNumber = 0; pinNumber < 8; pinNumber++)
    {
        if (GPIOx == GPIOD && pinNumber == 1) continue; // PD1 = SWIO: ở ANALOG làm mất printf qua SWIO
        pinModePort(GPIOx, pinNumber, mode);
    }
}

int main(void)
{
    SystemInit();
    setPort(GPIOA, INPUT);
    setPort(GPIOC, INPUT);
    setPort(GPIOD, INPUT);
    delayMs(1000); // Cửa sổ nạp

    setPort(GPIOA, ANALOG);
    setPort(GPIOC, ANALOG);
    setPort(GPIOD, ANALOG);

    printf("HIL:STANDBY_HOLD giu trang thai " HOLD_NAME "\n");
    delayMs(50); // Cho dòng printf truyền xong trước khi SWD tắt

#if HOLD_MODE == 2
    // ReSharper disable once CppDFAEndlessLoop
    while (1)
    {
    }
#elif HOLD_MODE == 3
    static const POWER_WakeupConfiguration_t config = {NULL, 0, 30000};
    powerConfigureWakeup(&config);
    // ReSharper disable once CppDFAEndlessLoop
    while (1)
    {
        HOLD_ENTER();
    }
#else
    static const POWER_WakeupPin_t wakePins[] = {{WAKE_PIN, POWER_WAKEUP_FALLING}};
    static const POWER_WakeupConfiguration_t config = {wakePins, 1, 0};
    powerConfigureWakeup(&config);
    HOLD_ENTER();
#endif

    delayMs(50);
    printf("HIL:AWAKE chip dang chay lien tuc\n");
    // ReSharper disable once CppDFAEndlessLoop
    while (1)
    {
    }
}

#include "ch32fun.h"
#include "ch32v003_delay.h"
#include "ch32v003_gpio.h"
#include "ch32v003_power.h"
#include "hil.h"

// Test HIL dòng tiêu thụ (máy đo Uno ở chân GND của chip, test/uno_meter/): mọi chân (trừ PD1) về ANALOG, rồi
// lần lượt chạy liên tục / Standby / Sleep, mỗi trạng thái giữ HOLD_MS để runner đo.
// Standby đo 2 lần: thức bằng hẹn giờ (LSI + AWU chạy suốt lúc ngủ) và thức bằng chân PA2 (LSI tắt) -> biết
// chi phí của hẹn giờ. Thức bằng chân: runner đo xong thì ESP32 kéo PA2 xuống.

#define HOLD_MS 3000
#define WAKE_PIN ((MCU_PORT_INDEX_A << 4) | 2) // PA2 (CH11)

static void analogAllPins(void)
{
    for (uint8_t pin = 0; pin < 8; pin++)
    {
        pinModePort(GPIOA, pin, ANALOG);
        pinModePort(GPIOC, pin, ANALOG);
        if (pin != 1) pinModePort(GPIOD, pin, ANALOG); // Giữ PD1: PD1 ở ANALOG làm mất printf qua SWIO đang chạy
    }
}

int main(void)
{
    SystemInit();
    delayMs(1000); // Cửa sổ nạp

    hilBegin("current");
    analogAllPins();

    // 1. Chạy liên tục 48MHz (delayMs là vòng chờ bận)
    hilWait("measure run");
    delayMs(HOLD_MS);

    // 2. Standby thức bằng chân: LSI tắt (Pull-up nội trên PA2, ESP32 kéo xuống để đánh thức)
    static const POWER_WakeupPin_t wakePins[] = {{WAKE_PIN, POWER_WAKEUP_FALLING}};
    static const POWER_WakeupConfiguration_t pinConfig = {wakePins, 1, 0};
    powerConfigureWakeup(&pinConfig);
    hilWait("measure standby_pin");
    powerStandby();
    delayMs(50); // SWD vừa bật lại: chờ mạch nạp đồng bộ trước khi in
    pinModePort(GPIOA, 2, ANALOG);

    // 3. Standby / Sleep thức bằng hẹn giờ HOLD_MS (LSI + AWU chạy)
    static const POWER_WakeupConfiguration_t timerConfig = {NULL, 0, HOLD_MS};
    powerConfigureWakeup(&timerConfig);

    hilWait("measure standby");
    powerStandby();
    delayMs(50);

    hilWait("measure sleep");
    powerSleep();
    delayMs(50);

    hilEnd();
}

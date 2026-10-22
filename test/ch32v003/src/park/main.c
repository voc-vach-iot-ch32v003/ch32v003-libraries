#include "ch32fun.h"
#include "ch32v003_debug.h"
#include "ch32v003_delay.h"
#include "ch32v003_gpio.h"

// Firmware "đỗ" cho bàn test HIL: chip không được test chạy firmware này để
// không tải bus tín hiệu dùng chung và không trả lời trên dây SWIO dùng chung.
//   1. Mọi chân GPIO về Input thả nổi (Hi-Z).
//   2. Giữ SWD bật trong 1 giây sau khởi động: cửa sổ để runner nạp chip này.
//   3. Hết cửa sổ thì tắt SWD (PD1 thành GPIO Hi-Z) rồi ngủ vĩnh viễn.
// Muốn nạp lại chip đang đỗ: ESP32 tắt/bật nguồn chip -> chip quay lại bước 2.

#if defined(CH32V003_J4M6)
#define PARK_CHIP_NAME "J4M6"
#elif defined(CH32V003_A4M6)
#define PARK_CHIP_NAME "A4M6"
#else
#define PARK_CHIP_NAME "F4P6"
#endif

/**
 * @brief Đưa toàn bộ chân của Port về Input thả nổi (Hi-Z).
 */
static void parkPort(GPIO_TypeDef* GPIOx)
{
    for (uint8_t pinNumber = 0; pinNumber < 8; pinNumber++)
    {
        pinModePort(GPIOx, pinNumber, INPUT);
    }
}

static void setup(void)
{
    // Hi-Z ngay từ đầu (trước cả cửa sổ nạp) để chip không đụng tới bus dùng chung
    parkPort(GPIOA);
    parkPort(GPIOC);
    parkPort(GPIOD);
}

int main(void)
{
    SystemInit();
    setup();
    delayMs(1000); // Cửa sổ nạp: SWD còn bật, runner nạp chip này trong khoảng thời gian này

    printf("HIL:PARKED " PARK_CHIP_NAME "\n");

    // Tắt SWD: từ đây chip không trả lời WCH-LinkE cho tới lần khởi động lại kế tiếp
    setSwioMode(SWIO_MODE_GPIO);

    // Không còn việc gì: ngủ (WFI) để giảm dòng tiêu thụ từ chân GPIO cấp nguồn của ESP32
    // ReSharper disable once CppDFAEndlessLoop
    while (1)
    {
        __WFI();
    }
}

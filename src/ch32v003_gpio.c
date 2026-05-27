#include "ch32v003_gpio.h"

void pinModePort(GPIO_TypeDef* GPIOx, const uint8_t pinNumber, const PinMode_t mode)
{
    // 1. Tự động cấp xung clock cho Port tương ứng
    if (GPIOx == GPIOA)
    {
        RCC->APB2PCENR |= RCC_APB2Periph_GPIOA;
    }
    else if (GPIOx == GPIOC)
    {
        RCC->APB2PCENR |= RCC_APB2Periph_GPIOC;
    }
    else if (GPIOx == GPIOD)
    {
        RCC->APB2PCENR |= RCC_APB2Periph_GPIOD;
    }

    // Mỗi chân (0->7) quản lý bởi 4 bit trong thanh ghi cấu hình CFGLR
    const uint8_t bitOffset = pinNumber * 4;

    // 2. Xóa cấu hình cũ của chân mục tiêu
    GPIOx->CFGLR &= ~(0x0F << bitOffset);

    // 3. Lọc lấy 4 bit cấu hình phần cứng từ enum mode
    const uint8_t hardwareConfig = mode & 0x0F;
    GPIOx->CFGLR |= hardwareConfig << bitOffset;

    // 4. Bật điện trở kéo nếu chọn chế độ Pull-up hoặc Pull-down
    if (mode == INPUT_PULLUP)
    {
        GPIOx->BSHR = 1 << pinNumber; // Ghi 1 vào OUTDR để kích hoạt Pull-up
    }
    else if (mode == INPUT_PULLDOWN)
    {
        GPIOx->BCR = 1 << pinNumber; // Ghi 0 vào OUTDR để kích hoạt Pull-down
    }
}

void digitalWritePort(GPIO_TypeDef* GPIOx, const uint8_t pinNumber, const uint8_t state)
{
    if (state == HIGH)
    {
        GPIOx->BSHR = 1 << pinNumber; // Đẩy chân lên HIGH siêu tốc bằng thanh ghi BSHR
    }
    else
    {
        GPIOx->BCR = 1 << pinNumber; // Hạ chân xuống LOW siêu tốc bằng thanh ghi BCR
    }
}

void digitalTogglePort(GPIO_TypeDef* GPIOx, const uint8_t pinNumber)
{
    if (GPIOx->OUTDR & 1 << pinNumber)
    {
        GPIOx->BCR = 1 << pinNumber;
    }
    else
    {
        GPIOx->BSHR = 1 << pinNumber;
    }
}

uint8_t digitalReadPort(const GPIO_TypeDef* GPIOx, const uint8_t pinNumber)
{
    if ((GPIOx->INDR & 1 << pinNumber))
    {
        return HIGH;
    }
    return LOW;
}

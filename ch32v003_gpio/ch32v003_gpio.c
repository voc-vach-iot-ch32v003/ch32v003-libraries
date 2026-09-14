#include "ch32v003_gpio.h"

void pinModePort(GPIO_TypeDef* GPIOx, const uint8_t pinNumber, const PinMode_t mode)
{
    // 1. Tự động cấp xung clock cho Port tương ứng
    if (GPIOx == GPIOA)
    {
        RCC->APB2PCENR |= RCC_APB2Periph_GPIOA;
        if (pinNumber == 1 || pinNumber == 2)
        {
            RCC->APB2PCENR |= RCC_APB2Periph_AFIO; // Bật clock AFIO
            AFIO->PCFR1 &= ~(1 << 15); // Tắt thạch anh ngoài để nhường PA1/PA2 cho GPIO
        }
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

void decodeHardwarePin(const uint8_t mcuPin, GPIO_TypeDef** GPIOx, uint8_t* pinNumber)
{
    *pinNumber = mcuPin & 0x0F;
    const uint8_t portIdx = mcuPin >> 4;
    *GPIOx = (portIdx == 0) ? GPIOA : ((portIdx == 1) ? GPIOC : GPIOD);
}

void pinMode(const uint8_t mcuPin, const PinMode_t mode)
{
    EXECUTE_ON_PIN(mcuPin, pinModePort, mode);
}

void digitalWrite(const uint8_t mcuPin, const DigitalState_t state)
{
    EXECUTE_ON_PIN(mcuPin, digitalWritePort, state);
}

void digitalToggle(const uint8_t mcuPin)
{
    EXECUTE_ON_PIN(mcuPin, digitalTogglePort);
}

DigitalState_t digitalRead(const uint8_t mcuPin)
{
    return EXECUTE_ON_PIN_RET(mcuPin, digitalReadPort) == 0 ? LOW : HIGH;
}

// Xử lý quản lý cấu hình các thanh ghi remap cho chân SWIO
void setSwioMode(const SwioMode_t swioMode)
{
    RCC->APB2PCENR |= RCC_APB2Periph_AFIO; // Bật clock cho AFIO
    if (swioMode == SWIO_MODE_GPIO)
    {
        // Gán bit giải phóng chân nạp trong thanh ghi CFGR
        AFIO->PCFR1 |= (1 << 26);
    }
    else
    {
        AFIO->PCFR1 &= ~(1 << 26);
    }
}

SwioMode_t getSwioMode(void)
{
    return (AFIO->PCFR1 & (1 << 26)) ? SWIO_MODE_GPIO : SWIO_MODE_DEBUG;
}

void toggleSwioMode(void)
{
    setSwioMode(getSwioMode() == SWIO_MODE_DEBUG ? SWIO_MODE_GPIO : SWIO_MODE_DEBUG);
}

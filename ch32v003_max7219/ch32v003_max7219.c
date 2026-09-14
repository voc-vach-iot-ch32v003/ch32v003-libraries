#include "ch32v003_max7219.h"

void max7219Init(Max7219_t* dev, const MCU_Pin_t csPin, const uint8_t cascadeCount)
{
    dev->csPin = csPin;
    dev->cascadeCount = (cascadeCount == 0) ? 1 : cascadeCount;

    // 1. Chỉ khởi tạo chân CS (Không tự ý gọi spiInit làm ghi đè cấu hình Bus)
    spiInitCS(dev->csPin);

    // 2. Cấu hình ban đầu cho các thanh ghi hệ thống MAX7219
    max7219WriteAll(dev, MAX7219_REG_DISPLAY_TEST, 0x00);
    max7219WriteAll(dev, MAX7219_REG_DECODE_MODE, MAX7219_NO_DECODE);
    max7219WriteAll(dev, MAX7219_REG_SCAN_LIMIT, MAX7219_SCAN_ALL_DIGITS);
    max7219SetIntensity(dev, MAX7219_INTENSITY_DEFAULT);
    max7219SetPower(dev, true);
    max7219ClearAll(dev);
}

void max7219WriteReg(Max7219_t* dev, const uint8_t deviceIdx, const uint8_t regAddr, const uint8_t data)
{
    if (deviceIdx >= dev->cascadeCount) return;

    // Kéo CS xuống LOW để bắt đầu phiên truyền
    spiSelect(dev->csPin);

    // Truyền dữ liệu qua Daisy Chain: IC ở xa nhất nhận dữ liệu trước
    for (int i = (int)dev->cascadeCount - 1; i >= 0; i--)
    {
        if (i == (int)deviceIdx)
        {
            spiTransfer(regAddr);
            spiTransfer(data);
        }
        else
        {
            spiTransfer(MAX7219_REG_NOOP);
            spiTransfer(0x00);
        }
    }

    spiWaitIdle();
    // Kéo CS lên HIGH để chốt dữ liệu
    spiDeselect(dev->csPin);
}

void max7219WriteAll(Max7219_t* dev, const uint8_t regAddr, const uint8_t data)
{
    spiSelect(dev->csPin);

    for (uint8_t i = 0; i < dev->cascadeCount; i++)
    {
        spiTransfer(regAddr);
        spiTransfer(data);
    }

    spiWaitIdle();
    spiDeselect(dev->csPin);
}

void max7219SetIntensity(Max7219_t* dev, uint8_t intensity)
{
    if (intensity > MAX7219_INTENSITY_MAX)
    {
        intensity = MAX7219_INTENSITY_MAX;
    }
    max7219WriteAll(dev, MAX7219_REG_INTENSITY, intensity);
}

void max7219SetPower(Max7219_t* dev, const bool powerOn)
{
    max7219WriteAll(dev, MAX7219_REG_SHUTDOWN, powerOn ? MAX7219_NORMAL_MODE : MAX7219_SHUTDOWN_MODE);
}

void max7219ClearAll(Max7219_t* dev)
{
    for (uint8_t digitReg = MAX7219_REG_DIGIT0; digitReg <= MAX7219_REG_DIGIT7; digitReg++)
    {
        max7219WriteAll(dev, digitReg, 0x00);
    }
}

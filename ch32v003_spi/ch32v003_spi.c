#include "ch32v003_spi.h"

typedef enum
{
    SPI_MODE_HARDWARE,
    SPI_MODE_SOFTWARE
} SPI_DriverMode_t;

static SPI_DriverMode_t currentSpiMode = SPI_MODE_SOFTWARE;
static SPI_SoftwareConfig_t softSpiConfig;

#if defined(CH32V003_F4P6)
void spiInitHardware(SPI_HardwareRemap_t remap)
{
    currentSpiMode = SPI_MODE_HARDWARE;

    RCC->APB2PCENR |= RCC_APB2Periph_SPI1 | RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOC;

    if (remap == NSS_PC0)
    {
        AFIO->PCFR1 |= (1 << 0);
    }
    else
    {
        AFIO->PCFR1 &= ~(1 << 0);
    }

    // PC5 (SCK) & PC6 (MOSI) -> Alternate Push-Pull
    GPIOC->CFGLR &= ~((0xF << (4 * 5)) | (0xF << (4 * 6)));
    GPIOC->CFGLR |= ((0x9 << (4 * 5)) | (0x9 << (4 * 6)));

    SPI1->CTLR1 = SPI_CTLR1_MSTR | SPI_CTLR1_SSM | SPI_CTLR1_SSI | SPI_CTLR1_BR_0;
    SPI1->CTLR1 |= SPI_CTLR1_SPE;
}
#endif

void spiInitSoftware(const SPI_SoftwareConfig_t config)
{
    currentSpiMode = SPI_MODE_SOFTWARE;
    softSpiConfig = config;

    // Cấu hình chân SCK và MOSI theo kiểu MCU_Pin_t
    pinMode(softSpiConfig.sckPin, OUTPUT);
    pinMode(softSpiConfig.mosiPin, OUTPUT);

    digitalWrite(softSpiConfig.sckPin, LOW);
    digitalWrite(softSpiConfig.mosiPin, LOW);

    if (softSpiConfig.misoPin != MCU_PIN_NONE)
    {
        pinMode(softSpiConfig.misoPin, INPUT);
    }
}

void spiInitCS(const MCU_Pin_t csPin)
{
    pinMode(csPin, OUTPUT);
    spiDeselect(csPin);
}

void spiSelect(const MCU_Pin_t csPin)
{
    digitalWrite(csPin, LOW);
}

void spiDeselect(const MCU_Pin_t csPin)
{
    digitalWrite(csPin, HIGH);
}

uint8_t spiTransfer(uint8_t data)
{
#if defined(CH32V003_F4P6)
    if (currentSpiMode == SPI_MODE_HARDWARE)
    {
        while (!(SPI1->STATR & SPI_STATR_TXE));
        SPI1->DATAR = data;
        while (!(SPI1->STATR & SPI_STATR_RXNE));
        return SPI1->DATAR;
    }
#endif

    // Software SPI Bit-banging (MSB First, SPI Mode 0)
    uint8_t rxData = 0;
    for (uint8_t i = 0; i < 8; i++)
    {
        if (data & 0x80)
        {
            digitalWrite(softSpiConfig.mosiPin, HIGH);
        }
        else
        {
            digitalWrite(softSpiConfig.mosiPin, LOW);
        }
        data <<= 1;

        digitalWrite(softSpiConfig.sckPin, HIGH);

        rxData <<= 1;
        if (softSpiConfig.misoPin != MCU_PIN_NONE)
        {
            if (digitalRead(softSpiConfig.misoPin))
            {
                rxData |= 0x01;
            }
        }

        digitalWrite(softSpiConfig.sckPin, LOW);
    }
    return rxData;
}

void spiWaitIdle(void)
{
#if defined(CH32V003_F4P6)
    if (currentSpiMode == SPI_MODE_HARDWARE)
    {
        while (SPI1->STATR & SPI_STATR_BSY);
    }
#endif
}

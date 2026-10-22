#include "ch32fun.h"
#include "ch32v003_delay.h"
#include "ch32v003_gpio.h"
#include "hil.h"

// Test HIL cho ch32v003_gpio: với mỗi chân có mặt trên bàn test, kiểm tra
// xuất HIGH/LOW/Toggle (ESP32 đo), đọc input (ESP32 kéo), Pull-up / Pull-down nội.

/**
 * @brief Một chân trên bàn test: mã MCU_PIN (Port << 4 | Pin) + tên net (khớp ESP32).
 */
typedef struct
{
    uint8_t mcuPin;
    const char* net;
    uint8_t externalPull; /**< Trở kéo ngoài trên net: 0 = không, PULL_UP (I2C 4.7kΩ), PULL_DOWN (nút C0 của TWEN) */
} TestPin_t;

#define PULL_UP 1
#define PULL_DOWN 2

#define PIN_A(n) ((MCU_PORT_INDEX_A << 4) | (n))
#define PIN_C(n) ((MCU_PORT_INDEX_C << 4) | (n))
#define PIN_D(n) ((MCU_PORT_INDEX_D << 4) | (n))

static const TestPin_t testPins[] = {
    {PIN_A(1), "PA1", 0},
    {PIN_A(2), "PA2", 0},
    {PIN_C(1), "PC1", PULL_UP},
    {PIN_C(2), "PC2", PULL_UP},
    {PIN_C(4), "PC4", 0},
#if defined(CH32V003_A4M6) || defined(CH32V003_F4P6)
    {PIN_C(0), "PC0", PULL_DOWN},
    {PIN_C(3), "PC3", 0},
    {PIN_C(6), "PC6", 0},
    {PIN_C(7), "PC7", 0},
    {PIN_D(4), "PD4", 0},
    {PIN_D(5), "PD5", 0},
    {PIN_D(6), "PD6", 0},
#endif
#if defined(CH32V003_F4P6)
    {PIN_C(5), "PC5", 0},
    {PIN_D(0), "PD0", 0},
    {PIN_D(2), "PD2", 0},
    {PIN_D(3), "PD3", 0},
#endif
};

static char text[48];

static void waitStep(const char* step, const char* net, const int value)
{
    mini_snprintf(text, sizeof(text), "%s %s %d", step, net, value);
    hilWait(text);
}

static void checkRead(const TestPin_t* pin, const char* what, const GPIO_DigitalState_t expected)
{
    const GPIO_DigitalState_t got = digitalRead(pin->mcuPin);
    mini_snprintf(text, sizeof(text), "%s_%s", what, pin->net);
    hilCheck(text, got == expected, got == HIGH ? "doc=1" : "doc=0");
}

static void testPin(const TestPin_t* pin)
{
    // 1. Xuất đẩy kéo: HIGH, LOW, Toggle -> ESP32 đo
    pinMode(pin->mcuPin, OUTPUT);
    digitalWrite(pin->mcuPin, HIGH);
    waitStep("expect", pin->net, 1);
    digitalWrite(pin->mcuPin, LOW);
    waitStep("expect", pin->net, 0);
    digitalToggle(pin->mcuPin);
    waitStep("expect", pin->net, 1);

    // 2. Input thả nổi: ESP32 kéo lên / xuống, chip tự đọc
    pinMode(pin->mcuPin, INPUT);
    waitStep("drive", pin->net, 1);
    checkRead(pin, "input_high", HIGH);
    waitStep("drive", pin->net, 0);
    checkRead(pin, "input_low", LOW);
    waitStep("release", pin->net, 0);

    // 3. Điện trở kéo nội: ESP32 thả nổi, chỉ đọc (bỏ qua chiều bị trở kéo ngoài lấn át)
    if (pin->externalPull != PULL_DOWN)
    {
        pinMode(pin->mcuPin, INPUT_PULLUP);
        waitStep("expect_pull", pin->net, 1);
    }
    if (pin->externalPull != PULL_UP)
    {
        pinMode(pin->mcuPin, INPUT_PULLDOWN);
        waitStep("expect_pull", pin->net, 0);
    }

    pinMode(pin->mcuPin, INPUT); // Trả chân về Hi-Z trước khi sang chân kế tiếp
}

int main(void)
{
    SystemInit();
    delayMs(1000); // Cửa sổ nạp

    hilBegin("gpio");
    for (uint8_t i = 0; i < sizeof(testPins) / sizeof(testPins[0]); i++)
    {
        testPin(&testPins[i]);
    }
    hilCheck("swio_mode_debug", getSwioMode() == SWIO_MODE_DEBUG, "");
    hilEnd();
}

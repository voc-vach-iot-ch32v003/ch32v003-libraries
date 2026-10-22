#include "bench_i2c.h"
#include <Wire.h>

static const uint8_t SDA_GPIO = 32; // CH13 PC1
static const uint8_t SCL_GPIO = 18; // CH14 PC2

static bool active = false;
static portMUX_TYPE rxLock = portMUX_INITIALIZER_UNLOCKED;
static uint8_t rxBuffer[BENCH_I2C_RX_MAX];
static uint16_t rxLength = 0;
static uint16_t rxTransactions = 0;

// Chạy trong task I2C slave của Arduino: gom byte master ghi
static void onReceive(int count)
{
    uint8_t local[BENCH_I2C_RX_MAX];
    uint16_t n = 0;
    while (Wire.available() && n < sizeof(local)) local[n++] = Wire.read();

    portENTER_CRITICAL(&rxLock);
    for (uint16_t i = 0; i < n && rxLength < BENCH_I2C_RX_MAX; i++) rxBuffer[rxLength++] = local[i];
    rxTransactions++;
    portEXIT_CRITICAL(&rxLock);
}

bool benchI2cStart(const uint8_t address, const uint32_t frequency)
{
    benchI2cStop();
    rxLength = 0;
    rxTransactions = 0;
    Wire.onReceive(onReceive);
    active = Wire.begin(address, SDA_GPIO, SCL_GPIO, frequency);
    return active;
}

void benchI2cStop()
{
    if (active)
    {
        Wire.end();
        active = false;
    }
    pinMode(SDA_GPIO, INPUT);
    pinMode(SCL_GPIO, INPUT);
}

bool benchI2cActive()
{
    return active;
}

size_t benchI2cLoad(const uint8_t* data, const size_t length)
{
    return active ? Wire.slaveWrite(data, length) : 0;
}

uint16_t benchI2cTakeReceived(uint8_t* buffer, const uint16_t maxLength, uint16_t& transactions)
{
    portENTER_CRITICAL(&rxLock);
    const uint16_t n = rxLength < maxLength ? rxLength : maxLength;
    memcpy(buffer, rxBuffer, n);
    transactions = rxTransactions;
    rxLength = 0;
    rxTransactions = 0;
    portEXIT_CRITICAL(&rxLock);
    return n;
}

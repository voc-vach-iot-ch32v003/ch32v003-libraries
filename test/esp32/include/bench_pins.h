/**
 * @file bench_pins.h
 * @author Vọc Vạch IoT
 * @brief Sơ đồ chân ESP32 của bàn test HIL. Phải khớp bảng CH trong test/wiring/gen.py.
 */

#ifndef BENCH_PINS_H
#define BENCH_PINS_H

#include <Arduino.h>

/**
 * @brief Thông tin một chip CH32V003 trên bàn test.
 */
struct BenchChip
{
    const char* name; /**< Tên gói vỏ (F4P6 / A4M6 / J4M6) */
    uint8_t powerPin; /**< Chân ESP32 cấp nguồn trực tiếp vào VDD chip (HIGH = có điện) */
};

/** @brief 3 chip trên bàn test, chân cấp nguồn theo bản vẽ. */
static const BenchChip BENCH_CHIPS[] = {
    {"F4P6", 2},
    {"A4M6", 22},
    {"J4M6", 23},
};

static const uint8_t BENCH_CHIP_COUNT = sizeof(BENCH_CHIPS) / sizeof(BENCH_CHIPS[0]);

/** @brief Thời gian tắt nguồn mặc định khi khởi động lại chip (đủ để tụ 100nF xả hết). */
static const uint16_t BENCH_RESET_OFF_MS = 50;

/**
 * @brief Một kênh tín hiệu: chân ESP32 nối (qua trở 1kΩ) tới net chân port của CH32.
 */
struct BenchChannel
{
    uint8_t gpio; /**< Chân ESP32 */
    const char* net; /**< Tên net = tên chân port của CH32 (PD4, PC1...) */
};

/** @brief 16 kênh, chỉ số mảng + 1 = số kênh (CH1..CH16). */
static const BenchChannel BENCH_CHANNELS[] = {
    {13, "PD4"}, // CH1
    {12, "PD3"}, // CH2
    {15, "PD2"}, // CH3
    {4, "PD0"}, // CH4
    {16, "PC7"}, // CH5  SPI MISO
    {17, "PC6"}, // CH6  SPI MOSI
    {5, "PC5"}, // CH7  SPI SCK
    {27, "PD6"}, // CH8  UART: CH32 RX
    {14, "PD5"}, // CH9  UART: CH32 TX
    {26, "PA1"}, // CH10 DAC2 -> ADC A1
    {25, "PA2"}, // CH11 DAC1 -> ADC A0
    {33, "PC0"}, // CH12 SPI CS
    {32, "PC1"}, // CH13 I2C SDA
    {18, "PC2"}, // CH14 I2C SCL
    {19, "PC3"}, // CH15
    {21, "PC4"}, // CH16
};

static const uint8_t BENCH_CHANNEL_COUNT = sizeof(BENCH_CHANNELS) / sizeof(BENCH_CHANNELS[0]);

#endif // BENCH_PINS_H

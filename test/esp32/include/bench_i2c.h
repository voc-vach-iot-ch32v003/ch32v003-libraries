/**
 * @file bench_i2c.h
 * @author Vọc Vạch IoT
 * @brief ESP32 đóng vai thiết bị I2C slave trên kênh CH13 (PC1 = SDA) / CH14 (PC2 = SCL) để kiểm thử thư viện I2C master.
 *
 * ESP32 bản gốc không kéo giãn (stretch) được SCL: dữ liệu trả lời phải nạp sẵn vào FIFO (LOAD)
 * TRƯỚC khi master đọc. Byte master ghi được gom lại, đọc bằng benchI2cTakeReceived().
 */

#ifndef BENCH_I2C_H
#define BENCH_I2C_H

#include <Arduino.h>

/** @brief Số byte tối đa gom được giữa 2 lần đọc (RX). */
static const uint16_t BENCH_I2C_RX_MAX = 128;

/**
 * @brief Bật I2C slave tại địa chỉ 7-bit (SDA = GPIO32, SCL = GPIO18).
 * @return false nếu ESP32 không khởi tạo được.
 */
bool benchI2cStart(uint8_t address, uint32_t frequency);

/**
 * @brief Tắt I2C slave, trả 2 chân về Input thả nổi. Gọi được cả khi chưa bật.
 */
void benchI2cStop();

/**
 * @brief I2C slave đang bật?
 */
bool benchI2cActive();

/**
 * @brief Nạp sẵn dữ liệu cho lần master đọc kế tiếp (tối đa 32 byte = FIFO phần cứng).
 * @return Số byte đã nạp.
 */
size_t benchI2cLoad(const uint8_t* data, size_t length);

/**
 * @brief Lấy (và xóa) các byte master đã ghi từ lần gọi trước.
 * @param transactions Trả về số giao dịch ghi đã nhận.
 * @return Số byte chép vào buffer.
 */
uint16_t benchI2cTakeReceived(uint8_t* buffer, uint16_t maxLength, uint16_t& transactions);

#endif // BENCH_I2C_H

/**
 * @file bench_spi.h
 * @author Vọc Vạch IoT
 * @brief ESP32 đóng vai thiết bị SPI slave (Mode 0) trên 4 kênh bất kỳ để kiểm thử thư viện SPI master.
 *
 * Mỗi giao dịch (một lần CS xuống rồi lên) phải được "nạp" trước bằng benchSpiLoad():
 * dữ liệu nạp được đẩy ra MISO, byte master gửi được lưu lại, đọc bằng benchSpiTakeReceived().
 */

#ifndef BENCH_SPI_H
#define BENCH_SPI_H

#include <Arduino.h>

/** @brief Số byte tối đa một giao dịch (không dùng DMA). */
static const uint16_t BENCH_SPI_MAX_BYTES = 64;

/**
 * @brief Bật SPI slave Mode 0 trên các chân ESP32 cho trước.
 * @return false nếu ESP-IDF báo lỗi.
 */
bool benchSpiStart(uint8_t sckGpio, uint8_t mosiGpio, uint8_t misoGpio, uint8_t csGpio);

/**
 * @brief Tắt SPI slave, trả 4 chân về Input thả nổi. Gọi được cả khi chưa bật.
 */
void benchSpiStop();

/**
 * @brief SPI slave đang bật?
 */
bool benchSpiActive();

/**
 * @brief Nạp một giao dịch: dữ liệu đẩy ra MISO (phần còn lại là 0x00).
 * @details Giao dịch nạp trước đó mà master chưa thực hiện sẽ bị hủy.
 * @return false nếu chưa bật.
 */
bool benchSpiLoad(const uint8_t* data, size_t length);

/**
 * @brief Lấy kết quả giao dịch đã nạp.
 * @param timeoutMs Chờ tối đa (giao dịch chưa xong thì trả -1).
 * @return Số byte master đã gửi (bit lẻ bị bỏ), -1 nếu chưa xong / chưa nạp.
 */
int benchSpiTakeReceived(uint8_t* buffer, uint16_t maxLength, uint32_t timeoutMs, uint32_t& bits);

#endif // BENCH_SPI_H

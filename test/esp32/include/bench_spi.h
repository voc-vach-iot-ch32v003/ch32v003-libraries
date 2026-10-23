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

/** @brief Giới hạn của benchSpiSniff(): tổng số byte và số khung CS. */
static const uint16_t BENCH_SNIFF_MAX_BYTES = 512;
static const uint8_t BENCH_SNIFF_MAX_FRAMES = 64;

/**
 * @brief Nghe lén SPI Mode 0 (thụ động, chỉ đọc chân): ghi lại byte MOSI của từng khung CS (CS xuống -> lên).
 * @details Thăm dò thanh ghi GPIO khi khóa ngắt (vòng lặp trong IRAM). Không cần nạp trước như SPI slave nên
 *          bắt được nhiều khung liền nhau. Cả 3 chân < GPIO32 thì mỗi lượt chỉ đọc 1 thanh ghi (nhanh nhất).
 *          Dừng khi CS nghỉ HIGH quá idleUs.
 * @param frameBits Nhận số bit của từng khung; byte các khung nối tiếp nhau trong bytes.
 * @param timeoutMs Chờ CS xuống lần đầu tối đa (giới hạn 100ms: chờ cũng khóa ngắt để không lỡ bit đầu).
 * @param maxGapNs Nhận khoảng hở lớn nhất giữa 2 lần lấy mẫu sau khi CS xuống (ns). Lớn hơn nửa chu kỳ SCK
 *                 nghĩa là có thể đã lỡ sườn (core 1 bị core 0 dừng tạm qua ngắt mức cao, không khóa được).
 * @return Số khung đã bắt.
 */
uint8_t benchSpiSniff(uint8_t sckGpio, uint8_t mosiGpio, uint8_t csGpio, uint32_t timeoutMs, uint32_t idleUs,
                      uint8_t* bytes, uint16_t* frameBits, uint32_t& maxGapNs);

#endif // BENCH_SPI_H

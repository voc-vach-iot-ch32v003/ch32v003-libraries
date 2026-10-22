/**
 * @file bench_ir.h
 * @author Vọc Vạch IoT
 * @brief ESP32 giả lập mắt thu hồng ngoại (phát khung đã giải điều chế) và bắt tín hiệu có sóng mang để kiểm thử thư viện IR.
 *
 * Thời gian chuẩn của từng giao thức đặt độc lập ở đây (theo đặc tả), không lấy từ thư viện cần kiểm thử.
 */

#ifndef BENCH_IR_H
#define BENCH_IR_H

#include <Arduino.h>

/**
 * @brief Phát khung IR đã giải điều chế như ngõ ra mắt thu (rảnh HIGH, có sóng mang = LOW).
 * @param protocol NEC | SAMSUNG | SONY | TCL.
 * @param data Các byte dữ liệu, bit 0 của byte 0 phát trước; số bit theo giao thức (NEC 32, SONY 12, TCL 76).
 * @param frames Số khung: NEC thêm mã lặp (108ms/khung), SAMSUNG / SONY lặp lại nguyên khung (108ms / 45ms).
 * @return false nếu giao thức không hỗ trợ hoặc thiếu dữ liệu.
 */
bool benchIrSend(int channel, const String& protocol, const uint8_t* data, size_t length, uint8_t frames);

/**
 * @brief Bắt tín hiệu IR có sóng mang (rảnh LOW): đường bao Mark/Space và tần số, duty của sóng mang.
 * @param durations Nhận độ dài Mark, Space, Mark... (us), bắt đầu bằng Mark.
 * @param timeoutMs Chờ sườn lên đầu tiên tối đa; sau đó dừng khi LOW liên tục quá gapUs.
 * @return Số đoạn đã ghi (0 nếu không có tín hiệu).
 */
uint16_t benchIrCapture(int channel, uint32_t* durations, uint16_t maxCount, uint32_t timeoutMs, uint32_t gapUs,
                        uint32_t& carrierHz, uint32_t& dutyPercent);

#endif // BENCH_IR_H

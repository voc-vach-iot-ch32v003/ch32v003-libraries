/**
 * @file bench_ws2812.h
 * @author Vọc Vạch IoT
 * @brief ESP32 bắt khung dữ liệu WS2812 bằng RMT (phân giải 12.5ns) để kiểm thử thư viện ws2812.
 *
 * Ngưỡng phân loại bit và thống kê độ rộng xung đặt độc lập ở đây theo datasheet WS2812B, không lấy từ thư viện.
 */

#ifndef BENCH_WS2812_H
#define BENCH_WS2812_H

#include <Arduino.h>

/** @brief Số bit tối đa bắt được trong 1 lần (bộ nhớ RMT 8 khối × 64 phần tử, mỗi bit 1 phần tử). */
static const uint16_t BENCH_WS_MAX_BITS = 500;

/** @brief Kết quả 1 lần bắt: các khung (ngăn bởi LOW > 50us) và thống kê thời gian (ns). */
typedef struct
{
    uint8_t frameCount; /**< Số khung */
    uint16_t frameBits[4]; /**< Số bit của từng khung (tối đa 4 khung) */
    uint8_t data[BENCH_WS_MAX_BITS / 8 + 1]; /**< Byte các khung nối liền, bit đầu tiên là MSB */
    uint32_t high0Min, high0Max; /**< Độ rộng HIGH của bit 0 */
    uint32_t high1Min, high1Max; /**< Độ rộng HIGH của bit 1 */
    uint32_t lowMin, lowMax; /**< Độ rộng LOW giữa 2 bit trong cùng khung */
    uint32_t gapMin; /**< LOW ngắn nhất giữa 2 khung (0 nếu chỉ có 1 khung) */
} BenchWsCapture_t;

/**
 * @brief Bắt tín hiệu WS2812 trên 1 kênh: chờ sườn đầu tiên tối đa timeoutMs, dừng khi LOW quá idleUs.
 * @param idleUs Tối đa 400 (giới hạn bộ đếm RMT); muốn bắt 2 khung liền nhau thì đặt lớn hơn thời gian chốt.
 * @return false nếu hết giờ không có tín hiệu.
 */
bool benchWsCapture(int channel, uint32_t timeoutMs, uint32_t idleUs, BenchWsCapture_t& result);

#endif // BENCH_WS2812_H

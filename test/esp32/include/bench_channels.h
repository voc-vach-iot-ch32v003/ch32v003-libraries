/**
 * @file bench_channels.h
 * @author Vọc Vạch IoT
 * @brief Thao tác 16 kênh tín hiệu của bàn test: chế độ chân, đọc/ghi, ADC/DAC, bắt sườn, đo tần số.
 */

#ifndef BENCH_CHANNELS_H
#define BENCH_CHANNELS_H

#include <Arduino.h>

/** @brief Số sườn tối đa một lần bắt (CAP). */
static const uint16_t BENCH_CAPTURE_MAX_EDGES = 512;

/**
 * @brief Tìm kênh theo số (3), tên kênh (CH3) hoặc tên net (PD2).
 * @return Chỉ số kênh 0..15, hoặc -1 nếu không hợp lệ.
 */
int benchFindChannel(const String& token);

/**
 * @brief Đưa toàn bộ 16 kênh về Input thả nổi (Hi-Z), tắt DAC.
 */
void benchReleaseAll();

/**
 * @brief Đặt chế độ một kênh.
 * @param mode IN (thả nổi) | PU (kéo lên) | PD (kéo xuống) | OUT (đẩy kéo) | OD (cực máng hở).
 * @return true nếu chế độ hợp lệ.
 */
bool benchSetMode(int channel, const String& mode);

/**
 * @brief Ghi mức logic ra kênh (tự chuyển sang OUT nếu kênh đang là input).
 */
void benchWrite(int channel, int level);

/**
 * @brief Đọc mức logic hiện tại của kênh.
 */
int benchRead(int channel);

/**
 * @brief Đọc điện áp kênh (mV) bằng ADC của ESP32.
 */
uint32_t benchReadMilliVolts(int channel);

/**
 * @brief Xuất điện áp DAC 8-bit (chỉ kênh nối GPIO25/26).
 * @return false nếu kênh không có DAC.
 */
bool benchWriteDac(int channel, uint8_t value);

/**
 * @brief Bắt các sườn tín hiệu trên kênh (thăm dò bận, độ phân giải ~0.1us).
 * @param startLevel Trả về mức logic lúc bắt đầu.
 * @param durations Mảng nhận độ dài từng đoạn mức logic liên tiếp (us, hoặc ns nếu nanoseconds = true).
 * @param timeoutMs Chờ sườn đầu tiên tối đa timeoutMs; sau đó dừng khi im lặng quá idleUs.
 * @return Số đoạn đã ghi.
 */
uint16_t benchCapture(int channel, int& startLevel, uint32_t* durations, uint16_t maxCount,
                      uint32_t timeoutMs, uint32_t idleUs, bool nanoseconds = false);

/**
 * @brief Đếm sườn lên trên kênh trong windowMs, quy đổi ra tần số (Hz).
 */
uint32_t benchMeasureFrequency(int channel, uint32_t windowMs);

/**
 * @brief Đo khoảng thời gian giữa 2 sườn lên liên tiếp bằng ngắt (không khóa CPU, đo được khoảng dài).
 * @param timeoutMs Thời gian chờ tối đa cho cả 2 sườn.
 * @return Khoảng thời gian (us), hoặc -1 nếu hết thời gian chờ.
 */
int64_t benchMeasureInterval(int channel, uint32_t timeoutMs);

#endif // BENCH_CHANNELS_H

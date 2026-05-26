/**
* @file ch32v003_timer.h
 * @author Vọc Vạch IoT
 * @brief Thư viện quản lý thời gian hệ thống không chặn (Non-blocking) cho CH32V003.
 * @version 1.0
 * @date 2026-05-26
 */

#ifndef CH32V003_TIMER_H
#define CH32V003_TIMER_H

#include "ch32fun.h"
#include "sys_config.h"

#if (TIMER_ARDUINO_STYLE_ENABLE)

// ============================================================================
// BỘ HÀM QUẢN LÝ THỜI GIAN HỆ THỐNG (NON-BLOCKING TIMING)
// ============================================================================

/**
 * @brief Trả về số ticks thô từ bộ đếm SysTick phần cứng của lõi RISC-V.
 * @details Hàm này bọc lại bộ đếm SysTick 32-bit phần cứng của lõi RISC-V [ch32fun].
 * @return Số ticks thô từ bộ đếm SysTick phần cứng của lõi RISC-V [ch32fun].
 */
uint32_t ticks(void);

/**
 * @brief Trả về số micro-giây (us) đã trôi qua kể từ khi vi điều khiển khởi động.
 * @return Số micro-giây (us) tính từ lúc khởi động.
 */
uint32_t micros(void);

/**
 * @brief Trả về số mili-giây (ms) đã trôi qua kể từ khi vi điều khiển khởi động.
 * @return Số mili-giây (ms) tính từ lúc khởi động.
 */
uint32_t millis(void);

/**
 * @brief Tính toán khoảng thời gian (us) thực tế đã trôi qua giữa hai mốc thời gian micros.
 * @details Tự động xử lý chính xác hiện tượng tràn số (Rollover) của thanh ghi đếm 32-bit.
 * @param current Mốc thời gian hiện tại (thường lấy từ hàm micros()).
 * @param start Mốc thời gian bắt đầu được lưu lại trước đó.
 * @return Khoảng thời gian chênh lệch tính bằng micro-giây (us).
 */
uint32_t elapsedUs(uint32_t current, uint32_t start);

/**
 * @brief Tính toán khoảng thời gian (ms) thực tế đã trôi qua giữa hai mốc thời gian millis.
 * @details Tự động xử lý chính xác hiện tượng tràn số (Rollover) của thanh ghi đếm 32-bit.
 * @param current Mốc thời gian hiện tại (thường lấy từ hàm millis()).
 * @param start Mốc thời gian bắt đầu được lưu lại trước đó.
 * @return Khoảng thời gian chênh lệch tính bằng mili-giây (ms).
 */
uint32_t elapsedMs(uint32_t current, uint32_t start);

#endif // TIMER_ARDUINO_STYLE_ENABLE

#endif // CH32V003_TIMER_H

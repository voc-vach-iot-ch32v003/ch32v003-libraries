/**
* @file ch32v003_delay.h
 * @author Vọc Vạch IoT
 * @brief Thư viện bọc các hàm tạo độ trễ (Delay) theo phong cách Arduino cho CH32V003.
 * @version 1.0
 * @date 2026-05-26
 */

#ifndef CH32V003_DELAY_H
#define CH32V003_DELAY_H

#include "ch32fun.h"
#include "sys_config.h"

#if (DELAY_ARDUINO_STYLE_ENABLE)

// ============================================================================
// ĐỒNG BỘ HÀM DELAY KIỂU ARDUINO (WRAPPERS)
// ============================================================================

/**
 * @brief Tạo độ trễ theo đơn vị mili-giây (ms).
 * @details Sử dụng bộ đếm định thời hệ thống SysTick chạy ở tần số tối đa 48MHz để đếm thời gian.
 *          Hàm này sẽ block luồng xử lý cho đến khi hết thời gian chờ.
 * @param ms Thời gian chờ mong muốn tính bằng mili-giây.
 */
void delayMs(uint32_t ms);

/**
 * @brief Tạo độ trễ cực ngắn theo đơn vị micro-giây (us).
 * @details Thích hợp cho các ứng dụng giao tiếp nhịp dòng lệnh (như One-Wire, DHT11, DS18B24).
 * @param us Thời gian chờ mong muốn tính bằng micro-giây.
 */
void delayUs(uint32_t us);

#endif // DELAY_ARDUINO_STYLE_ENABLE

#endif // CH32V003_DELAY_H

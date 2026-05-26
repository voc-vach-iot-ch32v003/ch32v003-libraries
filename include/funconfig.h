/**
* @file funconfig.h
 * @author Vọc Vạch IoT
 * @brief Tệp cấu hình phần cứng gốc cho framework ch32v003fun.
 * @version 1.0
 * @date 2026-05-26
 */

#ifndef FUNCONFIG_H
#define FUNCONFIG_H

/**
 * @brief Cấu hình nguồn cấp xung nhịp cho bộ định thời hệ thống (SysTick).
 * @details Giá trị 1 nghĩa là cho phép SysTick sử dụng trực tiếp tần số của lõi HCLK (48MHz).
 *          Giúp các hàm Delay tạo ra độ trễ chính xác tuyệt đối.
 */
#define FUNCONF_SYSTICK_USE_HCLK 1

/**
 * @brief Kích hoạt tính năng in dữ liệu gỡ lỗi (Debug Printf).
 * @details Giá trị 1 cho phép sử dụng hàm printf() để truyền dữ liệu ngược lên máy tính
 *          thông qua ngay chính chân nạp Single Wire (SWIO) mà không tốn thêm chân UART.
 */
#define FUNCONF_USE_DEBUGPRINTF  1

#endif // FUNCONFIG_H

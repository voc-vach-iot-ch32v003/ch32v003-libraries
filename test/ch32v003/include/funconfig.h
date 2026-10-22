/**
 * @file funconfig.h
 * @author Vọc Vạch IoT
 * @brief Tệp cấu hình phần cứng gốc cho framework ch32v003fun.
 */

#ifndef FUNCONFIG_H
#define FUNCONFIG_H

/**
 * @brief Cấu hình nguồn cấp xung nhịp cho bộ định thời hệ thống (SysTick).
 */
#define FUNCONF_SYSTICK_USE_HCLK 1

/**
 * @brief Kích hoạt tính năng in dữ liệu gỡ lỗi (Debug Printf qua SWIO).
 */
#define FUNCONF_USE_DEBUGPRINTF  1

#endif // FUNCONFIG_H

/**
 * @file sys_config.h
 * @author Vọc Vạch IoT
 * @brief Tệp cấu hình hệ thống, quản lý kích hoạt các tính năng và tối ưu bộ nhớ Flash cho CH32V003.
 * @version 1.1
 * @date 2026-05-26
 */

#ifndef SYS_CONFIG_H
#define SYS_CONFIG_H

/**
 * @brief Định nghĩa cờ kích hoạt cấu hình hệ thống.
 * @note Macro này xác nhận tệp sys_config đã được nạp thành công vào dự án.
 */
#define SYS_CONFIG_ACTIVE 1

#define DELAY_ARDUINO_STYLE_ENABLE 1 /**< Kích hoạt các hàm Delay theo phong cách Arduino (delay, delayMicroseconds) */

/** @brief Bật (1) hoặc Tắt (0) thư viện quản lý thời gian hệ thống (micros, millis) */
#define TIMER_ARDUINO_STYLE_ENABLE       1

#define IR_ENABLE                       1
#define IR_SUPPORT_NEC                  1
#define IR_SUPPORT_SONY                 1
#define IR_SUPPORT_SAMSUNG              1

#endif

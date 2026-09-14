/**
* @file ch32v003_ir_defs.h
 * @author Vọc Vạch IoT
 * @brief Định nghĩa các macro cấu hình hệ thống, ngưỡng thời gian và tham số biên dịch cho thư viện IR.
 * @version 2.0.0
 * @date 2026-06-07
 */
#ifndef CH32V003_IR_DEFS_H
#define CH32V003_IR_DEFS_H

#include "ch32fun.h"
#if defined(__has_include)
#if __has_include("sys_config.h")
#include "sys_config.h"
#endif
#endif

#if (IR_ENABLE)

/** @brief Cấu hình chung hệ thống */
#ifndef IR_TIMEOUT_US
#define IR_TIMEOUT_US 15000 /**< Thời gian chờ tối đa chặn treo chip (15ms) */
#endif

#ifndef IR_TOLERANCE_PERCENT
#define IR_TOLERANCE_PERCENT 25 /**< Dung sai cho phép khi so khớp thời gian xung (25%) */
#endif

#ifndef IR_RAW_BUFFER_SIZE
#define IR_RAW_BUFFER_SIZE 256 /**< Số lượng sườn xung (Edges) tối đa có thể bắt được trong một khung truyền. */
#endif

#ifndef IR_MAX_SUPPORTED_BITS
#define IR_MAX_SUPPORTED_BITS 80 /**< Số lượng BIT dữ liệu tối đa mà thư viện có thể giải mã và quản lý. */
#endif

#define IR_RAW_DATA_BYTES ((IR_MAX_SUPPORTED_BITS + 7) / 8) /**< Tự động tính toán số BYTE cần thiết trong struct IR_Data_t (Chia tròn lên). */



#endif // IR_ENABLE

#endif // CH32V003_IR_DEFS_H

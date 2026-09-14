/**
* @file ch32v003_power_types.h
 * @author Vọc Vạch IoT
 * @brief Định nghĩa các enum và kiểu dữ liệu liên quan đến quản lý năng lượng cho CH32V003.
 * @version 2.0
 * @date 2026-06-07
 */

#ifndef CH32V003_POWER_TYPES_H
#define CH32V003_POWER_TYPES_H

/**
 * @brief Kiểu enum định nghĩa các kiểu kích hoạt đánh thức chip từ chế độ ngủ sâu.
 * @details Các giá trị có thể được sử dụng để chỉ định cách mà chân Wakeup sẽ
 * kích hoạt chip tỉnh dậy: cạnh xuống (FALLING), cạnh lên (RISING), hoặc cả hai cạnh (CHANGE).
 */
typedef enum
{
    WAKEUP_FALLING = 0, /* Kích hoạt khi cạnh xuống: Khi chân Wakeup chuyển từ HIGH sang LOW */
    WAKEUP_RISING = 1, /* Kích hoạt khi cạnh lên: Khi chân Wakeup chuyển từ LOW sang HIGH */
    WAKEUP_CHANGE = 2
    /* Kích hoạt khi có thay đổi trạng thái: Khi chân Wakeup thay đổi từ HIGH sang LOW hoặc từ LOW sang HIGH */
} WakeupTrigger_t;

#endif

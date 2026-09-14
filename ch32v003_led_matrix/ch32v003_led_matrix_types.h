/**
* @file ch32v003_led_matrix_types.h
 * @author Vọc Vạch IoT
 * @brief Định nghĩa kiểu dữ liệu và Struct cho LED Matrix 8x8.
 * @version 1.0.1
 * @date 2026-09-14
 */

#ifndef CH32V003_LED_MATRIX_TYPES_H
#define CH32V003_LED_MATRIX_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "ch32v003_max7219.h"

/**
 * @brief Chế độ xoay màn hình Matrix.
 */
typedef enum
{
    MATRIX_ROTATE_0 = 0,
    MATRIX_ROTATE_90,
    MATRIX_ROTATE_180,
    MATRIX_ROTATE_270
} MatrixRotation_t;

/**
 * @brief Hướng cuộn chữ/hiệu ứng trên màn hình Matrix.
 */
typedef enum
{
    MATRIX_SCROLL_LEFT = 0, /**< Cuộn nội dung sang bên trái (Chữ chạy từ Phải qua Trái) */
    MATRIX_SCROLL_RIGHT, /**< Cuộn nội dung sang bên phải (Chữ chạy từ Trái qua Phải) */
    MATRIX_SCROLL_UP, /**< Cuộn nội dung lên phía trên */
    MATRIX_SCROLL_DOWN /**< Cuộn nội dung xuống phía dưới */
} MatrixScrollDirection_t;

/**
 * @brief Struct quản lý đối tượng LED Matrix.
 */
typedef struct
{
    Max7219_t maxDev; /**< Con trỏ đối tượng phần cứng MAX7219 */
    uint8_t* frameBuffer; /**< Bộ đệm đếm nét màn hình (Mỗi IC tốn 8 byte) */
    uint8_t matrixCount; /**< Số lượng module 8x8 ghép nối tiếp */
    MatrixRotation_t rotation; /**< Góc xoay toàn hệ thống */
    MatrixScrollDirection_t scrollDirection; /**< Hướng dịch chuyển/cuộn mặc định */
} LedMatrix_t;

#endif // CH32V003_LED_MATRIX_TYPES_H

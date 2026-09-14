/**
* @file ch32v003_max7219_types.h
 * @author Vọc Vạch IoT
 * @brief Định nghĩa các kiểu dữ liệu và Struct cho thư viện MAX7219.
 * @version 1.0.0
 * @date 2026-09-13
 */

#ifndef CH32V003_MAX7219_TYPES_H
#define CH32V003_MAX7219_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "ch32v003_gpio.h"

/**
 * @brief Struct quản lý đối tượng điều khiển chuỗi MAX7219.
 */
typedef struct
{
    MCU_Pin_t csPin; /**< Chân Chip Select (CS/LOAD) */
    uint8_t cascadeCount; /**< Số lượng IC MAX7219 mắc nối tiếp */
} Max7219_t;

#endif // CH32V003_MAX7219_TYPES_H

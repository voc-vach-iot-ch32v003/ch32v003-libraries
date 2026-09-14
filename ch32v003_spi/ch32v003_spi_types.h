/**
* @file ch32v003_spi_types.h
 * @author Vọc Vạch IoT
 * @brief Định nghĩa enum và kiểu dữ liệu cấu hình SPI chuẩn theo MCU_Pin_t cho CH32V003.
 * @version 1.0.0
 * @date 2026-09-14
 */

#ifndef CH32V003_SPI_TYPES_H
#define CH32V003_SPI_TYPES_H

#include "ch32v003_gpio.h"

#define MCU_PIN_NONE    0xFF    /**< Định nghĩa chân không sử dụng */

/**
 * @brief Chế độ Remap Hardware SPI1 (Dành riêng cho TSSOP20 / F4P6).
 */
typedef enum
{
    NONE = 0, /**< SCK=PC5, MOSI=PC6, MISO=PC7, NSS=PC1 (Default) */
    NSS_PC0 = 1 /**< SCK=PC5, MOSI=PC6, MISO=PC7, NSS=PC0 */
} SPI_HardwareRemap_t;

/**
 * @brief Struct cấu hình Software SPI Bit-banging sử dụng chân vật lý MCU_Pin_t.
 */
typedef struct
{
    MCU_Pin_t sckPin; /**< Chân Clock vật lý (Ví dụ: SOP8 dùng MCU_PIN5) */
    MCU_Pin_t mosiPin; /**< Chân MOSI vật lý  (Ví dụ: SOP8 dùng MCU_PIN6) */
    MCU_Pin_t misoPin; /**< Chân MISO vật lý  (Nếu không dùng gán MCU_PIN_NONE) */
} SPI_SoftwareConfig_t;

#endif // CH32V003_SPI_TYPES_H

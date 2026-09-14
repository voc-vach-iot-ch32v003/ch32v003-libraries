/**
* @file ch32v003_spi.h
 * @author Vọc Vạch IoT
 * @brief Thư viện SPI hợp nhất hỗ trợ chân vật lý MCU_Pin_t cho CH32V003.
 * @version 1.0.0
 * @date 2026-09-14
 */

#ifndef CH32V003_SPI_H
#define CH32V003_SPI_H

#include "ch32fun.h"
#include "ch32v003_gpio.h"
#include "ch32v003_spi_types.h"

// ============================================================================
// HÀM KHỞI TẠO BUS SPI
// ============================================================================

#if defined(CH32V003_F4P6)
/**
 * @brief Khởi tạo Hardware SPI1 Master (Chỉ dùng cho TSSOP20 / F4P6).
 */
void spiInitHardware(SPI_HardwareRemap_t remap);
#endif

/**
 * @brief Khởi tạo Software SPI Bit-banging với chân vật lý MCU_Pin_t.
 */
void spiInitSoftware(SPI_SoftwareConfig_t config);

// ============================================================================
// HÀM QUẢN LÝ CHÂN CHÍP SELECT (CS / NSS SOFTWARE)
// ============================================================================

/**
 * @brief Khởi tạo chân GPIO làm CS bằng chân vật lý MCU_Pin_t.
 */
void spiInitCS(MCU_Pin_t csPin);

/**
 * @brief Kéo CS xuống LOW.
 */
void spiSelect(MCU_Pin_t csPin);

/**
 * @brief Kéo CS lên HIGH.
 */
void spiDeselect(MCU_Pin_t csPin);

// ============================================================================
// HÀM TRUYỀN NHẬN DỮ LIỆU
// ============================================================================

/**
 * @brief Truyền nhận 1 Byte qua SPI.
 */
uint8_t spiTransfer(uint8_t data);

/**
 * @brief Chờ bus SPI rảnh.
 */
void spiWaitIdle(void);

#endif // CH32V003_SPI_H

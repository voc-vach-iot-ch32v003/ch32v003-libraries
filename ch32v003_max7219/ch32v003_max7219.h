/**
 * @file ch32v003_max7219.h
 * @author Vọc Vạch IoT
 * @brief Thư viện điều khiển IC MAX7219 truyền dữ liệu qua SPI cho CH32V003.
 * @version 1.0.0
 * @date 2026-09-14
 */

#ifndef CH32V003_MAX7219_H
#define CH32V003_MAX7219_H

#include "ch32fun.h"
#include "ch32v003_gpio.h"
#include "ch32v003_spi.h"
#include "ch32v003_max7219_defs.h"
#include "ch32v003_max7219_types.h"

#if defined(__has_include)
#if __has_include("sys_config.h")
#include "sys_config.h"
#endif
#endif

// ============================================================================
// KHAI BÁO CÁC HÀM ĐIỀU KHIỂN (CAMELCASE)
// ============================================================================

/**
 * @brief Khởi tạo đối tượng MAX7219 và thiết lập các thanh ghi ban đầu.
 * @note Bus SPI phải được khởi tạo trước bằng spiInitHardware() hoặc spiInitSoftware().
 * @param dev Con trỏ tới struct Max7219_t.
 * @param csPin Chân CS kiểu MCU_Pin_t.
 * @param cascadeCount Số lượng module nối tiếp.
 */
void max7219Init(Max7219_t* dev, MCU_Pin_t csPin, uint8_t cascadeCount);

/**
 * @brief Ghi dữ liệu tới một thanh ghi cụ thể trên 1 IC trong chuỗi.
 * @param dev Con trỏ tới đối tượng max7219_t.
 * @param deviceIdx Chỉ số IC trong chuỗi (0 là IC gần vi điều khiển nhất).
 * @param regAddr Địa chỉ thanh ghi (MAX7219_REG_*).
 * @param data Dữ liệu 8-bit cần ghi.
 */
void max7219WriteReg(Max7219_t* dev, uint8_t deviceIdx, uint8_t regAddr, uint8_t data);

/**
 * @brief Ghi cùng một thanh ghi và dữ liệu tới TẤT CẢ các IC trong chuỗi.
 * @param dev Con trỏ tới đối tượng max7219_t.
 * @param regAddr Địa chỉ thanh ghi (MAX7219_REG_*).
 * @param data Dữ liệu 8-bit cần ghi.
 */
void max7219WriteAll(Max7219_t* dev, uint8_t regAddr, uint8_t data);

/**
 * @brief Cài đặt độ sáng hiển thị cho toàn bộ chuỗi IC.
 * @param dev Con trỏ tới đối tượng max7219_t.
 * @param intensity Mức độ sáng (từ 0 đến 15).
 */
void max7219SetIntensity(Max7219_t* dev, uint8_t intensity);

/**
 * @brief Bật hoặc tắt nguồn hiển thị (Shutdown Mode).
 * @param dev Con trỏ tới đối tượng max7219_t.
 * @param powerOn true: Hoạt động bình thường, false: Vào chế độ tiết kiệm điện.
 */
void max7219SetPower(Max7219_t* dev, bool powerOn);

/**
 * @brief Xóa toàn bộ dữ liệu hiển thị trên tất cả IC (Ghi 0x00 vào 8 Digit).
 * @param dev Con trỏ tới đối tượng max7219_t.
 */
void max7219ClearAll(Max7219_t * dev);

#endif // CH32V003_MAX7219_H

/**
 * @file ch32v003_ir.h
 * @author Vọc Vạch IoT
 * @brief Thư viện thu phát hồng ngoại đa giao thức (NEC, Sony, Samsung) cho CH32V003.
 * @version 2.0
 * @date 2026-06-07
 */

#ifndef CH32V003_IR_H
#define CH32V003_IR_H

#include "ch32v003_ir_defs.h"
#include "ch32v003_ir_types.h"

#if (IR_ENABLE)

// ============================================================================
// CÁC HÀM ĐƠN ĐIỀU KHIỂN THEO CHÂN MCU (API KIỂU ARDUINO)
// Hỗ trợ hiển thị tài liệu tối đa khi hover chuột
// ============================================================================

/**
 * @brief Quét và lưu trữ dữ liệu xung thô từ tín hiệu hồng ngoại vào bộ đệm (Hàm chặn).
 * @param mcu_pin Chân vật lý MCU làm đầu vào thu hồng ngoại, chọn từ @ref MCUPin_t hoặc @ref MCUSpecialPin_t.
 * @param rawData Con trỏ tới cấu trúc @ref IR_RawData_t để lưu kết quả độ rộng xung thô.
 * @return 1 nếu đã thu thập được dữ liệu thô thành công, 0 nếu không có tín hiệu hoặc bộ đệm đầy.
 */
uint8_t irReadRaw(uint8_t mcu_pin, IR_RawData_t* rawData);

/**
 * @brief Đọc và giải mã dữ liệu hồng ngoại trực tiếp từ chân vật lý chỉ định (Hàm chặn).
 * @param mcu_pin Chân vật lý MCU làm đầu vào thu hồng ngoại, chọn từ @ref MCUPin_t hoặc @ref MCUSpecialPin_t.
 * @param irData Con trỏ tới cấu trúc @ref IR_Data_t để lưu thông tin sau giải mã.
 * @return 1 nếu nhận dạng và giải mã thành công, 0 nếu không có tín hiệu hoặc nhiễu.
 */
uint8_t irRead(uint8_t mcu_pin, IR_Data_t* irData);

/**
 * @brief Phát tín hiệu hồng ngoại theo cấu trúc xung thô đã thu thập (Học lệnh) ra chân vật lý (Hàm chặn).
 * @param mcu_pin Chân vật lý MCU kết nối mạch đệm LED phát IR, chọn từ @ref MCUPin_t hoặc @ref MCUSpecialPin_t.
 * @param rawData Con trỏ tới cấu trúc @ref IR_RawData_t chứa dữ liệu học lệnh thô.
 */
void irSendRaw(uint8_t mcu_pin, const IR_RawData_t* rawData);

/**
 * @brief Phát tín hiệu điều khiển hồng ngoại theo giao thức chuẩn hóa ra chân vật lý MCU.
 * @param mcu_pin Chân vật lý MCU kết nối mạch đệm LED phát IR, chọn từ @ref MCUPin_t hoặc @ref MCUSpecialPin_t.
 * @param protocol Tên giao thức cần phát chọn từ danh mục @ref IR_Protocol_t.
 * @param address Địa chỉ mã hóa của thiết bị cần điều khiển.
 * @param command Mã phím bấm / Lệnh chức năng cần truyền đi.
 */
void irSend(uint8_t mcu_pin, IR_Protocol_t protocol, uint16_t address, uint32_t command);

// ============================================================================
// HÀM ĐỘC LẬP THEO PORT THÔ GỐC (DÙNG ĐỂ CHẠY HẬU TRƯỜNG FILE .C)
// ============================================================================

/**
 * @brief Quét và giải mã tín hiệu hồng ngoại dựa theo cấu trúc Port và Số pin chỉ định (Hàm chặn).
 * @param GPIOx Con trỏ quản lý vùng nhớ Port (GPIOA, GPIOC, GPIOD).
 * @param pinNumber Số thứ tự bit của chân trong Port (Giá trị từ 0 đến 7).
 * @param irData Con trỏ tới cấu trúc lưu kết quả giải mã thành công.
 * @return 1 nếu giải mã thành công một giao thức, 0 nếu không thành công.
 */
uint8_t irReadPort(const GPIO_TypeDef* GPIOx, uint8_t pinNumber, IR_Data_t* irData);

/**
 * @brief Quét và lưu trữ dữ liệu xung thô từ tín hiệu hồng ngoại vào bộ đệm, không giải mã (Hàm chặn).
 * @param GPIOx Con trỏ quản lý vùng nhớ Port (GPIOA, GPIOC, GPIOD).
 * @param pinNumber Số thứ tự bit của chân trong Port (Giá trị từ 0 đến 7).
 * @param rawData Con trỏ tới cấu trúc lưu mảng độ rộng xung và số lượng xung đã lưu.
 * @return 1 nếu đã thu thập được dữ liệu thô thành công, 0 nếu không có tín hiệu hoặc bộ đệm đầy.
 */
uint8_t irReadPortRaw(const GPIO_TypeDef* GPIOx, uint8_t pinNumber, IR_RawData_t* rawData);

/**
 * @brief Phát sóng mang hồng ngoại theo đúng cấu trúc giao thức ra chân Port chỉ định.
 * @param GPIOx Con trỏ quản lý vùng nhớ Port (GPIOA, GPIOC, GPIOD).
 * @param pinNumber Số thứ tự bit của chân trong Port (Giá trị từ 0 đến 7).
 * @param protocol Giao thức muốn phát (IR_NEC, IR_SONY, IR_SAMSUNG).
 * @param address Địa chỉ thiết bị cần gửi.
 * @param command Mã lệnh thực thi cần gửi đi.
 */
void irSendPort(GPIO_TypeDef* GPIOx, uint8_t pinNumber, IR_Protocol_t protocol, uint16_t address, uint32_t command);

/**
 * @brief Phát tín hiệu hồng ngoại theo cấu trúc xung thô đã thu thập được, không cần giải mã (Hàm chặn).
 * @param GPIOx Con trỏ quản lý vùng nhớ Port (GPIOA, GPIOC, GPIOD).
 * @param pinNumber Số thứ tự bit của chân trong Port (Giá trị từ 0 đến 7).
 * @param rawData Con trỏ tới cấu trúc chứa dữ liệu xung thô đã thu thập được từ hàm irReadPortRaw.
 */
void irSendPortRaw(GPIO_TypeDef* GPIOx, uint8_t pinNumber, const IR_RawData_t* rawData);

/**
 * @brief Phân tích mảng dữ liệu xung thô và in thông số chuỗi thời gian phục vụ debug/tối ưu.
 * @param rawData Con trỏ tới cấu trúc dữ liệu xung thô cần phân tích.
 */
void irAnalyzeRaw(const IR_RawData_t* rawData);

/**
 * @brief In kết quả gói tin IR đã được giải mã ra cổng truyền thông (UART/Printf Debug).
 * @param irData Con trỏ tới cấu trúc dữ liệu gói tin IR đã giải mã.
 */
void irPrintResult(const IR_Data_t* irData);

#endif // IR_ENABLE

#endif // CH32V003_IR_H

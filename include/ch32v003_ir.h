/**
 * @file ch32v003_ir.h
 * @author Vọc Vạch IoT
 * @brief Thư viện thu phát hồng ngoại đa giao thức (NEC, Sony, Samsung) cho CH32V003.
 * @version 1.0
 * @date 2026-05-26
 */

#ifndef CH32V003_IR_H
#define CH32V003_IR_H

#include "ch32fun.h"
#include "sys_config.h"

#if (IR_ENABLE)

/**
 * @brief Định nghĩa danh sách các giao thức hồng ngoại được hỗ trợ.
 */
typedef enum
{
    IR_UNKNOWN = 0,
    IR_NEC = 1,
    IR_SONY = 2,
    IR_SAMSUNG = 3
} IR_Protocol_t;

/**
 * @brief Cấu trúc dữ liệu chứa thông tin gói tin IR giải mã được.
 */
typedef struct
{
    IR_Protocol_t protocol; /**< Loại giao thức giải mã được */
    uint16_t address; /**< Địa chỉ thiết bị (Address) */
    uint32_t command; /**< Lệnh thực thi (Command / Data) */
    uint8_t bits; /**< Số lượng bit của gói tin */
} IR_Data_t;

typedef struct
{
    uint16_t rawBuf[IR_RAW_BUFFER_SIZE];
    uint8_t rawLen;
} IR_RawData_t;


// ============================================================================
// CÁC HÀM CHỨC NĂNG CHÍNH (THU VÀ PHÁT)
// ============================================================================

/**
 * @brief Đọc dữ liệu xung thô từ tín hiệu hồng ngoại mà không giải mã, lưu vào buffer để phân tích sau (Hàm chặn).
 * @param mcu_pin Số hiệu chân vật lý cấu hình từ user_config.h (Ví dụ: MCU_PIN1).
 * @param irRawData Con trỏ tới cấu trúc lưu trữ kết quả thô của tín hiệu, bao gồm mảng độ rộng xung và số lượng xung đã lưu.
 * @return 1 nếu đã thu thập được dữ liệu thô thành công, 0 nếu không có tín hiệu hoặc buffer đầy.
 */
#define irReadRaw(mcu_pin, irRawData)               irReadPortRaw(mcu_pin, irRawData)

/** 
 * @brief Đọc và giải mã hồng ngoại trực tiếp từ chân vật lý MCU (Ví dụ: IR_LEARN_PIN).
 * @param mcu_pin Số hiệu chân vật lý cấu hình từ user_config.h (Ví dụ: MCU_PIN1).
 * @param irData Con trỏ tới cấu trúc lưu trữ kết quả giải mã @ref IR_Data_t.
 * @return 1 nếu nhận dạng và giải mã thành công, 0 nếu không có tín hiệu hoặc nhiễu.
 */
#define irRead(mcu_pin, irData)                   irReadPort(mcu_pin, irData)

/**
 * @brief Phát tín hiệu hồng ngoại theo cấu trúc xung thô đã thu thập được, không cần giải mã (Hàm chặn).
 * @param mcu_pin Số hiệu chân vật lý kết nối mạch đệm LED phát (Ví dụ: MCU_PIN2).
 * @param irRawData Con trỏ tới cấu trúc chứa dữ liệu xung thô đã thu thập được từ hàm irReadPortRaw.
 */
#define irSendRaw(mcu_pin, irRawData)               irSendPortRaw(mcu_pin, irRawData)

/** 
 * @brief Phát tín hiệu hồng ngoại điều khiển thiết bị từ chân vật lý MCU.
 * @param mcu_pin Số hiệu chân vật lý kết nối mạch đệm LED phát (Ví dụ: MCU_PIN2).
 * @param protocol Tên giao thức cần phát chọn từ @ref IR_Protocol_t.
 * @param address Địa chỉ của thiết bị cần điều khiển.
 * @param command Mã phím bấm/Lệnh cần truyền đi.
 */
#define irSend(mcu_pin, protocol, address, cmd)   irSendPort(mcu_pin, protocol, address, cmd)

/**
 * @brief Quét và giải mã tín hiệu hồng ngoại dựa theo cấu trúc Port và Số pin chỉ định (Hàm chặn).
 * @param GPIOx Con trỏ quản lý vùng nhớ Port (GPIOA, GPIOC, GPIOD).
 * @param pinNumber Số thứ tự bit của chân trong Port (Giá trị từ 0 đến 7).
 * @param irData Con trỏ tới cấu trúc lưu kết quả giải mã thành công.
 * @return 1 nếu giải mã thành công một giao thức, 0 nếu không thành công.
 */
uint8_t irReadPort(const GPIO_TypeDef* GPIOx, uint8_t pinNumber, IR_Data_t* irData);

/**
 * @brief Quét và lưu trữ dữ liệu xung thô từ tín hiệu hồng ngoại vào buffer, không giải mã (Hàm chặn).
 * @param GPIOx Con trỏ quản lý vùng nhớ Port (GPIOA, GPIOC, GPIOD).
 * @param pinNumber Số thứ tự bit của chân trong Port (Giá trị từ 0 đến 7).
 * @param rawData Con trỏ tới cấu trúc lưu kết quả thô của tín hiệu, bao gồm mảng độ rộng xung và số lượng xung đã lưu.
 * @return 1 nếu đã thu thập được dữ liệu thô thành công, 0 nếu không có tín hiệu hoặc buffer đầy.
 */
uint8_t irReadPortRaw(const GPIO_TypeDef* GPIOx, uint8_t pinNumber, IR_RawData_t* rawData);

/**
 * @brief Phát sóng mang hồng ngoại 38kHz theo đúng cấu trúc giao thức ra chân Port chỉ định.
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

#endif // IR_ENABLE

#endif // CH32V003_IR_H

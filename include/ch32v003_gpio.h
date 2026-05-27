/**
 * @file ch32v003_gpio.h
 * @author Vọc Vạch IoT
 * @brief Thư viện cấu hình GPIO cho CH32V003 (J4M6, A4M6, F4P6) hỗ trợ kiểu Arduino.
 * @version 1.2
 * @date 2026-05-26
 */

#ifndef CH32V003_GPIO_H
#define CH32V003_GPIO_H

#include "ch32fun.h"

// ============================================================================
// ĐỊNH NGHĨA CÁC ĐẠI LƯỢNG GIỐNG ARDUINO (STATES & MODES)
// ============================================================================

/** @brief Mức logic thấp (GND / 0V) */
#define LOW  0

/** @brief Mức logic cao (VCC / 3.3V) */
#define HIGH 1

/**
 * @anchor PinMode_t
 * @brief Định nghĩa đầy đủ các chế độ hoạt động của chân GPIO trên CH32V003.
 * @details Giá trị Hex tương ứng trực tiếp với cấu trúc [CNF][MODE] trong thanh ghi CFGLR.
 */
typedef enum
{
    // --- CÁC CHẾ ĐỘ ĐẦU VÀO (INPUT) ---
    ANALOG = 0x00, /**< Đầu vào Analog (ADC / OPA / Tiết kiệm điện) */
    INPUT = 0x04, /**< Đầu vào thả nổi (Floating Input) */
    INPUT_PULLUP = 0x08, /**< Đầu vào có điện trở kéo lên nguồn (Pull-up) */
    INPUT_PULLDOWN = 0x18, /**< Đầu vào có điện trở kéo xuống đất (Pull-down) - Định danh phân biệt bằng bit ảo */

    // --- CÁC CHẾ ĐỘ ĐẦU RA PHỔ THÔNG (GENERAL OUTPUT) ---
    OUTPUT = 0x01, /**< Đầu ra đẩy kéo (Push-Pull) tốc độ 10MHz */
    OUTPUT_20M = 0x02, /**< Đầu ra đẩy kéo (Push-Pull) tốc độ cao 20MHz */
    OUTPUT_50M = 0x03, /**< Đầu ra đẩy kéo (Push-Pull) tốc độ tối đa 50MHz */

    OUTPUT_OD = 0x05, /**< Đầu ra cực máng hở (Open-Drain) tốc độ 10MHz */
    OUTPUT_OD_20M = 0x06, /**< Đầu ra cực máng hở (Open-Drain) tốc độ cao 20MHz */
    OUTPUT_OD_50M = 0x07, /**< Đầu ra cực máng hở (Open-Drain) tốc độ tối đa 50MHz */

    // --- CÁC CHẾ ĐỘ ĐẦU RA NGOẠI VI GÁN CHÂN (ALTERNATE FUNCTION OUTPUT) ---
    AF_PP = 0x09, /**< Đầu ra ngoại vi Đẩy kéo (UART TX, PWM, SPI) 10MHz */
    AF_PP_20M = 0x0A, /**< Đầu ra ngoại vi Đẩy kéo (UART TX, PWM, SPI) 20MHz */
    AF_PP_50M = 0x0B, /**< Đầu ra ngoại vi Đẩy kéo (UART TX, PWM, SPI) 50MHz */

    AF_OD = 0x0D, /**< Đầu ra ngoại vi Máng hở (Bắt buộc cho I2C SDA/SCL) 10MHz */
    AF_OD_20M = 0x0E, /**< Đầu ra ngoại vi Máng hở (Bắt buộc cho I2C SDA/SCL) 20MHz */
    AF_OD_50M = 0x0F /**< Đầu ra ngoại vi Máng hở (Bắt buộc cho I2C SDA/SCL) 50MHz */
} PinMode_t;

// ============================================================================
// LOGIC LOẠI 2: ĐỊNH NGHĨA CHÂN THEO SỐ THỨ TỰ VẬT LÝ TRÊN MCU (MCU PIN)
// Tự động nhận diện dựa trên cờ build_flags từ platformio.ini
// ============================================================================

#if defined(CH32V003_J4M6) // Bản SOP8 (8 chân)
#define MCU_PIN1  GPIOA, 1 /**< Chân vật lý 1 (PA1) */
// #define MCU_PIN1 GPIOD, 6 /**< Chân vật lý 1 (PD6) */
// MCU_PIN2 GND
#define MCU_PIN3  GPIOA, 2 /**< Chân vật lý 2 (PA2) */
// MCU_PIN4 VCC
#define MCU_PIN5  GPIOC, 1 /**< Chân vật lý 5 (PC1) */
#define MCU_PIN6  GPIOC, 2 /**< Chân vật lý 6 (PC2) */
#define MCU_PIN7  GPIOC, 4 /**< Chân vật lý 7 (PC4) */
#define MCU_PIN8  GPIOD, 1 /**< Chân vật lý 8 (PD1 / SWDIO) */
// #define MCU_PIN8 GPIOD, 4 /**< Chân vật lý 8 (PD4) */
// #define MCU_PIN8  GPIOD, 5 /**< Chân vật lý 8 (PD5) */

#elif defined(CH32V003_A4M6) // Bản SOP16 (16 chân)
#define MCU_PIN1  GPIOC, 1  /**< Chân vật lý 1 (PC1) */
#define MCU_PIN2  GPIOC, 2  /**< Chân vật lý 2 (PC2) */
#define MCU_PIN3  GPIOC, 3  /**< Chân vật lý 3 (PC3) */
#define MCU_PIN4  GPIOC, 4  /**< Chân vật lý 4 (PC4) */
#define MCU_PIN5  GPIOC, 6  /**< Chân vật lý 5 (PC6) */
#define MCU_PIN6  GPIOC, 7  /**< Chân vật lý 6 (PC7) */
#define MCU_PIN7  GPIOD, 1  /**< Chân vật lý 7 (PD1 / SWDIO) */
#define MCU_PIN8  GPIOD, 4  /**< Chân vật lý 8 (PD4) */
#define MCU_PIN9  GPIOD, 5  /**< Chân vật lý 9 (PD5) */
#define MCU_PIN10 GPIOD, 6  /**< Chân vật lý 10 (PD6) */
#define MCU_PIN11 GPIOD, 7  /**< Chân vật lý 11 (PD7 / NRST) */
#define MCU_PIN12 GPIOA, 1  /**< Chân vật lý 12 (PA1) */
#define MCU_PIN13 GPIOA, 2  /**< Chân vật lý 13 (PA2) */
// MCU_PIN14 GND
// MCU_PIN15 VCC
#define MCU_PIN16 GPIOC, 0  /**< Chân vật lý 16 (PC0) */

#elif defined(CH32V003_F4P6) // Bản TSSOP20 (20 chân)
#define MCU_PIN1  GPIOD, 4  /**< Chân vật lý 1 (PD4) */
#define MCU_PIN2  GPIOD, 5  /**< Chân vật lý 2 (PD5) */
#define MCU_PIN3  GPIOD, 6  /**< Chân vật lý 3 (PD6) */
#define MCU_PIN4  GPIOD, 7  /**< Chân vật lý 4 (PD7 / NRST) */
#define MCU_PIN5  GPIOA, 1  /**< Chân vật lý 5 (PA1) */
#define MCU_PIN6  GPIOA, 2  /**< Chân vật lý 6 (PA2) */
// MCU_PIN7 GND
#define MCU_PIN8  GPIOD, 0  /**< Chân vật lý 8 (PD0) */
// MCU_PIN9 VCC
#define MCU_PIN10 GPIOC, 0  /**< Chân vật lý 10 (PC0) */
#define MCU_PIN11 GPIOC, 1  /**< Chân vật lý 11 (PC1) */
#define MCU_PIN12 GPIOC, 2  /**< Chân vật lý 12 (PC2) */
#define MCU_PIN13 GPIOC, 3  /**< Chân vật lý 13 (PC3) */
#define MCU_PIN14 GPIOC, 4  /**< Chân vật lý 14 (PC4) */
#define MCU_PIN15 GPIOC, 5  /**< Chân vật lý 15 (PC5) */
#define MCU_PIN16 GPIOC, 6  /**< Chân vật lý 16 (PC6) */
#define MCU_PIN17 GPIOC, 7  /**< Chân vật lý 17 (PC7) */
#define MCU_PIN18 GPIOD, 1  /**< Chân vật lý 18 (PD1 / SWIO) */
#define MCU_PIN19 GPIOD, 2  /**< Chân vật lý 19 (PD2) */
#define MCU_PIN20 GPIOD, 3  /**< Chân vật lý 20 (PD3) */
#endif

/** 
 * @brief Cấu hình chế độ hoạt động cho chân vật lý MCU (kiểu Arduino).
 * @param mcu_pin Số hiệu chân vật lý (Ví dụ: MCU_PIN1).
 * @param mode Chế độ hoạt động chọn từ @ref PinMode_t.
 */
#define pinMode(mcu_pin, mode)         pinModePort(mcu_pin, mode)

/** 
 * @brief Ghi mức logic HIGH hoặc LOW ra chân vật lý MCU.
 * @param mcu_pin Số hiệu chân vật lý (Ví dụ: MCU_PIN1).
 * @param state Mức logic ghi ra (HIGH hoặc LOW).
 */
#define digitalWrite(mcu_pin, state)   digitalWritePort(mcu_pin, state)

/** 
 * @brief Đảo trạng thái logic hiện tại của chân vật lý MCU.
 * @param mcu_pin Số hiệu chân vật lý (Ví dụ: MCU_PIN1).
 */
#define digitalToggle(mcu_pin)         digitalTogglePort(mcu_pin)

/** 
 * @brief Đọc trạng thái logic đầu vào của chân vật lý MCU.
 * @param mcu_pin Số hiệu chân vật lý (Ví dụ: MCU_PIN1).
 * @return Mức logic hiện tại (HIGH hoặc LOW).
 */
#define digitalRead(mcu_pin)           digitalReadPort(mcu_pin)

// ============================================================================
// LOGIC LOẠI 1: CÁC HÀM XỬ LÝ THEO PORT VÀ SỐ PIN RÕ RÀNG
// ============================================================================

/**
 * @brief Thiết lập chế độ chân dựa theo cấu trúc Port và Số pin chỉ định.
 * @param GPIOx Con trỏ quản lý vùng nhớ Port (GPIOA, GPIOC, GPIOD).
 * @param pinNumber Số thứ tự bit của chân trong Port (Giá trị từ 0 đến 7).
 * @param mode Chế độ hoạt động cần cấu hình cấu trúc từ @ref PinMode_t.
 */
void pinModePort(GPIO_TypeDef* GPIOx, uint8_t pinNumber, PinMode_t mode);

/**
 * @brief Xuất mức logic HIGH hoặc LOW ra chân chỉ định của Port.
 * @param GPIOx Con trỏ quản lý vùng nhớ Port (GPIOA, GPIOC, GPIOD).
 * @param pinNumber Số thứ tự bit của chân trong Port (Giá trị từ 0 đến 7).
 * @param state Trạng thái logic mong muốn (HIGH hoặc LOW).
 */
void digitalWritePort(GPIO_TypeDef* GPIOx, uint8_t pinNumber, uint8_t state);

/**
 * @brief Đảo ngược mức logic đầu ra hiện tại (Toggle) của chân thuộc Port chỉ định.
 * @param GPIOx Con trỏ quản lý vùng nhớ Port (GPIOA, GPIOC, GPIOD).
 * @param pinNumber Số thứ tự bit của chân trong Port (Giá trị từ 0 đến 7).
 */
void digitalTogglePort(GPIO_TypeDef* GPIOx, uint8_t pinNumber);

/**
 * @brief Đọc trạng thái logic đầu vào hiện tại của chân từ thanh ghi dữ liệu INDR.
 * @param GPIOx Con trỏ hằng quản lý vùng nhớ Port (GPIOA, GPIOC, GPIOD).
 * @param pinNumber Số thứ tự bit của chân trong Port (Giá trị từ 0 đến 7).
 * @return Trạng thái logic đầu vào (HIGH hoặc LOW).
 */
uint8_t digitalReadPort(const GPIO_TypeDef* GPIOx, uint8_t pinNumber);

#endif // CH32V003_GPIO_H

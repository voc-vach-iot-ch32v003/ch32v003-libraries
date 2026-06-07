/**
 * @file ch32v003_gpio.h
 * @author Vọc Vạch IoT
 * @brief Thư viện cấu hình GPIO cho CH32V003 (J4M6, A4M6, F4P6) hỗ trợ kiểu Arduino.
 * @version 1.3
 * @date 2026-06-07
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
#define MCU_PIN1  GPIOA, GPIO_Pin_1 /**< Chân vật lý 1 (PA1) */
// #define MCU_PIN1 GPIOD, GPIO_Pin_6 /**< Chân vật lý 1 (PD6) */
// MCU_PIN2 GND
#define MCU_PIN3  GPIOA, GPIO_Pin_2 /**< Chân vật lý 2 (PA2) */
// MCU_PIN4 VCC
#define MCU_PIN5  GPIOC, GPIO_Pin_1 /**< Chân vật lý 5 (PC1) */
#define MCU_PIN6  GPIOC, GPIO_Pin_2 /**< Chân vật lý 6 (PC2) */
#define MCU_PIN7  GPIOC, GPIO_Pin_4 /**< Chân vật lý 7 (PC4) */
#define MCU_PIN8  GPIOD, GPIO_Pin_1 /**< Chân vật lý 8 (PD1 / SWDIO) */
// #define MCU_PIN8 GPIOD, GPIO_Pin_4 /**< Chân vật lý 8 (PD4) */
// #define MCU_PIN8  GPIOD, GPIO_Pin_5 /**< Chân vật lý 8 (PD5) */

#elif defined(CH32V003_A4M6) // Bản SOP16 (16 chân)
#define MCU_PIN1  GPIOC, GPIO_Pin_1  /**< Chân vật lý 1 (PC1) */
#define MCU_PIN2  GPIOC, GPIO_Pin_2  /**< Chân vật lý 2 (PC2) */
#define MCU_PIN3  GPIOC, GPIO_Pin_3  /**< Chân vật lý 3 (PC3) */
#define MCU_PIN4  GPIOC, GPIO_Pin_4  /**< Chân vật lý 4 (PC4) */
#define MCU_PIN5  GPIOC, GPIO_Pin_6  /**< Chân vật lý 5 (PC6) */
#define MCU_PIN6  GPIOC, GPIO_Pin_7  /**< Chân vật lý 6 (PC7) */
#define MCU_PIN7  GPIOD, GPIO_Pin_1  /**< Chân vật lý 7 (PD1 / SWDIO) */
#define MCU_PIN8  GPIOD, GPIO_Pin_4  /**< Chân vật lý 8 (PD4) */
#define MCU_PIN9  GPIOD, GPIO_Pin_5  /**< Chân vật lý 9 (PD5) */
#define MCU_PIN10 GPIOD, GPIO_Pin_6  /**< Chân vật lý 10 (PD6) */
#define MCU_PIN11 GPIOD, GPIO_Pin_7  /**< Chân vật lý 11 (PD7 / NRST) */
#define MCU_PIN12 GPIOA, GPIO_Pin_1  /**< Chân vật lý 12 (PA1) */
#define MCU_PIN13 GPIOA, GPIO_Pin_2  /**< Chân vật lý 13 (PA2) */
// MCU_PIN14 GND
// MCU_PIN15 VCC
#define MCU_PIN16 GPIOC, GPIO_Pin_0  /**< Chân vật lý 16 (PC0) */

#elif defined(CH32V003_F4P6) // Bản TSSOP20 (20 chân)
#define MCU_PIN1  GPIOD, GPIO_Pin_4  /**< Chân vật lý 1 (PD4) */
#define MCU_PIN2  GPIOD, GPIO_Pin_5  /**< Chân vật lý 2 (PD5) */
#define MCU_PIN3  GPIOD, GPIO_Pin_6  /**< Chân vật lý 3 (PD6) */
#define MCU_PIN4  GPIOD, GPIO_Pin_7  /**< Chân vật lý 4 (PD7 / NRST) */
#define MCU_PIN5  GPIOA, GPIO_Pin_1  /**< Chân vật lý 5 (PA1) */
#define MCU_PIN6  GPIOA, GPIO_Pin_2  /**< Chân vật lý 6 (PA2) */
// MCU_PIN7 GND
#define MCU_PIN8  GPIOD, GPIO_Pin_0  /**< Chân vật lý 8 (PD0) */
// MCU_PIN9 VCC
#define MCU_PIN10 GPIOC, GPIO_Pin_0  /**< Chân vật lý 10 (PC0) */
#define MCU_PIN11 GPIOC, GPIO_Pin_1  /**< Chân vật lý 11 (PC1) */
#define MCU_PIN12 GPIOC, GPIO_Pin_2  /**< Chân vật lý 12 (PC2) */
#define MCU_PIN13 GPIOC, GPIO_Pin_3  /**< Chân vật lý 13 (PC3) */
#define MCU_PIN14 GPIOC, GPIO_Pin_4  /**< Chân vật lý 14 (PC4) */
#define MCU_PIN15 GPIOC, GPIO_Pin_5  /**< Chân vật lý 15 (PC5) */
#define MCU_PIN16 GPIOC, GPIO_Pin_6  /**< Chân vật lý 16 (PC6) */
#define MCU_PIN17 GPIOC, GPIO_Pin_7  /**< Chân vật lý 17 (PC7) */
#define MCU_PIN18 GPIOD, GPIO_Pin_1  /**< Chân vật lý 18 (PD1 / SWIO) */
#define MCU_PIN19 GPIOD, GPIO_Pin_2  /**< Chân vật lý 19 (PD2) */
#define MCU_PIN20 GPIOD, GPIO_Pin_3  /**< Chân vật lý 20 (PD3) */
#endif

// ============================================================================
// LOGIC LOẠI 3: ĐỊNH NGHĨA CHÂN ĐẶC BIỆT CHO CÁC CHỨC NĂNG PHỔ BIẾN
// Tự động nhận diện dựa trên cờ build_flags từ platformio.ini
// ============================================================================
// --- CẤP ĐỘ 1: Tất cả các dòng chip (J4M6, A4M6, F4P6) đều có ---
#if defined(CH32V003_J4M6) || defined(CH32V003_A4M6) || defined(CH32V003_F4P6)
#define MCU_SWIO      GPIOD, GPIO_Pin_1  /**< Chân nạp chương trình Single-wire */
#define MCU_I2C_SDA   GPIOC, GPIO_Pin_1  /**< [I2C SDA] -> SOP8: Chân 5 | SOP16: Chân 1 | TSSOP20: Chân 11 */
#define MCU_I2C_SCL   GPIOC, GPIO_Pin_2  /**< [I2C SCL] -> SOP8: Chân 6 | SOP16: Chân 2 | TSSOP20: Chân 12 */
#define MCU_UART_TX   GPIOD, GPIO_Pin_5  /**< [UART TX] -> SOP8: Chân 8 (Remap) | SOP16: Chân 9 | TSSOP20: Chân 2 */
#define MCU_UART_RX   GPIOD, GPIO_Pin_6  /**< [UART RX] -> SOP8: Chân 1 (Remap) | SOP16: Chân 10 | TSSOP20: Chân 3 */
#define MCU_SPI_NSS   GPIOC, GPIO_Pin_1  /**< [SPI NSS] -> SOP8: Chân 5 | SOP16: Chân 1 | TSSOP20: Chân 11 */

/* --- CÁC KÊNH ADC CÓ TRÊN CẢ 3 BẢN --- */
#define MCU_ADC_CH5   GPIOD, GPIO_Pin_4  /**< [ADC CH5] -> SOP8: Chân 8 | SOP16: Chân 8 | TSSOP20: Chân 1 */
#define MCU_ADC_CH7   GPIOC, GPIO_Pin_1  /**< [ADC CH7] -> SOP8: Chân 5 | SOP16: Chân 1 | TSSOP20: Chân 11 */
#define MCU_ADC_CH8   GPIOC, GPIO_Pin_2  /**< [ADC CH8] -> SOP8: Chân 6 | SOP16: Chân 2 | TSSOP20: Chân 12 */
#endif

// --- CẤP ĐỘ 2: Chỉ dành cho bản SOP16 và TSSOP20 (Sử dụng #if độc lập để kế thừa) ---
#if defined(CH32V003_A4M6) || defined(CH32V003_F4P6)
#define MCU_NRST      GPIOD, GPIO_Pin_7  /**< [RESET CỨNG] -> SOP16: Chân 11 | TSSOP20: Chân 4 */
#define MCU_SPI_MOSI  GPIOC, GPIO_Pin_6  /**< [SPI MOSI] -> SOP16: Chân 5 | TSSOP20: Chân 16 */
#define MCU_SPI_MISO  GPIOC, GPIO_Pin_7  /**< [SPI MISO] -> SOP16: Chân 6 | TSSOP20: Chân 17 */

/* --- MỞ RỘNG KÊNH ADC CHO SOP16 & TSSOP20 --- */
#define MCU_ADC_CH0   GPIOA, GPIO_Pin_2  /**< [ADC CH0] -> SOP16: Chân 13 | TSSOP20: Chân 6 */
#define MCU_ADC_CH1   GPIOA, GPIO_Pin_1  /**< [ADC CH1] -> SOP16: Chân 12 | TSSOP20: Chân 5 */
#define MCU_ADC_CH2   GPIOC, GPIO_Pin_4  /**< [ADC CH2] -> SOP16: Chân 4  | TSSOP20: Chân 14 */
#define MCU_ADC_CH6   GPIOD, GPIO_Pin_5  /**< [ADC CH6] -> SOP16: Chân 9  | TSSOP20: Chân 2 */
#endif

// --- CẤP ĐỘ 3: Chỉ dành riêng cho bản đầy đủ nhất TSSOP20 ---
#if defined(CH32V003_F4P6)
#define MCU_SPI_SCK   GPIOC, GPIO_Pin_5  /**< [SPI SCK Mặc định] -> TSSOP20: Chân 15 */

/* --- MỞ RỘNG CÁC KÊNH ADC CÒN LẠI CỦA TSSOP20 --- */
#define MCU_ADC_CH3   GPIOC, GPIO_Pin_5  /**< [ADC CH3] -> TSSOP20: Chân 15 */
#define MCU_ADC_CH4   GPIOC, GPIO_Pin_3  /**< [ADC CH4] -> TSSOP20: Chân 13 */
#endif

// ============================================================================
// LOGIC LOẠI 4: ĐỊNH NGHĨA CHÂN BUILTIN CHO TỪNG BOARD (LED, BUTTON, v.v.)
// Tự động nhận diện dựa trên cờ build_flags từ platformio.ini
// ============================================================================
#if defined(CH32V003_J4M6)
#elif defined(CH32V003_A4M6)
#elif defined(CH32V003_F4P6)
#define LED_BUILTIN_TWEN32F003 GPIOD, GPIO_Pin_1 /**< Chân mặc định cho LED trên board dạng Twen32F003 */
#define BTN_BUILTIN_TWEN32F003 GPIOC, GPIO_Pin_0 /**< Chân mặc định cho nút bấm trên board dạng Twen32F003 */
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

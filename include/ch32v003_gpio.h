/**
 * @file ch32v003_gpio.h
 * @author Vọc Vạch IoT
 * @brief Thư viện cấu hình GPIO nâng cao cho CH32V003 (J4M6, A4M6, F4P6) hỗ trợ kiểu Arduino.
 * @version 2.0
 * @date 2026-06-07
 */

#ifndef CH32V003_GPIO_H
#define CH32V003_GPIO_H

#include "ch32fun.h"

// ============================================================================
// ĐỊNH NGHĨA TRẠNG THÁI LOGIC & CHẾ ĐỘ HOẠT ĐỘNG (STATES & MODES)
// ============================================================================

/**
 * @brief Định nghĩa mức logic của chân tín hiệu.
 */
typedef enum
{
    LOW = 0, /**< Mức logic thấp (GND / 0V) */
    HIGH = 1 /**< Mức logic cao (VCC / 3.3V) */
} DigitalState_t;

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

/**
 * @brief Cấu hình chế độ hoạt động cho chân chức năng nạp SWIO (PD1).
 */
typedef enum
{
    SWIO_MODE_DEBUG = 0, /**< Chế độ nạp / gỡ lỗi một dây (Mặc định). Chân PD1 dành riêng cho chức năng nạp SWD. */
    SWIO_MODE_GPIO = 1 /**< Giải phóng chân thành GPIO thường. Chân PD1 có thể dùng điều khiển thiết bị/LED. */
} SwioMode_t;

// ============================================================================
// ĐỊNH NGHĨA CHÂN THEO SỐ THỨ TỰ VẬT LÝ TRÊN MCU (MCUPin_t)
// Kiến trúc Bit-packing: (Mã hóa Port << 4) | Số chân Pin. 
// Trong đó: Port A = 0, Port C = 1, Port D = 2.
// ============================================================================

/**
 * @brief Định nghĩa danh sách chân vật lý dựa trên sơ đồ đóng gói IC thực tế.
 */
typedef enum
{
#if defined(CH32V003_J4M6) // ================= Bản SOP8 (8 chân) =================
    /** @brief Chân vật lý 1: Cổng Port A - Pin 1 (PA1). Chức năng: GPIO / ADC CH1 / XTALI */
    MCU_PIN1 = (0 << 4) | 1,
    /** @brief Chân vật lý 3: Cổng Port A - Pin 2 (PA2). Chức năng: GPIO / ADC CH0 / XTALO */
    MCU_PIN3 = (0 << 4) | 2,
    /** @brief Chân vật lý 5: Cổng Port C - Pin 1 (PC1). Chức năng: GPIO / ADC CH7 / I2C SDA / SPI NSS */
    MCU_PIN5 = (1 << 4) | 1,
    /** @brief Chân vật lý 6: Cổng Port C - Pin 2 (PC2). Chức năng: GPIO / ADC CH8 / I2C SCL */
    MCU_PIN6 = (1 << 4) | 2,
    /** @brief Chân vật lý 7: Cổng Port C - Pin 4 (PC4). Chức năng: GPIO / ADC CH2 / TIM1 CH4 */
    MCU_PIN7 = (1 << 4) | 4,
    /** @brief Chân vật lý 8: Cổng Port D - Pin 1 (PD1). Chức năng: GPIO / SWDIO (Chân nạp mặc định) */
    MCU_PIN8 = (2 << 4) | 1

#elif defined(CH32V003_A4M6) // ================= Bản SOP16 (16 chân) =================
    /** @brief Chân vật lý 1: Cổng Port C - Pin 1 (PC1). Chức năng: GPIO / ADC CH7 / I2C SDA / SPI NSS */
    MCU_PIN1 = (1 << 4) | 1,
        /** @brief Chân vật lý 2: Cổng Port C - Pin 2 (PC2). Chức năng: GPIO / ADC CH8 / I2C SCL */
        MCU_PIN2 = (1 << 4) | 2,
        /** @brief Chân vật lý 3: Cổng Port C - Pin 3 (PC3). Chức năng: GPIO / ADC CH4 / TIM1 CH3 */
        MCU_PIN3 = (1 << 4) | 3,
        /** @brief Chân vật lý 4: Cổng Port C - Pin 4 (PC4). Chức năng: GPIO / ADC CH2 / TIM1 CH4 */
        MCU_PIN4 = (1 << 4) | 4,
        /** @brief Chân vật lý 5: Cổng Port C - Pin 6 (PC6). Chức năng: GPIO / TIM1 CH1 / SPI MOSI */
        MCU_PIN5 = (1 << 4) | 6,
        /** @brief Chân vật lý 6: Cổng Port C - Pin 7 (PC7). Chức năng: GPIO / TIM1 CH2 / SPI MISO */
        MCU_PIN6 = (1 << 4) | 7,
        /** @brief Chân vật lý 7: Cổng Port D - Pin 1 (PD1). Chức năng: GPIO / SWDIO (Chân nạp mặc định) */
        MCU_PIN7 = (2 << 4) | 1,
        /** @brief Chân vật lý 8: Cổng Port D - Pin 4 (PD4). Chức năng: GPIO / ADC CH5 / TIM1 CH4 / UART RX (Remap) */
        MCU_PIN8 = (2 << 4) | 4,
        /** @brief Chân vật lý 9: Cổng Port D - Pin 5 (PD5). Chức năng: GPIO / ADC CH6 / TIM1 CH1 / UART TX */
        MCU_PIN9 = (2 << 4) | 5,
        /** @brief Chân vật lý 10: Cổng Port D - Pin 6 (PD6). Chức năng: GPIO / ADC CH3 / TIM1 CH2 / UART RX */
        MCU_PIN10 = (2 << 4) | 6,
        /** @brief Chân vật lý 11: Cổng Port D - Pin 7 (PD7). Chức năng: GPIO / NRST (Chân Reset phần cứng) */
        MCU_PIN11 = (2 << 4) | 7,
        /** @brief Chân vật lý 12: Cổng Port A - Pin 1 (PA1). Chức năng: GPIO / ADC CH1 / XTALI */
        MCU_PIN12 = (0 << 4) | 1,
        /** @brief Chân vật lý 13: Cổng Port A - Pin 2 (PA2). Chức năng: GPIO / ADC CH0 / XTALO */
        MCU_PIN13 = (0 << 4) | 2,
        /** @brief Chân vật lý 16: Cổng Port C - Pin 0 (PC0). Chức năng: GPIO / TIM1 CH1 / TIM1 ETRI */
        MCU_PIN16 = (1 << 4) | 0

#elif defined(CH32V003_F4P6) // ================= Bản TSSOP20 (20 chân) =================
    /** @brief Chân vật lý 1: Cổng Port D - Pin 4 (PD4). Chức năng: GPIO / ADC CH5 / TIM1 CH4 / UART RX (Remap) */
    MCU_PIN1 = (2 << 4) | 4,
        /** @brief Chân vật lý 2: Cổng Port D - Pin 5 (PD5). Chức năng: GPIO / ADC CH6 / TIM1 CH1 / UART TX */
        MCU_PIN2 = (2 << 4) | 5,
        /** @brief Chân vật lý 3: Cổng Port D - Pin 6 (PD6). Chức năng: GPIO / ADC CH3 / TIM1 CH2 / UART RX */
        MCU_PIN3 = (2 << 4) | 6,
        /** @brief Chân vật lý 4: Cổng Port D - Pin 7 (PD7). Chức năng: GPIO / NRST (Chân Reset phần cứng) */
        MCU_PIN4 = (2 << 4) | 7,
        /** @brief Chân vật lý 5: Cổng Port A - Pin 1 (PA1). Chức năng: GPIO / ADC CH1 / XTALI */
        MCU_PIN5 = (0 << 4) | 1,
        /** @brief Chân vật lý 6: Cổng Port A - Pin 2 (PA2). Chức năng: GPIO / ADC CH0 / XTALO */
        MCU_PIN6 = (0 << 4) | 2,
        /** @brief Chân vật lý 8: Cổng Port D - Pin 0 (PD0). Chức năng: GPIO / TIM1 CH1 / TIM1 FLT */
        MCU_PIN8 = (2 << 4) | 0,
        /** @brief Chân vật lý 10: Cổng Port C - Pin 0 (PC0). Chức năng: GPIO / TIM1 CH1 / TIM1 ETRI */
        MCU_PIN10 = (1 << 4) | 0,
        /** @brief Chân vật lý 11: Cổng Port C - Pin 1 (PC1). Chức năng: GPIO / ADC CH7 / I2C SDA / SPI NSS */
        MCU_PIN11 = (1 << 4) | 1,
        /** @brief Chân vật lý 12: Cổng Port C - Pin 2 (PC2). Chức năng: GPIO / ADC CH8 / I2C SCL */
        MCU_PIN12 = (1 << 4) | 2,
        /** @brief Chân vật lý 13: Cổng Port C - Pin 3 (PC3). Chức năng: GPIO / ADC CH4 / TIM1 CH3 */
        MCU_PIN13 = (1 << 4) | 3,
        /** @brief Chân vật lý 14: Cổng Port C - Pin 4 (PC4). Chức năng: GPIO / ADC CH2 / TIM1 CH4 */
        MCU_PIN14 = (1 << 4) | 4,
        /** @brief Chân vật lý 15: Cổng Port C - Pin 5 (PC5). Chức năng: GPIO / ADC CH3 / SPI SCK */
        MCU_PIN15 = (1 << 4) | 5,
        /** @brief Chân vật lý 16: Cổng Port C - Pin 6 (PC6). Chức năng: GPIO / TIM1 CH1 / SPI MOSI */
        MCU_PIN16 = (1 << 4) | 6,
        /** @brief Chân vật lý 17: Cổng Port C - Pin 7 (PC7). Chức năng: GPIO / TIM1 CH2 / SPI MISO */
        MCU_PIN17 = (1 << 4) | 7,
        /** @brief Chân vật lý 18: Cổng Port D - Pin 1 (PD1). Chức năng: GPIO / SWDIO (Chân nạp mặc định) */
        MCU_PIN18 = (2 << 4) | 1,
        /** @brief Chân vật lý 19: Cổng Port D - Pin 2 (PD2). Chức năng: GPIO / TIM1 CH2 / UART TX (Remap) */
        MCU_PIN19 = (2 << 4) | 2,
        /** @brief Chân vật lý 20: Cổng Port D - Pin 3 (PD3). Chức năng: GPIO / TIM1 CH3 / UART RX (Remap) */
        MCU_PIN20 = (2 << 4) | 3
#endif
} MCUPin_t;

// ============================================================================
// ĐỊNH NGHĨA CHÂN NGOẠI VI ĐẶC BIỆT (MCUSpecialPin_t)
// ============================================================================

/**
 * @brief Định nghĩa các chân chức năng đặc biệt của hệ thống (I2C, UART, SPI, ADC, v.v.).
 */
typedef enum
{
    /** * @brief [GIAO TIẾP] Chân nạp chương trình Single-wire (SWIO).
     * - Vị trí: PD1 trên mọi gói vỏ (SOP8: Chân 8 | SOP16: Chân 7 | TSSOP20: Chân 18).
     */
    MCU_SWIO = (2 << 4) | 1,

    /** * @brief [GIAO TIẾP] Chân truyền dữ liệu I2C SDA.
     * - Vị trí: PC1 trên mọi gói vỏ (SOP8: Chân 5 | SOP16: Chân 1 | TSSOP20: Chân 11).
     */
    MCU_I2C_SDA = (1 << 4) | 1,

    /** * @brief [GIAO TIẾP] Chân tạo xung nhịp I2C SCL.
     * - Vị trí: PC2 trên mọi gói vỏ (SOP8: Chân 6 | SOP16: Chân 2 | TSSOP20: Chân 12).
     */
    MCU_I2C_SCL = (1 << 4) | 2,

    /** * @brief [GIAO TIẾP] Chân truyền tín hiệu UART TX.
     * - Vị trí: PD5 trên mọi gói vỏ (SOP8: Chân 8-Remap | SOP16: Chân 9 | TSSOP20: Chân 2).
     */
    MCU_UART_TX = (2 << 4) | 5,

    /** * @brief [GIAO TIẾP] Chân nhận tín hiệu UART RX.
     * - Vị trí: PD6 trên mọi gói vỏ (SOP8: Chân 1-Remap | SOP16: Chân 10 | TSSOP20: Chân 3).
     */
    MCU_UART_RX = (2 << 4) | 6,

    /** * @brief [GIAO TIẾP] Chân chọn chip ngoại vi SPI NSS.
     * - Vị trí: PC1 trên mọi gói vỏ (SOP8: Chân 5 | SOP16: Chân 1 | TSSOP20: Chân 11).
     */
    MCU_SPI_NSS = (1 << 4) | 1,

    /** * @brief [NGOẠI VI ANLOG] Kênh chuyển đổi ADC CH5.
     * - Vị trí: PD4 trên mọi gói vỏ (SOP8: Chân 8 | SOP16: Chân 8 | TSSOP20: Chân 1).
     */
    MCU_ADC_CH5 = (2 << 4) | 4,

    /** * @brief [NGOẠI VI ANLOG] Kênh chuyển đổi ADC CH7.
     * - Vị trí: PC1 trên mọi gói vỏ (SOP8: Chân 5 | SOP16: Chân 1 | TSSOP20: Chân 11).
     */
    MCU_ADC_CH7 = (1 << 4) | 1,

    /** * @brief [NGOẠI VI ANLOG] Kênh chuyển đổi ADC CH8.
     * - Vị trí: PC2 trên mọi gói vỏ (SOP8: Chân 6 | SOP16: Chân 2 | TSSOP20: Chân 12).
     */
    MCU_ADC_CH8 = (1 << 4) | 2,

#if defined(CH32V003_A4M6) || defined(CH32V003_F4P6)
    /** * @brief [HỆ THỐNG] Chân Reset cứng (NRST).
     * - Vị trí: PD7 (SOP16: Chân 11 | TSSOP20: Chân 4). Không có trên bản SOP8.
     */
    MCU_NRST = (2 << 4) | 7,

        /** * @brief [GIAO TIẾP] Chân xuất dữ liệu ngoại vi SPI MOSI.
         * - Vị trí: PC6 (SOP16: Chân 5 | TSSOP20: Chân 16). Không có trên bản SOP8.
         */
        MCU_SPI_MOSI = (1 << 4) | 6,

        /** * @brief [GIAO TIẾP] Chân nhận dữ liệu ngoại vi SPI MISO.
         * - Vị trí: PC7 (SOP16: Chân 6 | TSSOP20: Chân 17). Không có trên bản SOP8.
         */
        MCU_SPI_MISO = (1 << 4) | 7,

        /** * @brief [NGOẠI VI ANLOG] Kênh chuyển đổi ADC CH0.
         * - Vị trí: PA2 (SOP16: Chân 13 | TSSOP20: Chân 6).
         */
        MCU_ADC_CH0 = (0 << 4) | 2,

        /** * @brief [NGOẠI VI ANLOG] Kênh chuyển đổi ADC CH1.
         * - Vị trí: PA1 (SOP16: Chân 12 | TSSOP20: Chân 5).
         */
        MCU_ADC_CH1 = (0 << 4) | 1,

        /** * @brief [NGOẠI VI ANLOG] Kênh chuyển đổi ADC CH2.
         * - Vị trí: PC4 (SOP16: Chân 4 | TSSOP20: Chân 14).
         */
        MCU_ADC_CH2 = (1 << 4) | 4,

        /** * @brief [NGOẠI VI ANLOG] Kênh chuyển đổi ADC CH6.
         * - Vị trí: PD5 (SOP16: Chân 9 | TSSOP20: Chân 2).
         */
        MCU_ADC_CH6 = (2 << 4) | 5, 
#endif

#if defined(CH32V003_F4P6)
    /** * @brief [GIAO TIẾP] Chân tạo xung nhịp đồng bộ SPI SCK.
     * - Vị trí: PC5 (Chỉ khả dụng trên cấu hình đầy đủ TSSOP20: Chân 15).
     */
    MCU_SPI_SCK = (1 << 4) | 5,

        /** * @brief [NGOẠI VI ANLOG] Kênh chuyển đổi ADC CH3.
         * - Vị trí: PC5 (Chỉ khả dụng trên cấu hình đầy đủ TSSOP20: Chân 15).
         */
        MCU_ADC_CH3 = (1 << 4) | 5,

        /** * @brief [NGOẠI VI ANLOG] Kênh chuyển đổi ADC CH4.
         * - Vị trí: PC3 (Chỉ khả dụng trên cấu hình đầy đủ TSSOP20: Chân 13).
         */
        MCU_ADC_CH4 = (1 << 4) | 3, 
#endif

    // --- TIỆN ÍCH PHẦN CỨNG SẴN CÓ TRÊN BOARD PHA THÁT ---
#if defined(CH32V003_F4P6)
    /** @brief Chân đấu nối đèn LED tích hợp (Built-in) trên kit phát triển Twen32F003 (PD1). */
    LED_BUILTIN_TWEN32F003 = (2 << 4) | 1,
        /** @brief Chân đấu nối Nút nhấn tích hợp (Built-in) trên kit phát triển Twen32F003 (PC0). */
        BTN_BUILTIN_TWEN32F003 = (1 << 4) | 0
#endif
} MCUSpecialPin_t;

// ============================================================================
// NGUYÊN MẪU CÁC HÀM XỬ LÝ (API FUNCTIONS)
// Cấu trúc phân tách logic rõ ràng không sử dụng Macro thay thế thô
// ============================================================================

/**
 * @brief Hàm chuyển đổi mã hóa chân vật lý MCU thành con trỏ Port GPIO và số pin tương ứng.
 * @param mcuPin Chân vật lý được mã hóa theo cấu trúc (Port << 4) | PinNumber.
 * @param GPIOx Pointer trả về con trỏ quản lý vùng nhớ Port GPIO tương ứng (GPIOA, GPIOC, GPIOD).
 * @param pinNumber Giá trị trả về số thứ tự bit của chân trong Port (0-7) được giải mã từ mcuPin.
 * @warning Hàm này chỉ giải mã chân vật lý được định nghĩa trong @ref MCUPin_t hoặc @ref MCUSpecialPin_t
 */
void decodeHardwarePin(uint8_t mcuPin, GPIO_TypeDef** GPIOx, uint8_t* pinNumber);

/** * @brief Cấu hình chế độ hoạt động cho chân vật lý MCU (Kiểu hàm Arduino).
 * @param mcu_pin Chân vật lý được chọn từ bảng mã hóa @ref MCUPin_t hoặc @ref MCUSpecialPin_t.
 * @param mode Chế độ hoạt động mong muốn lấy từ cấu trúc danh mục @ref PinMode_t.
 */
void pinMode(uint8_t mcu_pin, PinMode_t mode);

/** * @brief Ghi mức logic HIGH hoặc LOW trực tiếp ra chân vật lý MCU.
 * @param mcu_pin Chân vật lý được chọn từ bảng mã hóa @ref MCUPin_t hoặc @ref MCUSpecialPin_t.
 * @param state Trạng thái logic cần ghi truyền vào từ cấu trúc @ref DigitalState_t.
 */
void digitalWrite(uint8_t mcu_pin, DigitalState_t state);

/** * @brief Đảo ngược tức thời trạng thái logic hiện tại của chân vật lý MCU.
 * @param mcu_pin Chân vật lý được chọn từ bảng mã hóa @ref MCUPin_t hoặc @ref MCUSpecialPin_t.
 */
void digitalToggle(uint8_t mcu_pin);

/** * @brief Đọc giá trị mức logic hiện tại của chân vật lý được cấu hình làm đầu vào.
 * @param mcu_pin Chân vật lý được chọn từ bảng mã hóa @ref MCUPin_t hoặc @ref MCUSpecialPin_t.
 * @return Mức logic hiện trạng đọc về từ thanh ghi cổng (Thuộc @ref DigitalState_t).
 */
DigitalState_t digitalRead(uint8_t mcu_pin);

// --- HỆ THỐNG CÁC HÀM QUẢN LÝ TIÊN TIẾN CHUYÊN DỤNG CHO CHÂN NẠP SWIO ---

/**
 * @brief Thiết lập chế độ hoạt động trực tiếp cho chân nạp chương trình SWIO (PD1).
 * @param swio_mode Chế độ thiết lập được cấu hình dựa trên @ref SwioMode_t.
 * @warning Việc chuyển sang chế độ @ref SWIO_MODE_GPIO sẽ vô hiệu hóa tính năng nạp chương trình tạm thời 
 * cho đến khi chip được đặt lại hoặc cấu hình trả lại chế độ gỡ lỗi.
 */
void setSwioMode(SwioMode_t swio_mode);

/**
 * @brief Đọc trạng thái cấu hình hiện tại của chân gỡ lỗi / nạp chương trình SWIO (PD1).
 * @return Chế độ hoạt động hiện tại thu thập từ phần cứng (Trả về @ref SwioMode_t).
 */
SwioMode_t getSwioMode(void);

/**
 * @brief Thực hiện đảo trạng thái chức năng chân SWIO tự động từ chế độ GPIO sang DEBUG hoặc ngược lại.
 */
void toggleSwioMode(void);

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

/**
 * @file ch32v003_gpio_pins.h
 * @author Vọc Vạch IoT
 * @brief Định nghĩa sơ đồ chân vật lý (MCU_Pin_t) và chân chức năng đặc biệt (MCU_SpecialPin_t) cho CH32V003.
 * @version 2.0.0
 * @date 2026-06-07
 */

#ifndef CH32V003_GPIO_PINS_H
#define CH32V003_GPIO_PINS_H

// ============================================================================
// ĐỊNH NGHĨA CHÂN THEO SỐ THỨ TỰ VẬT LÝ TRÊN MCU (MCU_Pin_t)
// Kiến trúc Bit-packing: (Mã hóa Port << 4) | Số chân Pin.
// Trong đó: Port A = 0, Port C = 1, Port D = 2.
// ============================================================================

#if !defined(CH32V003_J4M6) && !defined(CH32V003_A4M6) && !defined(CH32V003_F4P6)
#define CH32V003_J4M6 /**< Mặc định chọn gói SOP8 nếu không xác định trước */
#endif

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
    MCU_PIN1= (1 << 4) | 1,
    /** @brief Chân vật lý 2: Cổng Port C - Pin 2 (PC2). Chức năng: GPIO / ADC CH8 / I2C SCL */
    MCU_PIN2= (1 << 4) | 2,
    /** @brief Chân vật lý 3: Cổng Port C - Pin 3 (PC3). Chức năng: GPIO / ADC CH4 / TIM1 CH3 */
    MCU_PIN3= (1 << 4) | 3,
    /** @brief Chân vật lý 4: Cổng Port C - Pin 4 (PC4). Chức năng: GPIO / ADC CH2 / TIM1 CH4 */
    MCU_PIN4= (1 << 4) | 4,
    /** @brief Chân vật lý 5: Cổng Port C - Pin 6 (PC6). Chức năng: GPIO / TIM1 CH1 / SPI MOSI */
    MCU_PIN5= (1 << 4) | 6,
    /** @brief Chân vật lý 6: Cổng Port C - Pin 7 (PC7). Chức năng: GPIO / TIM1 CH2 / SPI MISO */
    MCU_PIN6= (1 << 4) | 7,
    /** @brief Chân vật lý 7: Cổng Port D - Pin 1 (PD1). Chức năng: GPIO / SWDIO (Chân nạp mặc định) */
    MCU_PIN7= (2 << 4) | 1,
    /** @brief Chân vật lý 8: Cổng Port D - Pin 4 (PD4). Chức năng: GPIO / ADC CH5 / TIM1 CH4 / UART RX (Remap) */
    MCU_PIN8= (2 << 4) | 4,
    /** @brief Chân vật lý 9: Cổng Port D - Pin 5 (PD5). Chức năng: GPIO / ADC CH6 / TIM1 CH1 / UART TX */
    MCU_PIN9= (2 << 4) | 5,
    /** @brief Chân vật lý 10: Cổng Port D - Pin 6 (PD6). Chức năng: GPIO / ADC CH3 / TIM1 CH2 / UART RX */
    MCU_PIN10= (2 << 4) | 6,
    /** @brief Chân vật lý 11: Cổng Port D - Pin 7 (PD7). Chức năng: GPIO / NRST (Chân Reset phần cứng) */
    MCU_PIN11= (2 << 4) | 7,
    /** @brief Chân vật lý 12: Cổng Port A - Pin 1 (PA1). Chức năng: GPIO / ADC CH1 / XTALI */
    MCU_PIN12= (0 << 4) | 1,
    /** @brief Chân vật lý 13: Cổng Port A - Pin 2 (PA2). Chức năng: GPIO / ADC CH0 / XTALO */
    MCU_PIN13= (0 << 4) | 2,
    /** @brief Chân vật lý 16: Cổng Port C - Pin 0 (PC0). Chức năng: GPIO / TIM1 CH1 / TIM1 ETRI */
    MCU_PIN16= (1 << 4) | 0

#elif defined(CH32V003_F4P6) // ================= Bản TSSOP20 (20 chân) =================
    /** @brief Chân vật lý 1: Cổng Port D - Pin 4 (PD4). Chức năng: GPIO / ADC CH5 / TIM1 CH4 / UART RX (Remap) */
    MCU_PIN1= (2 << 4) | 4,
    /** @brief Chân vật lý 2: Cổng Port D - Pin 5 (PD5). Chức năng: GPIO / ADC CH6 / TIM1 CH1 / UART TX */
    MCU_PIN2= (2 << 4) | 5,
    /** @brief Chân vật lý 3: Cổng Port D - Pin 6 (PD6). Chức năng: GPIO / ADC CH3 / TIM1 CH2 / UART RX */
    MCU_PIN3= (2 << 4) | 6,
    /** @brief Chân vật lý 4: Cổng Port D - Pin 7 (PD7). Chức năng: GPIO / NRST (Chân Reset phần cứng) */
    MCU_PIN4= (2 << 4) | 7,
    /** @brief Chân vật lý 5: Cổng Port A - Pin 1 (PA1). Chức năng: GPIO / ADC CH1 / XTALI */
    MCU_PIN5= (0 << 4) | 1,
    /** @brief Chân vật lý 6: Cổng Port A - Pin 2 (PA2). Chức năng: GPIO / ADC CH0 / XTALO */
    MCU_PIN6= (0 << 4) | 2,
    /** @brief Chân vật lý 8: Cổng Port D - Pin 0 (PD0). Chức năng: GPIO / TIM1 CH1 / TIM1 FLT */
    MCU_PIN8= (2 << 4) | 0,
    /** @brief Chân vật lý 10: Cổng Port C - Pin 0 (PC0). Chức năng: GPIO / TIM1 CH1 / TIM1 ETRI */
    MCU_PIN10= (1 << 4) | 0,
    /** @brief Chân vật lý 11: Cổng Port C - Pin 1 (PC1). Chức năng: GPIO / ADC CH7 / I2C SDA / SPI NSS */
    MCU_PIN11= (1 << 4) | 1,
    /** @brief Chân vật lý 12: Cổng Port C - Pin 2 (PC2). Chức năng: GPIO / ADC CH8 / I2C SCL */
    MCU_PIN12= (1 << 4) | 2,
    /** @brief Chân vật lý 13: Cổng Port C - Pin 3 (PC3). Chức năng: GPIO / ADC CH4 / TIM1 CH3 */
    MCU_PIN13= (1 << 4) | 3,
    /** @brief Chân vật lý 14: Cổng Port C - Pin 4 (PC4). Chức năng: GPIO / ADC CH2 / TIM1 CH4 */
    MCU_PIN14= (1 << 4) | 4,
    /** @brief Chân vật lý 15: Cổng Port C - Pin 5 (PC5). Chức năng: GPIO / ADC CH3 / SPI SCK */
    MCU_PIN15= (1 << 4) | 5,
    /** @brief Chân vật lý 16: Cổng Port C - Pin 6 (PC6). Chức năng: GPIO / TIM1 CH1 / SPI MOSI */
    MCU_PIN16= (1 << 4) | 6,
    /** @brief Chân vật lý 17: Cổng Port C - Pin 7 (PC7). Chức năng: GPIO / TIM1 CH2 / SPI MISO */
    MCU_PIN17= (1 << 4) | 7,
    /** @brief Chân vật lý 18: Cổng Port D - Pin 1 (PD1). Chức năng: GPIO / SWDIO (Chân nạp mặc định) */
    MCU_PIN18= (2 << 4) | 1,
    /** @brief Chân vật lý 19: Cổng Port D - Pin 2 (PD2). Chức năng: GPIO / TIM1 CH2 / UART TX (Remap) */
    MCU_PIN19= (2 << 4) | 2,
    /** @brief Chân vật lý 20: Cổng Port D - Pin 3 (PD3). Chức năng: GPIO / TIM1 CH3 / UART RX (Remap) */
    MCU_PIN20= (2 << 4) | 3
#endif
} MCU_Pin_t;

// ============================================================================
// ĐỊNH NGHĨA CHÂN NGOẠI VI ĐẶC BIỆT (MCU_SpecialPin_t)
// ============================================================================

/**
 * @brief Định nghĩa các chân chức năng đặc biệt của hệ thống (I2C, UART, SPI, ADC, v.v.).
 */
typedef enum
{
    /** @brief [GIAO TIẾP] Chân nạp chương trình Single-wire (SWIO).
     * - Vị trí: PD1 trên mọi gói vỏ (SOP8: Chân 8 | SOP16: Chân 7 | TSSOP20: Chân 18).
     */
    MCU_SWIO = (2 << 4) | 1,

    /** @brief [GIAO TIẾP] Chân truyền dữ liệu I2C SDA.
     * - Vị trí: PC1 trên mọi gói vỏ (SOP8: Chân 5 | SOP16: Chân 1 | TSSOP20: Chân 11).
     */
    MCU_I2C_SDA = (1 << 4) | 1,

    /** @brief [GIAO TIẾP] Chân tạo xung nhịp I2C SCL.
     * - Vị trí: PC2 trên mọi gói vỏ (SOP8: Chân 6 | SOP16: Chân 2 | TSSOP20: Chân 12).
     */
    MCU_I2C_SCL = (1 << 4) | 2,

    /** @brief [GIAO TIẾP] Chân truyền tín hiệu UART TX.
     * - Vị trí: PD5 trên mọi gói vỏ (SOP8: Chân 8-Remap | SOP16: Chân 9 | TSSOP20: Chân 2).
     */
    MCU_UART_TX = (2 << 4) | 5,

    /** @brief [GIAO TIẾP] Chân nhận tín hiệu UART RX.
     * - Vị trí: PD6 trên mọi gói vỏ (SOP8: Chân 1-Remap | SOP16: Chân 10 | TSSOP20: Chân 3).
     */
    MCU_UART_RX = (2 << 4) | 6,

    /** @brief [GIAO TIẾP] Chân chọn chip ngoại vi SPI NSS.
     * - Vị trí: PC1 trên mọi gói vỏ (SOP8: Chân 5 | SOP16: Chân 1 | TSSOP20: Chân 11).
     */
    MCU_SPI_NSS = (1 << 4) | 1,

    /** @brief [NGOẠI VI ANALOG] Kênh chuyển đổi ADC CH5.
     * - Vị trí: PD4 trên mọi gói vỏ (SOP8: Chân 8 | SOP16: Chân 8 | TSSOP20: Chân 1).
     */
    MCU_ADC_CH5 = (2 << 4) | 4,

    /** @brief [NGOẠI VI ANALOG] Kênh chuyển đổi ADC CH7.
     * - Vị trí: PC1 trên mọi gói vỏ (SOP8: Chân 5 | SOP16: Chân 1 | TSSOP20: Chân 11).
     */
    MCU_ADC_CH7 = (1 << 4) | 1,

    /** @brief [NGOẠI VI ANALOG] Kênh chuyển đổi ADC CH8.
     * - Vị trí: PC2 trên mọi gói vỏ (SOP8: Chân 6 | SOP16: Chân 2 | TSSOP20: Chân 12).
     */
    MCU_ADC_CH8 = (1 << 4) | 2,

#if defined(CH32V003_A4M6) || defined(CH32V003_F4P6)
    /** @brief [HỆ THỐNG] Chân Reset cứng (NRST).
     * - Vị trí: PD7 (SOP16: Chân 11 | TSSOP20: Chân 4). Không có trên bản SOP8.
     */
    MCU_NRST= (2 << 4) | 7,

    /** @brief [GIAO TIẾP] Chân xuất dữ liệu ngoại vi SPI MOSI.
                                 * - Vị trí: PC6 (SOP16: Chân 5 | TSSOP20: Chân 16). Không có trên bản SOP8.
                                 */
    MCU_SPI_MOSI= (1 << 4) | 6,

    /** @brief [GIAO TIẾP] Chân nhận dữ liệu ngoại vi SPI MISO.
                                 * - Vị trí: PC7 (SOP16: Chân 6 | TSSOP20: Chân 17). Không có trên bản SOP8.
                                 */
    MCU_SPI_MISO= (1 << 4) | 7,

    /** @brief [NGOẠI VI ANALOG] Kênh chuyển đổi ADC CH0.
                                 * - Vị trí: PA2 (SOP16: Chân 13 | TSSOP20: Chân 6).
                                 */
    MCU_ADC_CH0= (0 << 4) | 2,

    /** @brief [NGOẠI VI ANALOG] Kênh chuyển đổi ADC CH1.
                                 * - Vị trí: PA1 (SOP16: Chân 12 | TSSOP20: Chân 5).
                                 */
    MCU_ADC_CH1= (0 << 4) | 1,

    /** @brief [NGOẠI VI ANALOG] Kênh chuyển đổi ADC CH2.
                                 * - Vị trí: PC4 (SOP16: Chân 4 | TSSOP20: Chân 14).
                                 */
    MCU_ADC_CH2= (1 << 4) | 4,

    /** @brief [NGOẠI VI ANALOG] Kênh chuyển đổi ADC CH6.
                                 * - Vị trí: PD5 (SOP16: Chân 9 | TSSOP20: Chân 2).
                                 */
    MCU_ADC_CH6= (2 << 4) | 5,
#endif

#if defined(CH32V003_F4P6)
    /** @brief [GIAO TIẾP] Chân tạo xung nhịp đồng bộ SPI SCK.
     * - Vị trí: PC5 (Chỉ khả dụng trên cấu hình đầy đủ TSSOP20: Chân 15).
     */
    MCU_SPI_SCK= (1 << 4) | 5,

    /** @brief [NGOẠI VI ANALOG] Kênh chuyển đổi ADC CH3.
                                 * - Vị trí: PC5 (Chỉ khả dụng trên cấu hình đầy đủ TSSOP20: Chân 15).
                                 */
    MCU_ADC_CH3= (1 << 4) | 5,

    /** @brief [NGOẠI VI ANALOG] Kênh chuyển đổi ADC CH4.
                                 * - Vị trí: PC3 (Chỉ khả dụng trên cấu hình đầy đủ TSSOP20: Chân 13).
                                 */
    MCU_ADC_CH4= (1 << 4) | 3,
#endif

    // --- TIỆN ÍCH PHẦN CỨNG SẴN CÓ TRÊN BOARD PHÁT TRIỂN ---
#if defined(CH32V003_F4P6)
    /** @brief Chân đấu nối đèn LED tích hợp (Built-in) trên kit phát triển Twen32F003 (PD1). */
    LED_BUILTIN_TWEN32F003= (2 << 4) | 1,
    /** @brief Chân đấu nối Nút nhấn tích hợp (Built-in) trên kit phát triển Twen32F003 (PC0). */
    BTN_BUILTIN_TWEN32F003= (1 << 4) | 0
#endif
} MCU_SpecialPin_t;

#endif // CH32V003_GPIO_PINS_H

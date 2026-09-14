/**
 * @file ch32v003_gpio_types.h
 * @author Vọc Vạch IoT
 * @brief Định nghĩa các kiểu dữ liệu, enum trạng thái logic và chế độ hoạt động cho GPIO CH32V003.
 * @version 2.0.0
 * @date 2026-06-07
 */

#ifndef CH32V003_GPIO_TYPES_H
#define CH32V003_GPIO_TYPES_H

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

#endif // CH32V003_GPIO_TYPES_H

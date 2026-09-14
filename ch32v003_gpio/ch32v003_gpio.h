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
#include "ch32v003_gpio_types.h"
#include "ch32v003_gpio_pins.h"

// ============================================================================
// MACRO HELPER HỖ TRỢ CHUYỂN ĐỔI CHÂNMCU TỰ ĐỘNG (EXECUTION HELPERS)
// ============================================================================

/**
 * @brief Macro Helper tự động giải mã mcuPin và thực thi hàm thao tác Port (Dùng cho hàm trả về void).
 * @param mcuPin Chân vật lý MCU (Thuộc @ref MCU_Pin_t hoặc @ref MCU_SpecialPin_t).
 * @param func Tên hàm xử lý cấp Port nhận tham số (GPIO_TypeDef*, uint8_t pinNumber, ...).
 * @param ... Các tham số phụ tùy chọn truyền thêm vào hàm `func` (nếu có).
 * @note Macro sử dụng khối lệnh `do { ... } while(0)` an toàn trong mọi ngữ cảnh điều kiện (if/else).
 */
#define EXECUTE_ON_PIN(mcuPin, func, ...) \
    do { \
        GPIO_TypeDef* _GPIOx; \
        uint8_t _pinNumber; \
        decodeHardwarePin((mcuPin), &_GPIOx, &_pinNumber); \
        func(_GPIOx, _pinNumber, ##__VA_ARGS__); \
    } while(0)

/**
 * @brief Macro Helper tự động giải mã mcuPin, thực thi hàm thao tác Port và trả về kết quả (Dùng cho hàm có giá trị return).
 * @param mcuPin Chân vật lý MCU (Thuộc @ref MCU_Pin_t hoặc @ref MCU_SpecialPin_t).
 * @param func Tên hàm xử lý cấp Port nhận tham số (GPIO_TypeDef*, uint8_t pinNumber, ...) và có giá trị trả về.
 * @param ... Các tham số phụ tùy chọn truyền thêm vào hàm `func` (nếu có).
 * @return Giá trị trả về từ kết quả thực thi của hàm `func`.
 * @note Macro sử dụng cú pháp Statement Expression `({ ... })` mở rộng của GCC/Clang.
 */
#define EXECUTE_ON_PIN_RET(mcuPin, func, ...) \
    ({ \
        GPIO_TypeDef* _GPIOx; \
        uint8_t _pinNumber; \
        decodeHardwarePin((mcuPin), &_GPIOx, &_pinNumber); \
        func(_GPIOx, _pinNumber, ##__VA_ARGS__); \
    })

// ============================================================================
// NGUYÊN MẪU CÁC HÀM XỬ LÝ (API FUNCTIONS)
// Cấu trúc phân tách logic rõ ràng không sử dụng Macro thay thế thô
// ============================================================================

/**
 * @brief Hàm chuyển đổi mã hóa chân vật lý MCU thành con trỏ Port GPIO và số pin tương ứng.
 * @param mcuPin Chân vật lý được mã hóa theo cấu trúc (Port << 4) | PinNumber.
 * @param GPIOx Pointer trả về con trỏ quản lý vùng nhớ Port GPIO tương ứng (GPIOA, GPIOC, GPIOD).
 * @param pinNumber Giá trị trả về số thứ tự bit của chân trong Port (0-7) được giải mã từ mcuPin.
 * @warning Hàm này chỉ giải mã chân vật lý được định nghĩa trong @ref MCU_Pin_t hoặc @ref MCU_SpecialPin_t
 */
void decodeHardwarePin(uint8_t mcuPin, GPIO_TypeDef** GPIOx, uint8_t* pinNumber);

/**
 * @brief Cấu hình chế độ hoạt động cho chân vật lý MCU (Kiểu hàm Arduino).
 * @param mcuPin Chân vật lý được chọn từ bảng mã hóa @ref MCU_Pin_t hoặc @ref MCU_SpecialPin_t.
 * @param mode Chế độ hoạt động mong muốn lấy từ cấu trúc danh mục @ref PinMode_t.
 */
void pinMode(uint8_t mcuPin, PinMode_t mode);

/**
 * @brief Ghi mức logic HIGH hoặc LOW trực tiếp ra chân vật lý MCU.
 * @param mcuPin Chân vật lý được chọn từ bảng mã hóa @ref MCU_Pin_t hoặc @ref MCU_SpecialPin_t.
 * @param state Trạng thái logic cần ghi truyền vào từ cấu trúc @ref DigitalState_t.
 */
void digitalWrite(uint8_t mcuPin, DigitalState_t state);

/**
 * @brief Đảo ngược tức thời trạng thái logic hiện tại của chân vật lý MCU.
 * @param mcuPin Chân vật lý được chọn từ bảng mã hóa @ref MCUPin_t hoặc @ref MCU_SpecialPin_t.
 */
void digitalToggle(uint8_t mcuPin);

/**
 * @brief Đọc giá trị mức logic hiện tại của chân vật lý được cấu hình làm đầu vào.
 * @param mcuPin Chân vật lý được chọn từ bảng mã hóa @ref MCUPin_t hoặc @ref MCU_SpecialPin_t.
 * @return Mức logic hiện trạng đọc về từ thanh ghi cổng (Thuộc @ref DigitalState_t).
 */
DigitalState_t digitalRead(uint8_t mcuPin);

// --- HỆ THỐNG CÁC HÀM QUẢN LÝ TIÊN TIẾN CHUYÊN DỤNG CHO CHÂN NẠP SWIO ---

/**
 * @brief Thiết lập chế độ hoạt động trực tiếp cho chân nạp chương trình SWIO (PD1).
 * @param swioMode Chế độ thiết lập được cấu hình dựa trên @ref SwioMode_t.
 * @warning Việc chuyển sang chế độ @ref SWIO_MODE_GPIO sẽ vô hiệu hóa tính năng nạp chương trình tạm thời cho đến khi chip được đặt lại hoặc cấu hình trả lại chế độ gỡ lỗi.
 */
void setSwioMode(SwioMode_t swioMode);

/**
 * @brief Đọc trạng thái cấu hình hiện tại của chân gỡ lỗi / nạp chương trình SWIO (PD1).
 * @return Chế độ hoạt động hiện tại thu thập từ phần cứng (Trả về @ref SwioMode_t).
 */
SwioMode_t getSwioMode(void);

/**
 * @brief Thực hiện đảo trạng thái chức năng chân SWIO tự động từ chế độ GPIO sang DEBUG hoặc ngược lại.
 */
void toggleSwioMode(void);

// --- CÁC HÀM TRUY XUẤT TRỰC TIẾP THEO PORT THẤP CẤP ---

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

/**
 * @file ch32v003_ir.h
 * @author Vọc Vạch IoT
 * @brief Thư viện thu phát hồng ngoại đa giao thức (NEC, Sony, Samsung) cho CH32V003.
 * @version 2.0
 * @date 2026-06-07
 */

#ifndef CH32V003_IR_H
#define CH32V003_IR_H

#include "ch32fun.h"
#include "sys_config.h"

#if (IR_ENABLE)
/** @brief Cấu hình chung hệ thống */
#ifndef IR_TIMEOUT_US
#define IR_TIMEOUT_US             15000 /**< Thời gian chờ tối đa chặn treo chip (15ms) */
#endif

#ifndef IR_TOLERANCE_PERCENT
#define IR_TOLERANCE_PERCENT  25    /**< Dung sai cho phép khi so khớp thời gian xung (25%) */
#endif

#ifndef IR_RAW_BUFFER_SIZE
#define IR_RAW_BUFFER_SIZE    256 /**< Số lượng sườn xung (Edges) tối đa có thể bắt được trong một khung truyền. */
#endif

#ifndef IR_MAX_SUPPORTED_BITS
#define IR_MAX_SUPPORTED_BITS 80 /**< Số lượng BIT dữ liệu tối đa mà thư viện có thể giải mã và quản lý. */
#endif

#define IR_RAW_DATA_BYTES        ((IR_MAX_SUPPORTED_BITS + 7) / 8) /**< Tự động tính toán số BYTE cần thiết trong struct IR_Data_t (Chia tròn lên). */



// ============================================================================
// ĐỊNH NGHĨA DANH MỤC GIAO THỨC & CẤU TRÚC DỮ LIỆU
// ============================================================================

/**
 * @brief Danh sách các giao thức hồng ngoại (IR Protocols) được thư viện hỗ trợ.
 */
typedef enum
{
    /** @brief Chưa xác định hoặc tín hiệu không hợp chuẩn giao thức nào. */
    IR_UNKNOWN = 0,

    /** * @brief Giao thức NEC (Mặc định phổ biến nhất).
     * - Tần số sóng mang: 38 kHz.
     * - Cấu trúc: 8-bit Address, 8-bit Command (Kèm bit đảo logic kiểm tra lỗi).
     */
    IR_NEC = 1,

    /** * @brief Giao thức SONY (SIRC).
     * - Tần số sóng mang: 40 kHz.
     * - Cấu trúc: Thường là 12-bit, 15-bit, hoặc 20-bit (7-bit Command, còn lại là Address).
     */
    IR_SONY = 2,

    /** * @brief Giao thức SAMSUNG.
     * - Tần số sóng mang: 38 kHz.
     * - Cấu trúc: Thường biến thể 16-bit Address và 16-bit Command.
     */
    IR_SAMSUNG = 3,

    /** * @brief Giao thức TCL (Smart TV Remote).
     * - Tần số sóng mang: 38 kHz.
     * - Khung truyền dữ liệu kỷ lục: 74 bit (Khác biệt hoàn toàn so với chuẩn NEC 32-bit).
     * - Mã hóa: Theo phương pháp khoảng cách xung (Pulse Distance Width).
     * + Leader: Mark 4000 us | Space 4000 us (Tỉ lệ 1:1).
     * + Bit 0 : Mark 560 us  | Space 1000 us.
     * + Bit 1 : Mark 560 us  | Space 2000 us (Khoảng lặng dài gấp đôi).
     * + Stop  : Mark 560 us để chốt khung.
     */
    IR_TCL = 4,
} IR_Protocol_t;

/**
 * @brief Định nghĩa kiểu dữ liệu thời gian xung (Mark/Space)
 */
typedef struct
{
    uint16_t mark; /**< Độ rộng xung phát mức CAO (us) */
    uint16_t space; /**< Độ rộng khoảng lặng mức THẤP (us) */
} IR_Pulse_t;

/**
 * @brief Kiểu mã hóa bit của giao thức (Bắt buộc phải phân loại vì Sony khác NEC)
 */
typedef enum
{
    IR_ENCODE_DISTANCE, /**< Mã hóa bằng độ rộng khoảng lặng - Space Distance (NEC, Samsung, TCL) */
    IR_ENCODE_WIDTH /**< Mã hóa bằng độ rộng xung vuông - Mark Width (Sony) */
} IR_EncodeType_t;

/**
 * @brief Cấu trúc định nghĩa thông số kỹ thuật cốt lõi của một giao thức IR.
 * @note Lưu hoàn toàn trong Flash nhờ từ khóa const.
 */
typedef struct
{
    IR_Protocol_t protocol; /**< Định danh enum giao thức */
    const char* name; /**< Chuỗi tên giao thức (phục vụ việc Debug/In tên) */
    uint16_t carrierFreqHz; /**< Tần số sóng mang (Ví dụ: 38000) */
    IR_Pulse_t leader; /**< Thông số xung mở đầu (Leader) */
    IR_Pulse_t bit0; /**< Thông số định nghĩa logic 0 */
    IR_Pulse_t bit1; /**< Thông số định nghĩa logic 1 */
    uint16_t stopMark; /**< Xung kết thúc gói tin (us) */
    uint8_t totalBits; /**< Tổng số lượng bit của giao thức (Hỗ trợ tối đa 64 bit, ví dụ TLC = 48) */
    IR_EncodeType_t encodeType;
} IR_ProtocolConfig_t;

/**
 * @brief Cấu trúc dữ liệu chứa thông tin gói tin IR giải mã được.
 */
typedef struct
{
    IR_Protocol_t protocol; /**< Loại giao thức giải mã thành công */
    uint8_t rawData[IR_RAW_DATA_BYTES]; /**< */
    uint16_t address; /**< Địa chỉ định danh thiết bị (Address) */
    uint32_t command; /**< Mã lệnh thực thi / Dữ liệu nút bấm (Command / Data) */
    uint8_t bits; /**< Số lượng bit thực tế của gói tin nhận được */
} IR_Data_t;

/**
 * @brief Cấu trúc dữ liệu chứa mảng xung thô (Raw Buffer) cho tính năng học lệnh.
 */
typedef struct
{
    uint16_t rawBuf[IR_RAW_BUFFER_SIZE]; /**< Mảng lưu độ rộng thời gian của các khoảng tick xung (us) */
    uint8_t rawLen; /**< Tổng số lượng sườn xung đã ghi nhận vào bộ đệm */
} IR_RawData_t;

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

void irAnalyzeRaw(const IR_RawData_t* rawData);

void irPrintResult(const IR_Data_t* irData);

#endif // IR_ENABLE

#endif // CH32V003_IR_H

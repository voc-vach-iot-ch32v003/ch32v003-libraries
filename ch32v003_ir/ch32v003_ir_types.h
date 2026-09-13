/**
 * @file ch32v003_ir_types.h
 * @author Vọc Vạch IoT
 * @brief Định nghĩa các enum, struct và kiểu dữ liệu quản lý giao thức hồng ngoại.
 * @version 2.0
 * @date 2026-06-07
 */

#ifndef CH32V003_IR_TYPES_H
#define CH32V003_IR_TYPES_H

#include <stdint.h>
#include "ch32v003_ir_defs.h"

#if (IR_ENABLE)

// ============================================================================
// ĐỊNH NGHĨA DANH MỤC GIAO THỨC & CẤU TRÚC DỮ LIỆU
// ============================================================================

/**
 * @brief Định nghĩa ID cho các giao thức hồng ngoại được thư viện hỗ trợ.
 * @note Dùng để map giữa dữ liệu động trong RAM và bảng tra cứu tĩnh trong Flash.
 */
typedef enum
{
    IR_UNKNOWN = 0,
    IR_NEC = 1,
    IR_SONY = 2,
    IR_SAMSUNG = 3,
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
 * @brief Định nghĩa chiều dịch bit (Bit Order) của giao thức.
 */
typedef enum
{
    IR_ORDER_LSB_FIRST = 0, /**< Bit thấp (Bit 0) được gửi/nhận trước (Mặc định: NEC, Samsung, TCL...) */
    IR_ORDER_MSB_FIRST = 1 /**< Bit cao (Bit 7) được gửi/nhận trước (Một số dòng tinh chỉnh đặc biệt) */
} IR_BitOrder_t;

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
    uint8_t totalBits; /**< Tổng số lượng bit của giao thức (Hỗ trợ tối đa 64 bit, ví dụ TCL = 48) */
    IR_EncodeType_t encodeType; /**< Kiểu mã hóa vật lý (Distance hay Width) */
    IR_BitOrder_t bitOrder; /**< Chiều dịch bit truyền thông của hãng */
} IR_ProtocolConfig_t;

/**
 * @brief Cấu trúc dữ liệu chứa thông tin gói tin IR giải mã được.
 */
typedef struct
{
    IR_Protocol_t protocol; /**< Loại giao thức giải mã thành công */
    uint8_t rawData[IR_RAW_DATA_BYTES]; /**< Bộ đệm lưu dữ liệu byte thô sau khi giải mã */
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

#endif // IR_ENABLE

#endif // CH32V003_IR_TYPES_H

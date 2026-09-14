/**
* @file ch32v003_max7219_defs.h
 * @author Vọc Vạch IoT
 * @brief Định nghĩa các địa chỉ thanh ghi và macro cho IC MAX7219.
 * @version 1.0.0
 * @date 2026-09-13
 */

#ifndef CH32V003_MAX7219_DEFS_H
#define CH32V003_MAX7219_DEFS_H

// ============================================================================
// ĐỊNH NGHĨA THANH GHI MAX7219 (REGISTER MAP)
// ============================================================================
#define MAX7219_REG_NOOP         0x00
#define MAX7219_REG_DIGIT0       0x01
#define MAX7219_REG_DIGIT1       0x02
#define MAX7219_REG_DIGIT2       0x03
#define MAX7219_REG_DIGIT3       0x04
#define MAX7219_REG_DIGIT4       0x05
#define MAX7219_REG_DIGIT5       0x06
#define MAX7219_REG_DIGIT6       0x07
#define MAX7219_REG_DIGIT7       0x08
#define MAX7219_REG_DECODE_MODE  0x09
#define MAX7219_REG_INTENSITY    0x0A
#define MAX7219_REG_SCAN_LIMIT   0x0B
#define MAX7219_REG_SHUTDOWN     0x0C
#define MAX7219_REG_DISPLAY_TEST 0x0F

// ============================================================================
// CÁC GIÁ TRỊ CẤU HÌNH MẶC ĐỊNH
// ============================================================================
#define MAX7219_SHUTDOWN_MODE    0x00
#define MAX7219_NORMAL_MODE      0x01

#define MAX7219_SCAN_ALL_DIGITS  0x07 // Scan Limit 0..7 (tất cả 8 digit)
#define MAX7219_NO_DECODE        0x00 // Tắt Code B decode (Chế độ Raw Matrix)

#define MAX7219_INTENSITY_MIN    0x00
#define MAX7219_INTENSITY_MAX    0x0F
#define MAX7219_INTENSITY_DEFAULT 0x05

#endif // CH32V003_MAX7219_DEFS_H

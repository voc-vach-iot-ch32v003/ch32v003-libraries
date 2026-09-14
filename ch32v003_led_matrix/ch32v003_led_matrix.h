/**
 * @file ch32v003_led_matrix.h
 * @author Vọc Vạch IoT
 * @brief Thư viện điều khiển LED Matrix 8x8 lớp đồ họa cao cấp cho CH32V003.
 * @version 1.0.0
 * @date 2026-09-13
 */

#ifndef CH32V003_LED_MATRIX_H
#define CH32V003_LED_MATRIX_H

#include "ch32fun.h"
#include "ch32v003_max7219.h"
#include "ch32v003_led_matrix_types.h"

// ============================================================================
// KHAI BÁO HÀM ĐIỀU KHIỂN LED MATRIX (CAMELCASE)
// ============================================================================

/**
 * @brief Khởi tạo hệ thống LED Matrix.
 * @param dev Con trỏ cấu hình led_matrix_t.
 * @param buffer Mảng RAM đệm do người dùng cấp phát (kích thước = matrixCount * 8).
 * @param matrixCount Số lượng module 8x8 ghép nối tiếp.
 * @param csPin Chân CS điều khiển.
 */
void ledMatrixInit(LedMatrix_t* dev, uint8_t* buffer, uint8_t matrixCount, MCU_Pin_t csPin);

/**
 * @brief Xóa toàn bộ dữ liệu trong Frame Buffer (không tự động đẩy lên màn hình).
 * @param dev Con trỏ cấu hình led_matrix_t.
 */
void ledMatrixClearBuffer(LedMatrix_t* dev);

/**
 * @brief Đẩy toàn bộ dữ liệu từ Frame Buffer trong RAM ra màn hình LED Matrix qua SPI.
 * @param dev Con trỏ cấu hình led_matrix_t.
 */
void ledMatrixUpdate(LedMatrix_t* dev);

/**
 * @brief Cài đặt góc xoay toàn màn hình.
 * @param dev Con trỏ cấu hình led_matrix_t.
 * @param rotation Góc xoay (MATRIX_ROTATE_0, MATRIX_ROTATE_90, MATRIX_ROTATE_180, MATRIX_ROTATE_270).
 */
void ledMatrixSetRotation(LedMatrix_t* dev, MatrixRotation_t rotation);

/**
 * @brief Cài đặt hướng cuộn chữ/hiệu ứng mặc định.
 * @param dev Con trỏ cấu hình led_matrix_t.
 * @param direction Hướng cuộn (MATRIX_SCROLL_LEFT, MATRIX_SCROLL_RIGHT, MATRIX_SCROLL_UP, MATRIX_SCROLL_DOWN).
 */
void ledMatrixSetScrollDirection(LedMatrix_t* dev, MatrixScrollDirection_t direction);

/**
 * @brief Cài đặt độ sáng hiển thị (0 .. 15).
 * @param dev Con trỏ cấu hình led_matrix_t.
 * @param brightness Độ sáng (0 .. 15).
 */
void ledMatrixSetBrightness(LedMatrix_t* dev, uint8_t brightness);

/**
 * @brief Vẽ hoặc xóa 1 điểm ảnh (Pixel) tại tọa độ (x, y).
 * @param dev Con trỏ cấu hình led_matrix_t.
 * @param x Tọa độ X (0 .. matrixCount * 8 - 1).
 * @param y Tọa độ Y (0 .. 7).
 * @param state true: Bật điểm LED, false: Tắt điểm LED.
 */
void ledMatrixDrawPixel(LedMatrix_t* dev, int16_t x, int16_t y, bool state);

/**
 * @brief Vẽ một đoạn thẳng từ (x0, y0) tới (x1, y1) bằng thuật toán Bresenham.
 * @param dev Con trỏ cấu hình led_matrix_t.
 * @param x0 Tọa độ X điểm đầu.
 * @param y0 Tọa độ Y điểm đầu.
 * @param x1 Tọa độ X điểm cuối.
 * @param y1 Tọa độ Y điểm cuối.
 * @param state true: Bật điểm LED, false: Tắt điểm LED.
 */
void ledMatrixDrawLine(LedMatrix_t* dev, int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool state);

/**
 * @brief Vẽ một ký tự ASCII (font 5x7) tại tọa độ góc trên trái (x, y).
 * @param dev Con trỏ cấu hình led_matrix_t.
 * @param x Tọa độ X (0 .. matrixCount * 8 - 1).
 * @param y Tọa độ Y (0 .. 7).
 * @param ch Ký tự ASCII cần vẽ.
 * @param state true: Bật điểm LED, false: Tắt điểm LED.
 */
void ledMatrixDrawChar(LedMatrix_t* dev, int16_t x, int16_t y, char ch, bool state);

/**
 * @brief Vẽ một chuỗi ký tự cố định tại tọa độ (x, y).
 * @param dev Con trỏ cấu hình led_matrix_t.
 * @param x Tọa độ X (0 .. matrixCount * 8 - 1).
 * @param y Tọa độ Y (0 .. 7).
 * @param str Chuỗi ký tự cần vẽ.
 * @param state true: Bật điểm LED, false: Tắt điểm LED.
 */
void ledMatrixDrawString(LedMatrix_t* dev, int16_t x, int16_t y, const char* str, bool state);

/**
 * @brief Tạo hiệu ứng cuộn chữ trên màn hình LED Matrix.
 * @param dev Con trỏ cấu hình led_matrix_t.
 * @param str Chuỗi ký tự cần cuộn.
 * @param y Tọa độ Y (0 .. 7).
 * @param state true: Bật điểm LED, false: Tắt điểm LED.
 */
void ledMatrixScrollTextAnimation(LedMatrix_t* dev, const char* str, int16_t y, bool state);

#endif // CH32V003_LED_MATRIX_H

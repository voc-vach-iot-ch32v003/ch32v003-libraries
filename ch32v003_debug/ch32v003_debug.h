/**
* @file ch32v003_debug.h
 * @author Vọc Vạch IoT
 * @brief Thư viện cấu hình và quản lý tính năng gỡ lỗi (Debug Printf) qua chân nạp cho CH32V003.
 * @version 1.0
 * @date 2026-05-26
 */

#ifndef CH32V003_DEBUG_H
#define CH32V003_DEBUG_H

#include "ch32fun.h"

// ============================================================================
// KHAI BÁO NGUYÊN MẪU HÀM NỘI BỘ (GIÚP VSCODE HẾT BÁO LỖI STDIO.H)
// ============================================================================

/**
 * @brief Hàm in dữ liệu định dạng siêu nhẹ của framework ch32v003fun.
 * @details Hàm này xuất dữ liệu trực tiếp qua chân nạp SWIO (PD1) lên máy tính thông qua mạch nạp WCH-Link.
 *          Hỗ trợ các định dạng cơ bản: %d, %x, %c, %s (Không hỗ trợ %f để tiết kiệm Flash).
 * @note KHÔNG ĐƯỢC include <stdio.h> để tránh làm phình to bộ nhớ Flash của vi điều khiển.
 * @param format Chuỗi định dạng dữ liệu truyền vào (tương tự printf tiêu chuẩn).
 * @param ... Các tham số biến đổi truyền kèm theo chuỗi định dạng.
 * @return Số lượng ký tự đã in ra thành công.
 */
int printf(const char* format, ...);

#endif // CH32V003_DEBUG_H

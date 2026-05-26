/**
* @file ch32v003_power.h
 * @author Vọc Vạch IoT
 * @brief Thư viện quản lý năng lượng thông minh cho CH32V003 (J4M6, A4M6, F4P6).
 * @version 1.2
 * @date 2026-05-26
 */

#ifndef CH32V003_POWER_H
#define CH32V003_POWER_H

#include "ch32fun.h"
#include "sys_config.h"

#if (POWER_ENABLE)

typedef enum
{
    WAKEUP_FALLING = 0,
    WAKEUP_RISING = 1,
    WAKEUP_CHANGE = 2
} WakeupTrigger_t;

// Macro bọc gọi theo kiểu chân Arduino
#define enableWakeupPin(mcu_pin, trigger)  enableWakeupPinPort(mcu_pin, trigger)

// ============================================================================
// KHAI BÁO CÁC HÀM CHỨC NĂNG CHÍNH
// ============================================================================

void enableWakeupPinPort(const GPIO_TypeDef* GPIOx, uint8_t pinNumber, WakeupTrigger_t trigger);

/**
 * @brief Chế độ ngủ nhẹ giữ RAM - Hạ xung nhịp (Dùng tốt cho cả 3 dòng J4M6, A4M6, F4P6).
 * @details Tỉnh dậy chạy tiếp dòng lệnh ngay phía dưới mà không bị mất dữ liệu.
 */
void sleepUltraLowPower(void);

#ifndef CH32V003_J4M6
/**
 * @brief Chế độ ngủ sâu cực hạn - Standby Mode (Khuyên dùng cho A4M6 và F4P6).
 * @details Ép dòng tiêu thụ về ~10uA. Toàn bộ lõi CPU và RAM bị ngắt điện.
 *          Khi chân Wakeup phần cứng (PD0) nhận mức HIGH, chip sẽ tỉnh dậy và RESET chạy lại từ đầu hàm main().
 */
void enterStandbyMode(void);
#endif

#endif // POWER_ENABLE

#endif // CH32V003_POWER_H

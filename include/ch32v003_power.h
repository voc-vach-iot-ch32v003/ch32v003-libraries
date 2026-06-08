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

// ============================================================================
// KHAI BÁO CÁC HÀM CHỨC NĂNG CHÍNH
// ============================================================================

/**
 * @brief Cấu hình một chân GPIO bất kỳ làm chân Wakeup để đánh thức chip khi ngủ sâu.
 * @details Tự động kích hoạt xung clock cho khối AFIO, cấu hình
 * @param GPIOx Cổng GPIO chứa chân Wakeup (GPIOA, GPIOC, hoặc GPIOD)
 * @param pinNumber Chỉ số chân (0-7) trên Port đó, tương ứng với EXTI Line 0-7
 * @param trigger Kiểu kích hoạt đánh thức: RISING, FALLING, hoặc CHANGE (cả 2 cạnh)
 */
void enableWakeupPinPort(const GPIO_TypeDef* GPIOx, uint8_t pinNumber, WakeupTrigger_t trigger);

/**
 * @brief Cấu hình một chân GPIO cụ thể làm chân Wakeup để đánh thức chip khi ngủ sâu.
 * @details Hàm tiện lợi cho người dùng không cần nhớ mã hóa Port và Line, chỉ cần truyền số chân vật lý trên board và kiểu kích hoạt.
 * @param mcu_pin Chỉ số chân trên board (Chọn từ @ref MCUPin_t hoặc @ref MCUSpecialPin_t) để làm chân Wakeup.
 * @param trigger Kiểu kích hoạt đánh thức: RISING, FALLING, hoặc CHANGE (cả 2 cạnh).
 * @warning Chỉ những chân vật lý có hỗ trợ ngắt ngoài (GPIOA0-7, GPIOC0-7, GPIOD0-7) mới có thể được cấu hình làm chân Wakeup. Việc chọn chân không hỗ trợ sẽ không có tác dụng và có thể gây ra lỗi khi chạy vào chế độ ngủ sâu. Vui lòng tham khảo tài liệu kỹ thuật của CH32V003 để biết danh sách chân hỗ trợ ngắt ngoài.
 */
void enableWakeupPin(uint8_t mcu_pin, WakeupTrigger_t trigger);

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

#include "ch32v003_ir.h"
#include "ch32v003_gpio.h"
#include "ch32v003_delay.h"
#include "ch32v003_timer.h"

#if (IR_ENABLE)
// ============================================================================
// CẤU HÌNH ĐỊNH NGHĨA HẰNG SỐ HỒNG NGOẠI (IR CONSTANTS)
// ============================================================================

/** @brief Cấu hình chung hệ thống */
#define IR_TIMEOUT_US             15000 /**< Thời gian chờ tối đa chặn treo chip (15ms) */
#define IR_CARRIER_HALF_PERIOD_US 13    /**< Nửa chu kỳ xung 38kHz (13us HIGH, 13us LOW) */
#define IR_CARRIER_FULL_PERIOD_US 26    /**< Một chu kỳ xung 38kHz (26us) */
#define IR_BIT_THRESHOLD_US       1200  /**< Ngưỡng phân biệt Bit 0 và Bit 1 (1.2ms) */

/** @brief Hằng số thời gian Giao thức NEC */
#define IR_NEC_LEADER_LOW_MIN     8000  /**< Xung mở đầu LOW tối thiểu */
#define IR_NEC_LEADER_LOW_MAX     10000 /**< Xung mở đầu LOW tối đa */
#define IR_NEC_LEADER_HIGH_MIN    4000  /**< Xung mở đầu HIGH tối thiểu */
#define IR_NEC_LEADER_HIGH_MAX    5000  /**< Xung mở đầu HIGH tối đa */
#define IR_NEC_LEADER_LOW_TX      9000  /**< Xung mở đầu LOW khi phát */
#define IR_NEC_LEADER_HIGH_TX     4500  /**< Xung mở đầu HIGH khi phát */
#define IR_NEC_BIT_MARK_TX        560   /**< Độ rộng xung mang khi phát bit */
#define IR_NEC_BIT_1_SPACE_TX     1690  /**< Khoảng lặng phát bit 1 */
#define IR_NEC_BIT_0_SPACE_TX     560   /**< Khoảng lặng phát bit 0 */

/** @brief Hằng số thời gian Giao thức SAMSUNG */
#define IR_SAMSUNG_LEADER_LOW_MIN  4000  /**< Xung mở đầu LOW tối thiểu */
#define IR_SAMSUNG_LEADER_LOW_MAX  5000  /**< Xung mở đầu LOW tối đa */
#define IR_SAMSUNG_LEADER_HIGH_MIN 4000  /**< Xung mở đầu HIGH tối thiểu */
#define IR_SAMSUNG_LEADER_HIGH_MAX 5000  /**< Xung mở đầu HIGH tối đa */
#define IR_SAMSUNG_LEADER_LOW_TX   4500  /**< Xung mở đầu LOW khi phát */
#define IR_SAMSUNG_LEADER_HIGH_TX  4500  /**< Xung mở đầu HIGH khi phát */
#define IR_SAMSUNG_BIT_MARK_TX     560   /**< Độ rộng xung mang khi phát bit */
#define IR_SAMSUNG_BIT_1_SPACE_TX  1690  /**< Khoảng lặng phát bit 1 */
#define IR_SAMSUNG_BIT_0_SPACE_TX  560   /**< Khoảng lặng phát bit 0 */

/** @brief Hằng số thời gian Giao thức SONY */
#define IR_SONY_LEADER_LOW_MIN    2000  /**< Xung mở đầu LOW tối thiểu */
#define IR_SONY_LEADER_LOW_MAX    2800  /**< Xung mở đầu LOW tối đa */
#define IR_SONY_BIT_THRESHOLD_US  1000  /**< Ngưỡng phân biệt bit 0 và 1 của Sony */
#define IR_SONY_LEADER_LOW_TX     2400  /**< Xung mở đầu LOW khi phát */
#define IR_SONY_LEADER_HIGH_TX    600   /**< Xung mở đầu HIGH khi phát */
#define IR_SONY_BIT_1_MARK_TX     1200  /**< Độ rộng xung mang khi phát bit 1 */
#define IR_SONY_BIT_0_MARK_TX     600   /**< Độ rộng xung mang khi phát bit 0 */
#define IR_SONY_SPACE_TX          600   /**< Khoảng lặng cố định giữa các bit */

// ============================================================================
// CÁC HÀM TRỢ GIÚP NỘI BỘ (HELPER FUNCTIONS)
// ============================================================================

/**
 * @brief Kiểm tra một giá trị có nằm trong khoảng [min, max] hay không.
 * @note Sử dụng static inline để trình biên dịch tự chèn code trực tiếp, tối ưu dung lượng.
 */
static inline uint8_t isBetween(const uint32_t value, const uint32_t min, const uint32_t max)
{
    return (value >= min && value <= max);
}

/**
 * @brief Đo độ rộng xung mức LOW hoặc HIGH.
 */
static uint32_t getPulseWidth(const GPIO_TypeDef* GPIOx, const uint8_t pinNumber, const uint8_t state)
{
    // Đổi hằng số thời gian giới hạn an toàn (15ms) từ us sang số lượng Ticks thô của CPU
    // Với chip 48MHz: 15000us * 48 ticks/us = 720,000 ticks
    const uint32_t timeoutTicks = IR_TIMEOUT_US * (FUNCONF_SYSTEM_CORE_CLOCK / 1000000);

    uint32_t startTime = ticks(); // Lấy mốc ticks thô xuất phát

    // 1. Chờ cho đến khi chân chuyển sang đúng trạng thái mong muốn
    while (digitalReadPort(GPIOx, pinNumber) != state)
    {
        // Phép trừ tự động xử lý tràn số thanh ghi đếm của CPU
        if ((ticks() - startTime) > timeoutTicks) return 0;
    }

    // 2. Đo khoảng thời gian chân duy trì trạng thái đó
    startTime = ticks(); // Reset lại mốc ticks để đo xung thực tế
    while (digitalReadPort(GPIOx, pinNumber) == state)
    {
        if ((ticks() - startTime) > timeoutTicks) break;
    }

    // Tính tổng số ticks thực tế đã trôi qua
    const uint32_t totalTicks = ticks() - startTime;

    // Quy đổi kết quả cuối cùng từ số Ticks thô sang đơn vị us để trả về cho hàm xử lý bit
    return totalTicks / (FUNCONF_SYSTEM_CORE_CLOCK / 1000000);
}

/**
 * @brief Phát một khoảng xung 38kHz (Modulation) ra chân GPIO bằng phần mềm.
 */
/**
 * @brief Phát sóng mang hồng ngoại chuẩn xác 38kHz sử dụng thanh ghi thô và ASM Delay.
 * @details Loại bỏ toàn bộ hàm bọc trung gian để đảm bảo tần số không bị lệch khỏi dải 38kHz.
 */
/**
 * @brief Phát sóng mang hồng ngoại chuẩn xác 38kHz bằng cách can thiệp thanh ghi và Delay_Us hệ thống.
 */
/**
 * @brief Phát sóng mang hồng ngoại 38kHz và bật LED indicator phụ để kiểm tra hoạt động.
 */
static void mark38kHz(GPIO_TypeDef* GPIOx, const uint8_t pinNumber, const uint32_t timeUs)
{
    const uint32_t cycles = timeUs / 26;

    for (uint32_t i = 0; i < cycles; i++)
    {
        digitalWritePort(GPIOx, pinNumber, HIGH); // Bật LED
        delayUs(IR_CARRIER_HALF_PERIOD_US);

        digitalWritePort(GPIOx, pinNumber, LOW); // Tắt LED
        delayUs(IR_CARRIER_HALF_PERIOD_US);
    }
}

/**
 * @brief Khoảng lặng không phát xung (Space).
 */
static void space(GPIO_TypeDef* GPIOx, const uint8_t pinNumber, const uint32_t timeUs)
{
    digitalWritePort(GPIOx, pinNumber, LOW); // Đảm bảo LED tắt
    delayUs(timeUs);
}

// ============================================================================
// TRIỂN KHAI HÀM ĐỌC GIẢI MÃ (IR_READ)
// ============================================================================
uint8_t irReadPort(const GPIO_TypeDef* GPIOx, const uint8_t pinNumber, IR_Data_t* irData)
{
    if (digitalReadPort(GPIOx, pinNumber) == HIGH)
    {
        return 0;
    }

    const uint32_t startLow = getPulseWidth(GPIOx, pinNumber, LOW);
    const uint32_t startHigh = getPulseWidth(GPIOx, pinNumber, HIGH);

    // --- KIỂM TRA GIAO THỨC NEC (9ms LOW, 4.5ms HIGH) ---
#if (IR_SUPPORT_NEC)
    if (isBetween(startLow, IR_NEC_LEADER_LOW_MIN, IR_NEC_LEADER_LOW_MAX) &&
        isBetween(startHigh, IR_NEC_LEADER_HIGH_MIN, IR_NEC_LEADER_HIGH_MAX))
    {
        uint32_t data = 0;
        for (uint8_t i = 0; i < 32; i++)
        {
            const uint32_t bitLow = getPulseWidth(GPIOx, pinNumber, LOW);
            const uint32_t bitHigh = getPulseWidth(GPIOx, pinNumber, HIGH);

            if (bitLow == 0 || bitHigh == 0) return 0;

            if (bitHigh > IR_BIT_THRESHOLD_US)
            {
                data |= (1UL << i);
            }
        }
        irData->protocol = IR_NEC;
        irData->bits = 32;
        irData->address = data & 0xFF;
        irData->command = (data >> 16) & 0xFF;
        return 1;
    }
#endif

    // --- KIỂM TRA GIAO THỨC SAMSUNG (4.5ms LOW, 4.5ms HIGH) ---
#if (IR_SUPPORT_SAMSUNG)
    if (isBetween(startLow, IR_SAMSUNG_LEADER_LOW_MIN, IR_SAMSUNG_LEADER_LOW_MAX) &&
        isBetween(startHigh, IR_SAMSUNG_LEADER_HIGH_MIN, IR_SAMSUNG_LEADER_HIGH_MAX))
    {
        uint32_t data = 0;
        for (uint8_t i = 0; i < 32; i++)
        {
            const uint32_t bitLow = getPulseWidth(GPIOx, pinNumber, LOW);
            const uint32_t bitHigh = getPulseWidth(GPIOx, pinNumber, HIGH);

            if (bitLow == 0 || bitHigh == 0) return 0;

            if (bitHigh > IR_BIT_THRESHOLD_US)
            {
                data |= (1UL << i);
            }
        }
        irData->protocol = IR_SAMSUNG;
        irData->bits = 32;
        irData->address = data & 0xFFFF;
        irData->command = (data >> 16) & 0xFF;
        return 1;
    }
#endif

    // --- KIỂM TRA GIAO THỨC SONY (2.4ms LOW, 600us HIGH) ---
#if (IR_SUPPORT_SONY)
    if (isBetween(startLow, IR_SONY_LEADER_LOW_MIN, IR_SONY_LEADER_LOW_MAX))
    {
        uint32_t data = 0;
        for (uint8_t i = 0; i < 12; i++)
        {
            if (i > 0)
            {
                getPulseWidth(GPIOx, pinNumber, HIGH);
            }

            const uint32_t bitLow = getPulseWidth(GPIOx, pinNumber, LOW);
            if (bitLow > IR_SONY_BIT_THRESHOLD_US)
            {
                data |= (1UL << i);
            }
        }
        irData->protocol = IR_SONY;
        irData->bits = 12;
        irData->command = data & 0x7F;
        irData->address = (data >> 7) & 0x1F;
        return 1;
    }
#endif

    return 0;
}

// ============================================================================
// TRIỂN KHAI HÀM PHÁT TÍN HIỆU (IR_SEND)
// ============================================================================
void irSendPort(GPIO_TypeDef* GPIOx, const uint8_t pinNumber, const IR_Protocol_t protocol, const uint16_t address,
                const uint32_t command)
{
#if (IR_SUPPORT_NEC)
    if (protocol == IR_NEC)
    {
        const uint32_t data = (address & 0xFF) | ((~(address) & 0xFF) << 8) |
            ((command & 0xFF) << 16) | ((~(command) & 0xFF) << 24);

        mark38kHz(GPIOx, pinNumber, IR_NEC_LEADER_LOW_TX);
        space(GPIOx, pinNumber, IR_NEC_LEADER_HIGH_TX);

        for (uint8_t i = 0; i < 32; i++)
        {
            mark38kHz(GPIOx, pinNumber, IR_NEC_BIT_MARK_TX);
            if (data & (1UL << i))
            {
                space(GPIOx, pinNumber, IR_NEC_BIT_1_SPACE_TX);
            }
            else
            {
                space(GPIOx, pinNumber, IR_NEC_BIT_0_SPACE_TX);
            }
        }
        mark38kHz(GPIOx, pinNumber, IR_NEC_BIT_MARK_TX); // Stop bit
    }
#endif

#if (IR_SUPPORT_SAMSUNG)
    if (protocol == IR_SAMSUNG)
    {
        const uint32_t data = (address & 0xFF) | (((address >> 8) & 0xFF) << 8) |
            ((command & 0xFF) << 16) | ((~(command) & 0xFF) << 24);

        mark38kHz(GPIOx, pinNumber, IR_SAMSUNG_LEADER_LOW_TX);
        space(GPIOx, pinNumber, IR_SAMSUNG_LEADER_HIGH_TX);

        for (uint8_t i = 0; i < 32; i++)
        {
            mark38kHz(GPIOx, pinNumber, IR_SAMSUNG_BIT_MARK_TX);
            if (data & (1UL << i))
            {
                space(GPIOx, pinNumber, IR_SAMSUNG_BIT_1_SPACE_TX);
            }
            else
            {
                space(GPIOx, pinNumber, IR_SAMSUNG_BIT_0_SPACE_TX);
            }
        }
        mark38kHz(GPIOx, pinNumber, IR_SAMSUNG_BIT_MARK_TX); // Stop bit
    }
#endif

#if (IR_SUPPORT_SONY)
    if (protocol == IR_SONY)
    {
        const uint16_t data = (command & 0x7F) | ((address & 0x1F) << 7);

        mark38kHz(GPIOx, pinNumber, IR_SONY_LEADER_LOW_TX);
        space(GPIOx, pinNumber, IR_SONY_LEADER_HIGH_TX);

        for (uint8_t i = 0; i < 12; i++)
        {
            if (data & (1 << i))
            {
                mark38kHz(GPIOx, pinNumber, IR_SONY_BIT_1_MARK_TX);
            }
            else
            {
                mark38kHz(GPIOx, pinNumber, IR_SONY_BIT_0_MARK_TX);
            }
            space(GPIOx, pinNumber, IR_SONY_SPACE_TX);
        }
    }
#endif
}

#endif // IR_ENABLE

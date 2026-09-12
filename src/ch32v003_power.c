#include "ch32v003_power.h"

#include "ch32v003_delay.h"
#include "ch32v003_gpio.h"

#if (POWER_ENABLE == 1)

static uint32_t apb1_clock_bak = 0;
static uint32_t apb2_clock_bak = 0;

// Biến toàn cục nội bộ: Tự động lưu và gộp tất cả các Port được người dùng chọn làm chân Wakeup
static uint32_t wakeup_ports_mask = RCC_APB2Periph_AFIO;

void enableWakeupPinPort(const GPIO_TypeDef* GPIOx, const uint8_t pinNumber, const WakeupTrigger_t trigger)
{
    // 1. Kích hoạt xung clock cho khối chức năng phụ trợ (AFIO) điều khiển ngắt ngoài
    RCC->APB2PCENR |= RCC_APB2Periph_AFIO;

    // Tự động cấu hình chân đó làm cổng vào có trở kéo
    if (trigger == WAKEUP_FALLING)
    {
        pinModePort((GPIO_TypeDef*)GPIOx, pinNumber, INPUT_PULLUP);
    }
    else
    {
        pinModePort((GPIO_TypeDef*)GPIOx, pinNumber, INPUT_PULLDOWN);
    }

    // Tự động tích lũy Port này vào mặt nạ xung nhịp để không bị tắt nhầm khi đi ngủ
    if (GPIOx == GPIOA) wakeup_ports_mask |= RCC_APB2Periph_GPIOA;
    else if (GPIOx == GPIOC) wakeup_ports_mask |= RCC_APB2Periph_GPIOC;
    else if (GPIOx == GPIOD) wakeup_ports_mask |= RCC_APB2Periph_GPIOD;

    // 2. Mã hóa PortCode chuẩn xác
    uint8_t portCode = 0;
    if (GPIOx == GPIOA) portCode = 0; // Port A = 00b
    else if (GPIOx == GPIOC) portCode = 2; // Port C = 10b
    else if (GPIOx == GPIOD) portCode = 3; // Port D = 11b

    uint32_t tempEXTICR = AFIO->EXTICR; // 1. Đọc giá trị hiện tại của thanh ghi ra
    tempEXTICR &= ~(0x03 << (pinNumber * 2)); // 2. Xóa sạch 2 bit quản lý của chân hiện tại
    tempEXTICR |= ((uint32_t)portCode << (pinNumber * 2)); // 3. Ghi mã Port mới vào 2 bit đó
    AFIO->EXTICR = tempEXTICR; // 4. Đè ngược giá trị hoàn chỉnh vào phần cứng

    // 3. Cấu hình khối EXTI (Giữ nguyên)
    EXTI->INTENR |= (1 << pinNumber);
    EXTI->FTENR &= ~(1 << pinNumber);
    EXTI->RTENR &= ~(1 << pinNumber);

    if (trigger == WAKEUP_FALLING || trigger == WAKEUP_CHANGE)
    {
        EXTI->FTENR |= (1 << pinNumber);
    }
    if (trigger == WAKEUP_RISING || trigger == WAKEUP_CHANGE)
    {
        EXTI->RTENR |= (1 << pinNumber);
    }

    // 4. Kích hoạt vector ngắt hợp nhất cho Line 0 -> Line 7 trong lõi RISC-V [ch32fun]
    NVIC_EnableIRQ(EXTI7_0_IRQn);

    // Kích hoạt ngắt toàn cục cho lõi RISC-V
    __enable_irq();
}

void enableWakeupPin(const uint8_t mcu_pin, const WakeupTrigger_t trigger)
{
    // Chuyển đổi số chân vật lý thành Port và PinNumber
    GPIO_TypeDef* GPIOx = NULL;
    uint8_t pinNumber = 0;

    decodeHardwarePin(mcu_pin, &GPIOx, &pinNumber);

    // Gọi hàm cấu hình chân Wakeup theo Port và PinNumber đã xác định
    enableWakeupPinPort(GPIOx, pinNumber, trigger);
}

void sleepUltraLowPower(void)
{
    // =================================================================
    // GIAI ĐOẠN 1: PRE-SLEEP (LƯU TRẠNG THÁI & XỬ LÝ CHÂN UNUSED)
    // =================================================================

    // 1.1. Lưu lại cấu hình Clock ngoại vi hiện tại
    apb1_clock_bak = RCC->APB1PCENR;
    apb2_clock_bak = RCC->APB2PCENR;

    // 1.2. Chờ UART/SWIO hoàn tất truyền dữ liệu
    delayMs(5);

    // 1.3. Vô hiệu hóa chân Debug SWIO (PD1) để tránh rò dòng qua WCH-Link
    // Note: Bit 26 trong AFIO->PCFR1 ngắt kết nối SWD
    AFIO->PCFR1 |= (1 << 26);

    // 1.4. Chỉ giữ Clock cho AFIO và các Port có ngắt EXTI
    RCC->APB1PCENR = 0;
    RCC->APB2PCENR = wakeup_ports_mask | RCC_APB2Periph_AFIO;


    // =================================================================
    // GIAI ĐOẠN 2: HẠ TẦN SỐ HỆ THỐNG VỀ 31.25 KHZ & TẮT PLL
    // =================================================================

    // 2.1. Chuyển SYSCLK về HSI (24MHz)
    RCC->CFGR0 &= ~(RCC_SW);
    RCC->CFGR0 |= RCC_SW_HSI;
    while ((RCC->CFGR0 & RCC_SWS) != RCC_SWS_HSI)
    {
    }

    // 2.2. Tắt khối PLL (Bit 24 = PLLON)
    RCC->CTLR &= ~(1 << 24);

    // 2.3. Tắt SysTick Timer để không làm tỉnh giấc CPU mỗi 1ms
    SysTick->CTLR &= ~1;

    // 2.4. Chia AHB Prescaler cho 256 -> Core Clock = 24MHz / 256 = 93.75kHz 
    // (Hoặc nếu HSI/2 = 12MHz / 256 = 46.87kHz / Tùy cấu hình HSI Prediv)
    RCC->CFGR0 &= ~(0xF << 4);
    RCC->CFGR0 |= (0x0F << 4); // AHB Prescaler /256
    __asm__("nop");
    __asm__("nop");

    // 2.5. Xóa sạch cờ ngắt EXTI tồn đọng tránh bị gỡ ngủ tức thì
    EXTI->INTFR = 0xFFFFFFFF;


    // =================================================================
    // GIAI ĐOẠN 3: THỰC THI LỆNH NGỦ SLEEP (WFI)
    // =================================================================

    // Đưa CPU vào chế độ Wait For Interrupt
    __asm__ volatile("wfi");


    // =================================================================
    // GIAI ĐOẠN 4: THỨC DẬY (WAKEUP) & KHÔI PHỤC TỐC ĐỘ CAO
    // =================================================================

    // 4.1. Trả bộ chia AHB Prescaler về 1 NGAY LẬP TỨC để CPU xử lý nhanh lệnh tiếp theo
    RCC->CFGR0 &= ~(0xF << 4);
    __asm__("nop");
    __asm__("nop");

    // 4.2. Bật lại khối PLL
    RCC->CTLR |= (1 << 24); // PLLON = 1
    while (!(RCC->CTLR & (1 << 25)))
    {
    }
    // Chờ PLL Ready (Lúc này CPU đã chạy nhanh nên chờ rất lẹ)

    // 4.3. Chuyển nguồn SYSCLK trở lại dùng PLL (48MHz)
    RCC->CFGR0 &= ~(RCC_SW);
    RCC->CFGR0 |= RCC_SW_PLL;
    while ((RCC->CFGR0 & RCC_SWS) != RCC_SWS_PLL)
    {
    }

    // 4.4. Bật lại SysTick
    SysTick->CTLR |= 1;


    // =================================================================
    // GIAI ĐOẠN 5: KHÔI PHỤC NGOẠI VI & DEBUG SWIO
    // =================================================================

    // 5.1. Khôi phục Clock ngoại vi ban đầu
    RCC->APB1PCENR = apb1_clock_bak;
    RCC->APB2PCENR = apb2_clock_bak;

    // 5.2. Mở lại chân Debug SWIO (PD1) để có thể nạp lại code bằng WCH-LinkE
    AFIO->PCFR1 &= ~(1 << 26);

    // 5.3. Trễ ngắn đồng bộ giao tiếp SWD
    delayMs(10);
}

// ============================================================================
// HÀM XỬ LÝ NGẮT PHẦN CỨNG HỢP NHẤT EXTI LINE 0 -> LINE 7 [ch32fun]
// ============================================================================
// ReSharper disable once CppUseInternalLinkage
void EXTI7_0_IRQHandler(void) __attribute__((interrupt));

void EXTI7_0_IRQHandler(void)
{
    // Tự động kiểm tra và quét sạch tất cả các cờ ngắt của các đường Line có thể xảy ra
    for (uint8_t i = 0; i < 8; i++)
    {
        if (EXTI->INTFR & (1 << i))
        {
            EXTI->INTFR = (1 << i); // Xóa cờ ngắt cho Line tương ứng
        }
    }
}

#ifndef CH32V003_J4M6
void enterStandbyMode(void)
{
    // 1. Cấp xung clock hệ thống cho khối quản lý nguồn PWR
    RCC->APB1PCENR |= RCC_APB1Periph_PWR;

    // 2. Kích hoạt tính năng chân Wakeup phần cứng chuyên dụng (PD0)
    PWR->AWUCSR |= (1 << 1);

    // 3. Xóa cờ báo hiệu thức giấc cũ (CWUF = Clear Wakeup Flag) để sẵn sàng nhận lệnh mới
    PWR->CTLR |= (1 << 2);

    // 4. Cấu hình bit PDDS (Power Down Deep Sleep) = 1 để ép chip chọn Standby thay vì Sleep
    PWR->CTLR |= PWR_CTLR_PDDS;

    // 5. Thiết lập bit SLEEPDEEP của lõi CPU RISC-V hệ thống
    // Trên dòng QingKe V2A, cấu trúc này được điều khiển gián tiếp qua lệnh WFI khi PDDS đã bật

    // 6. Thực thi lệnh hợp ngữ Wait For Interrupt (wfi) để đưa toàn bộ chip vào cõi hư vô
    __asm volatile("wfi");

    // Do ở chế độ Standby, khi có tín hiệu chân PD0, chip sẽ Reset phần cứng 
    // và nạp lại từ đầu hàm main(), nên code sẽ không bao giờ chạy xuống dưới dòng này.
}
#endif // CH32V003_J4M6
#endif

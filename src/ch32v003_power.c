#include "ch32v003_power.h"

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

void sleepUltraLowPower(void)
{
    // Xóa bỏ tham số truyền vào Port cũ, thư viện tự lo!
    // Sao lưu lại trạng thái bật/tắt clock hiện tại của toàn bộ ngoại vi
    apb1_clock_bak = RCC->APB1PCENR;
    apb2_clock_bak = RCC->APB2PCENR;

    // Đợi bộ đệm SWIO đẩy hết dữ liệu chuỗi ký tự cũ ra màn hình máy tính [ch32fun]
    Delay_Ms(5);

    // [BƯỚC 1]: Ngắt kết nối chân debug SWIO để triệt tiêu dòng rò phần cứng [ch32fun]
    AFIO->PCFR1 |= (1 << 26);

    // Tắt toàn bộ clock ngoại vi, CHỈ GIỮ LẠI khối AFIO và CÁC PORT chứa chân Wakeup đã cấu hình
    RCC->APB1PCENR = 0;
    RCC->APB2PCENR = wakeup_ports_mask;

    // [BƯỚC 2]: Hạ tần số hệ thống về HSI 8MHz trực tiếp
    RCC->CFGR0 &= ~(RCC_SW);
    RCC->CFGR0 |= RCC_SW_HSI;
    while ((RCC->CFGR0 & RCC_SWS) != RCC_SWS_HSI)
        RCC->CTLR &= ~(1 << 3); // Tắt bộ nhân tần PLL

    // Cấu hình bộ chia AHB Prescaler lên mức tối đa (Chia 256 -> Tần số ngủ = 31.25 kHz)
    RCC->CFGR0 &= ~(0xf << 4);
    RCC->CFGR0 |= (0x0f << 4);
    __asm__("nop");
    __asm__("nop");

    // [BƯỚC 3]: THỰC THI LỆNH NGỦ TIẾT KIỆM ĐIỆN CỰC HẠN
    __asm__("wfi");

    // =================================================================
    // THỨC DẬY: Khi bạn nhấn nút bấm kích hoạt ngắt, chip chạy tiếp từ đây
    // =================================================================

    // [BƯỚC 4]: Khôi phục tần số HSI 24MHz
    RCC->CTLR |= (1 << 3);

    // Trả bộ chia AHB Prescaler về 1 (Không chia)
    RCC->CFGR0 &= ~(0xf << 4);
    __asm__("nop");
    __asm__("nop");

    // Bật lại bộ nhân tần PLL lên tốc độ tối đa 48MHz ban đầu
    RCC->CFGR0 &= ~(RCC_SW);
    RCC->CFGR0 |= RCC_SW_PLL;
    while ((RCC->CFGR0 & RCC_SWS) != RCC_SWS_PLL)

        // Khôi phục lại toàn bộ xung clock ngoại vi ban đầu cho hệ thống hoạt động
        RCC->APB1PCENR = apb1_clock_bak;
    RCC->APB2PCENR = apb2_clock_bak;

    // [BƯỚC 5]: Kích hoạt lại tính năng nạp/debug trên chân SWIO [ch32fun]
    AFIO->PCFR1 &= ~(1 << 26);

    // Trễ nhỏ để mạch nạp WCH-Link đồng bộ lại nhịp thở với chip [ch32fun]
    Delay_Ms(10);
}

// ============================================================================
// HÀM XỬ LÝ NGẮT PHẦN CỨNG HỢP NHẤT EXTI LINE 0 -> LINE 7 [ch32fun]
// ============================================================================
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

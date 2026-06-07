#include "ch32v003_ir.h"
#include "ch32v003_gpio.h"
#include "ch32v003_delay.h"
#include "ch32v003_timer.h"
#include "ch32v003_debug.h"

#if (IR_ENABLE)

// ============================================================================
// MẢNG ĐỊNH NGHĨA THÔNG SỐ GIAO THỨC (LƯU TRÊN FLASH)
// ============================================================================
static const IR_ProtocolConfig_t irConfigs[] = {
#if (IR_SUPPORT_NEC)
    {
        .protocol = IR_NEC,
        .name = "NEC",
        .carrierFreqHz = 38000,
        .leader = {.mark = 9000, .space = 4500},
        .bit0 = {.mark = 560, .space = 560},
        .bit1 = {.mark = 560, .space = 1690},
        .stopMark = 560,
        .totalBits = 32,
        .encodeType = IR_ENCODE_DISTANCE
    },
#endif

#if (IR_SUPPORT_SAMSUNG)
    {
        .protocol = IR_SAMSUNG,
        .name = "SAMSUNG",
        .carrierFreqHz = 38000,
        .leader = {.mark = 4500, .space = 4500},
        .bit0 = {.mark = 560, .space = 560},
        .bit1 = {.mark = 560, .space = 1600},
        .stopMark = 560,
        .totalBits = 32,
        .encodeType = IR_ENCODE_DISTANCE
    },
#endif

#if (IR_SUPPORT_SONY)
    {
        .protocol = IR_SONY,
        .name = "SONY",
        .carrierFreqHz = 40000,
        .leader = {.mark = 2400, .space = 600},
        .bit0 = {.mark = 600, .space = 600},
        .bit1 = {.mark = 1200, .space = 600},
        .stopMark = 0,
        .totalBits = 12,
        .encodeType = IR_ENCODE_WIDTH, // Sony mã hóa bằng độ rộng xung vuông
    },
#endif
};

// Tính toán số lượng giao thức đang được enable thực tế trong mảng
static const uint8_t numConfigs = sizeof(irConfigs) / sizeof(irConfigs[0]);

// ============================================================================
// CÁC HÀM TRỢ GIÚP NỘI BỘ (HELPER FUNCTIONS)
// ============================================================================

/**
 * @brief Kiểm tra xem thời gian thực tế thu được có nằm trong khoảng dung sai của đích hay không.
 * @details Hàm tính toán khoảng giá trị hợp lệ: [target - delta, target + delta]
 * với delta = (target * IR_TOLERANCE_PERCENT) / 100.
 * Từ khóa static inline gợi ý trình biên dịch chèn trực tiếp đoạn code này vào nơi gọi
 * để triệt tiêu chi phí gọi hàm (function call overhead), tăng tốc độ xử lý ngắt/quét bit.
 * * @param duration Thời gian đo đạc thực tế tính bằng micro giây (us).
 * @param target Thời gian đích lý thuyết được cấu hình trong bảng giao thức (us).
 * @return uint8_t Trả về 1 nếu khớp (nằm trong khoảng dung sai), ngược lại trả về 0.
 */
static inline uint8_t isMatching(const uint32_t duration, const uint32_t target)
{
    const uint32_t delta = (target * IR_TOLERANCE_PERCENT) / 100;
    return (duration >= (target - delta) && duration <= (target + delta));
}

/**
 * @brief Đo độ rộng (thời gian duy trì) của một trạng thái xung (HIGH hoặc LOW) trên một chân GPIO.
 * @details Hàm sử dụng bộ đếm định thời hệ thống thông qua hàm ticks() để đo đạc chính xác thời gian theo us.
 * Cơ chế chặn treo (Timeout) dựa vào `IR_TIMEOUT_US` ngăn chặn việc vòng lặp `while` bị kẹt vô tận
 * khi tín hiệu bị nhiễu hoặc đứt đoạn.
 * * @param GPIOx Con trỏ tới ngoại vi GPIO quản lý chân (Ví dụ: GPIOA, GPIOD).
 * @param pinNumber Chỉ số chân cần đọc dữ liệu (0 - 7).
 * @param state Trạng thái xung cần đo (HIGH hoặc LOW).
 * @return uint32_t Thời gian duy trì trạng thái xung đó (us). Trả về 0 nếu quá thời gian chờ (Timeout).
 */
static uint32_t getPulseWidth(const GPIO_TypeDef* GPIOx, const uint8_t pinNumber, const uint8_t state)
{
    const uint32_t ticksPerUs = FUNCONF_SYSTEM_CORE_CLOCK / 1000000;
    const uint32_t timeoutTicks = IR_TIMEOUT_US * ticksPerUs;
    uint32_t startTime = ticks();

    // Vòng lặp 1: Chờ cho đến khi chân GPIO chuyển sang trạng thái mong muốn (Bắt đầu sườn xung)
    while (digitalReadPort(GPIOx, pinNumber) != state)
    {
        if ((ticks() - startTime) > timeoutTicks) return 0; // Timeout chờ sườn -> Thoát
    }

    // Ghi lại thời điểm bắt đầu sườn xung thực tế
    startTime = ticks();

    // Vòng lặp 2: Đếm thời gian chân GPIO duy trì trạng thái đó
    while (digitalReadPort(GPIOx, pinNumber) == state)
    {
        // SỬA TẠI ĐÂY: Nếu quá thời gian duy trì, trả về 0 lập tức để báo lỗi treo/mất tín hiệu
        if ((ticks() - startTime) > timeoutTicks) return 0;
    }

    // Tính toán thời gian kết thúc xung thực tế
    const uint32_t durationTicks = ticks() - startTime;

    // Quy đổi số chu kỳ máy (ticks) đo được sang đơn vị micro giây (us)
    return durationTicks / ticksPerUs;
}

/**
 * @brief Phát ra một chuỗi xung vuông liên tục (Mark) băm theo tần số sóng mang xác định.
 * @details Hàm thực hiện tính toán động chu kỳ dựa trên tần số truyền vào. 
 * Công thức toán học áp dụng:
 * - Toàn chu kỳ: `fullPeriodUs = 1,000,000 / freqHz` (Ví dụ 38000Hz => ~26us)
 * - Nửa chu kỳ (Bật/Tắt): `halfPeriodUs = fullPeriodUs / 2` (Ví dụ 38000Hz => ~13us)
 * - Số lượng chu kỳ cần lặp: `cycles = timeUs / fullPeriodUs`
 * Nhờ cơ chế này, bạn không cần khai báo cứng 13us hay 26us trong cấu hình chung hệ thống nữa.
 * * @param GPIOx Con trỏ tới ngoại vi GPIO quản lý chân phát IR.
 * @param pinNumber Chỉ số chân cần xuất tín hiệu (0 - 7).
 * @param freqHz Tần số sóng mang hồng ngoại cần băm (Ví dụ: 38000 cho 38kHz, 40000 cho 40kHz).
 * @param timeUs Tổng thời gian muốn duy trì việc phát chuỗi xung này (us).
 */
static void markHz(GPIO_TypeDef* GPIOx, const uint8_t pinNumber, const uint32_t freqHz, const uint32_t timeUs)
{
    const uint32_t fullPeriodUs = 1000000 / freqHz;
    const uint32_t halfPeriodUs = fullPeriodUs / 2;
    const uint32_t cycles = timeUs / fullPeriodUs;

    for (uint32_t i = 0; i < cycles; i++)
    {
        digitalWritePort(GPIOx, pinNumber, HIGH);
        delayUs(halfPeriodUs); // Trễ nửa chu kỳ (Mức CAO) -> ~13us với 38kHz
        digitalWritePort(GPIOx, pinNumber, LOW);
        delayUs(halfPeriodUs); // Trễ nửa chu kỳ (Mức THẤP) -> ~13us với 38kHz
    }
}

/**
 * @brief Phát ra một khoảng lặng (Space) bằng cách kéo chân GPIO xuống mức thấp trong một khoảng thời gian.
 * @details Khoảng lặng không chứa sóng mang băm tần số, bóng LED IR tắt hoàn toàn để đầu thu phân biệt với khoảng MARK.
 * * @param GPIOx Con trỏ tới ngoại vi GPIO quản lý chân phát IR.
 * @param pinNumber Chỉ số chân cần xuất tín hiệu (0 - 7).
 * @param timeUs Khoảng thời gian duy trì trạng thái lặng (us).
 */
static void space(GPIO_TypeDef* GPIOx, const uint8_t pinNumber, const uint32_t timeUs)
{
    digitalWritePort(GPIOx, pinNumber, LOW);
    delayUs(timeUs);
}

// ============================================================================
// LÕI GIẢI MÃ TỰ ĐỘNG QUÉT MẢNG LOOP DYNAMIC
// ============================================================================

uint8_t irReadPort(const GPIO_TypeDef* GPIOx, const uint8_t pinNumber, IR_Data_t* irData)
{
    if (digitalReadPort(GPIOx, pinNumber) == HIGH) return 0;

    // Đo xung mở đầu thực tế
    const uint32_t actualLeaderMark = getPulseWidth(GPIOx, pinNumber, LOW);
    const uint32_t actualLeaderSpace = getPulseWidth(GPIOx, pinNumber, HIGH);

    for (uint8_t p = 0; p < numConfigs; p++)
    {
        const IR_ProtocolConfig_t* cfg = &irConfigs[p];

        // 1. NỚI RỘNG CỬA CHECK LEADER (Cho phép lệch hẳn 1000us - 1500us vẫn đón nhận)
        if (isMatching(actualLeaderMark, cfg->leader.mark) &&
            (cfg->leader.space == 0 || isMatching(actualLeaderSpace, cfg->leader.space)))
        {
            uint64_t decodedData = 0;
            uint8_t bitCount = 0;
            uint8_t success = 1;

            // Tính toán ngưỡng cắt động
            uint32_t bitThreshold = 0;
            if (cfg->encodeType == IR_ENCODE_DISTANCE)
            {
                bitThreshold = (cfg->bit0.space + cfg->bit1.space) / 2;
            }
            else if (cfg->encodeType == IR_ENCODE_WIDTH)
            {
                bitThreshold = (cfg->bit0.mark + cfg->bit1.mark) / 2;
            }

            // 2. Vòng lặp bẫy bit dữ liệu
            for (uint8_t i = 0; i < cfg->totalBits; i++)
            {
                const uint32_t bitMark = getPulseWidth(GPIOx, pinNumber, LOW);
                const uint32_t bitSpace = getPulseWidth(GPIOx, pinNumber, HIGH);

                // Bảo vệ chống treo mạch nếu mất sườn xung giữa chừng
                if (bitMark == 0 || (bitSpace == 0 && i < (cfg->totalBits - 1)))
                {
                    success = 0;
                    break;
                }

                // Phân định Bit 0 / Bit 1 bằng ngưỡng cắt động và 1ULL
                if (cfg->encodeType == IR_ENCODE_DISTANCE)
                {
                    if (bitSpace > bitThreshold)
                    {
                        decodedData |= (1ULL << i);
                    }
                }
                else if (cfg->encodeType == IR_ENCODE_WIDTH)
                {
                    if (bitMark > bitThreshold)
                    {
                        decodedData |= (1ULL << i);
                    }
                }

                bitCount++;
            }

            // 3. ĐÓNG GÓI DỮ LIỆU (Bỏ check Checksum rườm rà, chỉ tin vào số lượng bit)
            if (success && bitCount == cfg->totalBits)
            {
                irData->protocol = cfg->protocol;
                irData->rawData = decodedData;
                irData->bits = cfg->totalBits;

                // Tách byte phục vụ tầng ứng dụng viết code cho tiện
                irData->address = decodedData & 0xFF;
                irData->command = (decodedData >> 16) & 0xFF;

                return 1; // Trả về THÀNH CÔNG ngay lập tức!
            }
        }
    }
    return 0;
}

uint8_t irReadPortRaw(const GPIO_TypeDef* GPIOx, const uint8_t pinNumber, IR_RawData_t* rawData)
{
    if (digitalReadPort(GPIOx, pinNumber) == HIGH) return 0;

    const uint32_t maxTimeoutTicks = IR_TIMEOUT_US * (FUNCONF_SYSTEM_CORE_CLOCK / 1000000);
    uint32_t lastTick = ticks();
    uint8_t index = 0;
    uint8_t lastState = LOW;

    rawData->rawLen = 0;

    while (index < IR_RAW_BUFFER_SIZE)
    {
        const uint8_t currentState = digitalReadPort(GPIOx, pinNumber);
        const uint32_t currentTick = ticks();

        if (currentState != lastState)
        {
            const uint32_t durationUs = (currentTick - lastTick) / (FUNCONF_SYSTEM_CORE_CLOCK / 1000000);
            rawData->rawBuf[index++] = (uint16_t)durationUs;
            lastState = currentState;
            lastTick = currentTick;
        }

        if ((currentTick - lastTick) > maxTimeoutTicks)
        {
            if (lastState == HIGH && index > 10)
            {
                rawData->rawLen = index;
                return 1;
            }
            break;
        }
    }

    if (index > 10)
    {
        rawData->rawLen = index;
        return 1;
    }
    return 0;
}


// ============================================================================
// TRIỂN KHAI PHÁT TÍN HIỆU THEO HƯỚNG CẤU HÌNH DỮ LIỆU
// ============================================================================

void irSendPortRaw(GPIO_TypeDef* GPIOx, const uint8_t pinNumber, const IR_RawData_t* rawData)
{
    for (uint8_t i = 0; i < rawData->rawLen; i++)
    {
        if (i % 2 == 0) markHz(GPIOx, pinNumber, 38000, rawData->rawBuf[i]);
        else space(GPIOx, pinNumber, rawData->rawBuf[i]);
    }
    digitalWritePort(GPIOx, pinNumber, LOW);
}

void irSendPort(GPIO_TypeDef* GPIOx, const uint8_t pinNumber, const IR_Protocol_t protocol, const uint16_t address,
                const uint32_t command)
{
    const IR_ProtocolConfig_t* cfg = NULL;
    for (uint8_t p = 0; p < numConfigs; p++)
    {
        if (irConfigs[p].protocol == protocol)
        {
            cfg = &irConfigs[p];
            break;
        }
    }
    if (!cfg) return; // Giao thức chưa được enable hoặc không tồn tại

    // Xử lý nén dữ liệu bit để phát (Ví dụ mẫu cho NEC, với TLC bạn có thể custom thêm)
    uint64_t dataToTx = 0;
    if (protocol == IR_NEC)
    {
        dataToTx = (address & 0xFF) | ((~(address) & 0xFF) << 8) |
            ((command & 0xFF) << 16) | ((~(command) & 0xFF) << 24);
    }
    else
    {
        return; // Cần bổ sung quy luật gộp bit riêng của từng hãng khi phát chuỗi tường minh
    }

    // Phát xung Leader
    markHz(GPIOx, pinNumber, cfg->carrierFreqHz, cfg->leader.mark);
    space(GPIOx, pinNumber, cfg->leader.space);

    // Phát chuỗi bit dữ liệu
    for (uint8_t i = 0; i < cfg->totalBits; i++)
    {
        if (dataToTx & ((uint64_t)1 << i))
        {
            markHz(GPIOx, pinNumber, cfg->carrierFreqHz, cfg->bit1.mark);
            space(GPIOx, pinNumber, cfg->bit1.space);
        }
        else
        {
            markHz(GPIOx, pinNumber, cfg->carrierFreqHz, cfg->bit0.mark);
            space(GPIOx, pinNumber, cfg->bit0.space);
        }
    }

    // Phát xung Stop kết thúc
    if (cfg->stopMark > 0)
    {
        markHz(GPIOx, pinNumber, cfg->carrierFreqHz, cfg->stopMark);
    }
    digitalWritePort(GPIOx, pinNumber, LOW);
}


// ============================================================================
// HÀM CHẨN ĐOÁN VÀ PHÂN TÍCH GIAO THỨC MỚI (TEST FUNCTIONS)
// ============================================================================

static uint32_t absDiff(const uint32_t number1, const uint32_t number2)
{
    return (number1 > number2) ? (number1 - number2) : (number2 - number1);
}

void printBin8(const uint8_t val)
{
    for (int i = 7; i >= 0; i--)
    {
        printf("%c", (val & (1 << i)) ? '1' : '0');
    }
}

void irAnalyzeRaw(const IR_RawData_t* rawData)
{
    if (rawData == NULL || rawData->rawLen < 4) return;

    const uint16_t actMark = rawData->rawBuf[0];
    const uint16_t actSpace = rawData->rawBuf[1];
    const uint16_t totalBits = (uint16_t)((rawData->rawLen - 2) / 2);

    printf("\r\n--- IR PROTOCOL FINDER ---\r\n");
    printf("Edges: %u | Leader M: %u us, S: %u us\r\n",
           (unsigned int)rawData->rawLen, (unsigned int)actMark, (unsigned int)actSpace);

    uint8_t matched = 0;
    const IR_ProtocolConfig_t* targetCfg = NULL;

    for (uint8_t p = 0; p < numConfigs; p++)
    {
        const IR_ProtocolConfig_t* cfg = &irConfigs[p];
        const int mDiff = (int)actMark - (int)cfg->leader.mark;
        const int sDiff = (int)actSpace - (int)cfg->leader.space;

        const uint16_t mErr = (uint16_t)((absDiff(actMark, cfg->leader.mark) * 100) / cfg->leader.mark);
        const uint16_t sErr = (uint16_t)((absDiff(actSpace, cfg->leader.space) * 100) / cfg->leader.space);

        printf(" -> [%s]: Lech M:%s%d (%u%%), S:%s%d (%u%%)\r\n",
               cfg->name,
               (mDiff >= 0) ? "+" : "", mDiff, (unsigned int)mErr,
               (sDiff >= 0) ? "+" : "", sDiff, (unsigned int)sErr);

        if (mErr <= 25 && sErr <= 25)
        {
            matched = 1;
            targetCfg = cfg;
        }
    }

    uint32_t avgBit0Space = 0, avgBit1Space = 0, avgMark = 0;
    uint16_t b0Count = 0, b1Count = 0;

    for (uint8_t i = 2; i < rawData->rawLen - 1; i += 2)
    {
        const uint16_t mTime = rawData->rawBuf[i];
        const uint16_t sTime = rawData->rawBuf[i + 1];
        avgMark += mTime;

        if (sTime > 1100)
        {
            avgBit1Space += sTime;
            b1Count++;
        }
        else
        {
            avgBit0Space += sTime;
            b0Count++;
        }
    }

    if (totalBits > 0) avgMark /= totalBits;

    printf("\r\n🔍 KET LUAN:\r\n");
    if (matched && targetCfg != NULL)
    {
        printf(" -> Khop giao thuc: [%s]\r\n", targetCfg->name);
        if (totalBits != (uint16_t)targetCfg->totalBits)
        {
            printf(" -> Canh bao: Sai so bit! Cấu hình: %u, Thuc te: %u\r\n",
                   (unsigned int)targetCfg->totalBits, (unsigned int)totalBits);
        }
    }
    else
    {
        printf(" -> KHONG THUOC GIAO THUC DA DINH NGHIA (Remote La)\r\n");
    }

    printf("\r\n🛠️ STRUCT SUGGEST:\r\n");
    printf("  .totalBits = %u\r\n", (unsigned int)totalBits);
    printf("  .leader    = {.mark = %u, .space = %u}\r\n", (unsigned int)actMark, (unsigned int)actSpace);

    if (b0Count)
        printf("  .bit0      = {.mark = %u, .space = %u}\r\n",
               (unsigned int)avgMark, (unsigned int)(avgBit0Space / b0Count));
    if (b1Count)
        printf("  .bit1      = {.mark = %u, .space = %u}\r\n",
               (unsigned int)avgMark, (unsigned int)(avgBit1Space / b1Count));

    printf("===========================\r\n\r\n");
}

void irPrintResult(const IR_Data_t* irData)
{
    if (irData == NULL) return;

    const char* pName = "UNKNOWN";
    for (uint8_t p = 0; p < numConfigs; p++)
    {
        if (irConfigs[p].protocol == irData->protocol)
        {
            pName = irConfigs[p].name;
            break;
        }
    }

    const uint32_t rawHigh = (uint32_t)(irData->rawData >> 32);
    const uint32_t rawLow = (uint32_t)(irData->rawData & 0xFFFFFFFF);

    printf("🎉 SUCCESS -> [%s] | Bits: %u\r\n", pName, (unsigned int)irData->bits);
    printf("RAW: 0x%08X%08X\r\n", (unsigned int)rawHigh, (unsigned int)rawLow);

    if (irData->bits == 32)
    {
        const uint8_t b1 = (uint8_t)(rawLow & 0xFF);
        const uint8_t b2 = (uint8_t)((rawLow >> 8) & 0xFF);
        const uint8_t b3 = (uint8_t)((rawLow >> 16) & 0xFF);
        const uint8_t b4 = (uint8_t)((rawLow >> 24) & 0xFF);

        printf("📊 LSB-First bytes:\r\n");
        printf("   Addr: 0x%02X (Bin: ", b1);
        printBin8(b1);
        printf(")\r\n");
        printf("   ~Add: 0x%02X (Bin: ", b2);
        printBin8(b2);
        printf(")\r\n");
        printf("   Cmd : 0x%02X (Bin: ", b3);
        printBin8(b3);
        printf(")\r\n");
        printf("   ~Cmd: 0x%02X (Bin: ", b4);
        printBin8(b4);
        printf(")\r\n");

        if ((uint8_t)(b1 + b2) == 0xFF && (uint8_t)(b3 + b4) == 0xFF)
        {
            printf("   👉 [Checksum OK]\r\n");
        }
        else
        {
            printf("   👉 [Extended / Repeat]\r\n");
        }
    }
    printf("---------------------------\r\n\r\n");
}

// ============================================================================
// ĐỒNG BỘ CÁC HÀM ĐƠN CHÂN KIỂU CORE MỚI
// ============================================================================

uint8_t irReadRaw(const uint8_t mcu_pin, IR_RawData_t* rawData)
{
    GPIO_TypeDef* GPIOx;
    uint8_t pinNumber;
    decodeHardwarePin(mcu_pin, &GPIOx, &pinNumber);
    return irReadPortRaw(GPIOx, pinNumber, rawData);
}

uint8_t irRead(const uint8_t mcu_pin, IR_Data_t* irData)
{
    GPIO_TypeDef* GPIOx;
    uint8_t pinNumber;
    decodeHardwarePin(mcu_pin, &GPIOx, &pinNumber);
    return irReadPort(GPIOx, pinNumber, irData);
}

void irSendRaw(const uint8_t mcu_pin, const IR_RawData_t* rawData)
{
    GPIO_TypeDef* GPIOx;
    uint8_t pinNumber;
    decodeHardwarePin(mcu_pin, &GPIOx, &pinNumber);
    irSendPortRaw(GPIOx, pinNumber, rawData);
}

void irSend(const uint8_t mcu_pin, const IR_Protocol_t protocol, const uint16_t address, const uint32_t command)
{
    GPIO_TypeDef* GPIOx;
    uint8_t pinNumber;
    decodeHardwarePin(mcu_pin, &GPIOx, &pinNumber);
    irSendPort(GPIOx, pinNumber, protocol, address, command);
}

#endif // IR_ENABLE

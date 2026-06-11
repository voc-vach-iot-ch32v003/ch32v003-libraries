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
        .encodeType = IR_ENCODE_DISTANCE,
        .bitOrder = IR_ORDER_LSB_FIRST,
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
        .encodeType = IR_ENCODE_DISTANCE,
        .bitOrder = IR_ORDER_LSB_FIRST,
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
        .encodeType = IR_ENCODE_WIDTH,
        .bitOrder = IR_ORDER_LSB_FIRST,
    },
#endif
#if (IR_SUPPORT_TCL)
    {
        .protocol = IR_TCL,
        .name = "TCL",
        .carrierFreqHz = 38000,
        .leader = {.mark = 4000, .space = 4000},
        .bit0 = {.mark = 590, .space = 1050},
        .bit1 = {.mark = 590, .space = 2450},
        .stopMark = 590,
        .totalBits = 76,
        .encodeType = IR_ENCODE_DISTANCE,
        .bitOrder = IR_ORDER_LSB_FIRST,
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
    IR_RawData_t rawTmp;

    if (!irReadPortRaw(GPIOx, pinNumber, &rawTmp))
    {
        return 0;
    }

    const uint16_t actMark = rawTmp.rawBuf[0];
    const uint16_t actSpace = rawTmp.rawBuf[1];
    const uint16_t actTotalBits = (uint16_t)((rawTmp.rawLen - 2) / 2);

    for (uint8_t p = 0; p < numConfigs; p++)
    {
        const IR_ProtocolConfig_t* cfg = &irConfigs[p];

        if (actTotalBits != (uint16_t)cfg->totalBits)
        {
            continue;
        }

        if (isMatching(actMark, cfg->leader.mark) &&
            (cfg->leader.space == 0 || isMatching(actSpace, cfg->leader.space)))
        {
            // 1. Xóa sạch mảng dữ liệu cũ bằng 0 trước khi nạp bit mới
            for (int b = 0; b < IR_RAW_DATA_BYTES; b++)
            {
                irData->rawData[b] = 0;
            }

            uint32_t bitThreshold = 0;
            if (cfg->encodeType == IR_ENCODE_DISTANCE)
            {
                bitThreshold = (cfg->bit0.space + cfg->bit1.space) / 2;
            }
            else if (cfg->encodeType == IR_ENCODE_WIDTH)
            {
                bitThreshold = (cfg->bit0.mark + cfg->bit1.mark) / 2;
            }

            // 2. Vòng lặp bóc tách và nạp bit vào mảng
            uint8_t rawIdx = 2;
            for (uint8_t i = 0; i < cfg->totalBits; i++)
            {
                const uint16_t bitMark = rawTmp.rawBuf[rawIdx++];
                const uint16_t bitSpace = rawTmp.rawBuf[rawIdx++];
                uint8_t isBit1 = 0;

                if (cfg->encodeType == IR_ENCODE_DISTANCE)
                {
                    if (bitSpace > bitThreshold) isBit1 = 1;
                }
                else if (cfg->encodeType == IR_ENCODE_WIDTH)
                {
                    if (bitMark > bitThreshold) isBit1 = 1;
                }

                // 👉 CẬP NHẬT: NẠP BIT VÀO MẢNG DỰA TRÊN BIT ORDER CỦA HÃNG
                if (isBit1)
                {
                    if (cfg->bitOrder == IR_ORDER_MSB_FIRST)
                    {
                        // Nếu là MSB-First, bit nhận được đầu tiên sẽ là bit cao nhất (Bit 7 của byte)
                        irData->rawData[i / 8] |= (1 << (7 - (i % 8)));
                    }
                    else
                    {
                        // Nếu là LSB-First (Mặc định), bit nhận được đầu tiên là bit thấp nhất (Bit 0 của byte)
                        irData->rawData[i / 8] |= (1 << (i % 8));
                    }
                }
            }

            // 3. Đóng gói các thông tin cơ bản cho tầng ứng dụng
            irData->protocol = cfg->protocol;
            irData->bits = cfg->totalBits;

            // Tách nhanh Address và Command từ các byte đầu tiên (phù hợp với hệ LSB như NEC/Samsung)
            // Lưu ý: Đoạn ép byte này hoạt động chuẩn khi cấu hình là LSB-First.
            irData->address = (uint16_t)(irData->rawData[0] | (irData->rawData[1] << 8));
            irData->command = (uint32_t)(irData->rawData[2] | (irData->rawData[3] << 16));

            return 1;
        }
    }
    return 0;
}

uint8_t irReadPortRaw(const GPIO_TypeDef* GPIOx, const uint8_t pinNumber, IR_RawData_t* rawData)
{
    if (digitalReadPort(GPIOx, pinNumber) == HIGH) return 0;

    const uint32_t maxTimeoutTicks = IR_TIMEOUT_US * (FUNCONF_SYSTEM_CORE_CLOCK / 1000000);
    uint32_t lastTick = ticks();
    uint16_t index = 0;
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

void irSendPortData(GPIO_TypeDef* GPIOx, const uint8_t pinNumber, const IR_Data_t* irData)
{
    if (irData == NULL) return;

    // 1. Quét mảng tìm cấu hình tần số và timeline xung của giao thức cần phát
    const IR_ProtocolConfig_t* cfg = NULL;
    for (uint8_t p = 0; p < numConfigs; p++)
    {
        if (irConfigs[p].protocol == irData->protocol)
        {
            cfg = &irConfigs[p];
            break;
        }
    }
    if (!cfg) return; // Không tìm thấy profile hoặc chưa enable hãng này

    // 2. Bắn chuỗi xung mồi (Leader Mark & Space)
    markHz(GPIOx, pinNumber, cfg->carrierFreqHz, cfg->leader.mark);
    if (cfg->leader.space > 0)
    {
        space(GPIOx, pinNumber, cfg->leader.space);
    }

    // 3. Vòng lặp bốc tách từng bit một từ mảng byte rawData để phát ra ngoài
    for (uint8_t i = 0; i < irData->bits; i++)
    {
        // 👉 ÁP DỤNG BIT ORDER: Kiểm tra cấu hình dịch bit của hãng
        uint8_t isBit1 = 0;
        if (cfg->bitOrder == IR_ORDER_MSB_FIRST)
        {
            // Dịch từ bit cao nhất (Bit 7) xuống bit thấp nhất (Bit 0) trong một byte
            isBit1 = (irData->rawData[i / 8] >> (7 - (i % 8))) & 0x01;
        }
        else
        {
            // Dịch từ bit thấp nhất (Bit 0) lên bit cao nhất (Bit 7) (Mặc định LSB-First)
            isBit1 = (irData->rawData[i / 8] >> (i % 8)) & 0x01;
        }

        if (cfg->encodeType == IR_ENCODE_DISTANCE) // Kiểu NEC, SAMSUNG, TCL
            if (!isBit1)
            {
                markHz(GPIOx, pinNumber, cfg->carrierFreqHz, cfg->bit0.mark);
                space(GPIOx, pinNumber, cfg->bit0.space);
            }
            else
            {
                markHz(GPIOx, pinNumber, cfg->carrierFreqHz, cfg->bit1.mark);
                space(GPIOx, pinNumber, cfg->bit1.space);
            }
        else if (cfg->encodeType == IR_ENCODE_WIDTH) // Kiểu SONY
        {
            if (isBit1)
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
    }

    // 4. Bắn xung chốt hạ (Stop Mark) để kết thúc khung truyền
    if (cfg->stopMark > 0)
    {
        markHz(GPIOx, pinNumber, cfg->carrierFreqHz, cfg->stopMark);
    }

    // Đảm bảo LED tắt hoàn toàn sau khi gửi dữ liệu xong
    digitalWritePort(GPIOx, pinNumber, LOW);
}

void irSendPort(GPIO_TypeDef* GPIOx, const uint8_t pinNumber, const IR_Protocol_t protocol, const uint16_t address,
                const uint32_t command)
{
    // 👉 1. Chủ động tìm kiếm cấu hình của giao thức này trong Flash trước
    const IR_ProtocolConfig_t* cfg = NULL;
    for (uint8_t p = 0; p < numConfigs; p++)
    {
        if (irConfigs[p].protocol == protocol)
        {
            cfg = &irConfigs[p];
            break;
        }
    }
    if (!cfg) return; // Nếu giao thức chưa được định nghĩa trong irConfigs[], thoát ngay

    IR_Data_t tmp;
    tmp.protocol = protocol;

    // 👉 2. Tự động lấy số lượng bit mặc định định nghĩa sẵn trong mảng Flash cấu hình tĩnh
    tmp.bits = cfg->totalBits;

    // 3. Đóng gói dữ liệu mộc vào mảng byte theo đúng quy tắc của từng hãng
    tmp.rawData[0] = address & 0xFF;
    tmp.rawData[1] = (address >> 8) & 0xFF;

    if (protocol == IR_NEC)
    {
        tmp.rawData[0] = address & 0xFF;
        tmp.rawData[1] = (~address) & 0xFF;
        tmp.rawData[2] = command & 0xFF;
        tmp.rawData[3] = (~command) & 0xFF;
    }
    else
    {
        tmp.rawData[2] = command & 0xFF;
        tmp.rawData[3] = (command >> 8) & 0xFF;
        tmp.rawData[4] = (command >> 16) & 0xFF;
        tmp.rawData[5] = (command >> 24) & 0xFF;
    }

    // 4. Đẩy sang hàm core chính để thực hiện phát xung băm
    irSendPortData(GPIOx, pinNumber, &tmp);
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

    // 1. Tính toán các thông số thực tế từ mảng xung thô thu được
    const uint16_t actMark = rawData->rawBuf[0];
    const uint16_t actSpace = rawData->rawBuf[1];
    const uint16_t totalBits = (uint16_t)((rawData->rawLen - 2) / 2);

    printf("\r\n--- IR PROTOCOL FINDER ---\r\n");
    printf("Edges: %u | Leader M: %u us, S: %u us | Total Bits: %u\r\n",
           (unsigned int)rawData->rawLen, (unsigned int)actMark, (unsigned int)actSpace, (unsigned int)totalBits);

    uint8_t matched = 0;
    const IR_ProtocolConfig_t* targetCfg = NULL;

    // 2. Chạy hết mảng cấu hình để tìm kiếm xem có thằng nào khớp không
    for (uint8_t p = 0; p < numConfigs; p++)
    {
        const IR_ProtocolConfig_t* cfg = &irConfigs[p];

        // Tính toán sai số Leader
        const uint16_t mErr = (uint16_t)((absDiff(actMark, cfg->leader.mark) * 100) / cfg->leader.mark);
        const uint16_t sErr = (uint16_t)((absDiff(actSpace, cfg->leader.space) * 100) / cfg->leader.space);

        // ĐIỀU KIỆN ÉP KHỚP HOÀN TOÀN: Khớp Leader (<25%) VÀ Phải khớp tuyệt đối số lượng Bit
        if (mErr <= 25 && sErr <= 25 && totalBits == (uint16_t)cfg->totalBits)
        {
            matched = 1;
            targetCfg = cfg;

            // In thông tin độ lệch của giao thức khớp duy nhất này
            const int mDiff = (int)actMark - (int)cfg->leader.mark;
            const int sDiff = (int)actSpace - (int)cfg->leader.space;
            printf(" -> [%s] MATCHED: Lech M:%s%d (%u%%), S:%s%d (%u%%) | Order: %s\r\n",
                   cfg->name,
                   (mDiff >= 0) ? "+" : "", mDiff, (unsigned int)mErr,
                   (sDiff >= 0) ? "+" : "", sDiff, (unsigned int)sErr,
                   (cfg->bitOrder == IR_ORDER_MSB_FIRST) ? "MSB-FIRST" : "LSB-FIRST");

            break; // Đã tìm thấy giao thức đích thực, dừng cuộc chơi!
        }
    }

    // 3. Tính toán các thông số trung bình của Bit 0 / Bit 1 để gợi ý STRUCT
    uint32_t avgBit0Space = 0, avgBit1Space = 0, avgMark = 0;
    uint16_t b0Count = 0, b1Count = 0;

    // Khởi tạo mảng tạm để thử nghiệm dịch dữ liệu HEX phục vụ in log gợi ý
    uint8_t hexLSB[IR_RAW_DATA_BYTES] = {0};
    uint8_t hexMSB[IR_RAW_DATA_BYTES] = {0};
    uint8_t bitIdx = 0;

    for (uint8_t i = 2; i < rawData->rawLen - 1; i += 2)
    {
        const uint16_t mTime = rawData->rawBuf[i];
        const uint16_t sTime = rawData->rawBuf[i + 1];
        avgMark += mTime;

        uint8_t isBit1 = 0;
        if (sTime > 1100) // Ngưỡng phân định khoảng lặng mặc định
        {
            avgBit1Space += sTime;
            b1Count++;
            isBit1 = 1;
        }
        else
        {
            avgBit0Space += sTime;
            b0Count++;
        }

        // Ép thử bit vào mảng tạm theo cả 2 chiều để hiển thị gợi ý mã HEX
        if (isBit1 && (bitIdx < IR_RAW_DATA_BYTES * 8))
        {
            hexLSB[bitIdx / 8] |= (1 << (bitIdx % 8)); // Thử dịch kiểu LSB
            hexMSB[bitIdx / 8] |= (1 << (7 - (bitIdx % 8))); // Thử dịch kiểu MSB
        }
        bitIdx++;
    }

    if (totalBits > 0) avgMark /= totalBits;

    // 4. ĐƯA RA KẾT LUẬN CUỐI CÙNG
    printf("\r\n🔍 KET LUAN:\r\n");
    if (matched && targetCfg != NULL)
    {
        printf(" -> Khop giao thuc: [%s]\r\n", targetCfg->name);
    }
    else
    {
        printf(" -> KHONG THUOC GIAO THUC DA DINH NGHIA (Remote La)\r\n");
    }

    // 5. LUÔN LUÔN GỢI Ý CẤU TRÚC (Bổ sung gợi ý cấu hình bitOrder và Data chuỗi Hex)
    printf("\r\n🛠️ STRUCT SUGGEST:\r\n");
    printf("  .totalBits   = %u\r\n", (unsigned int)totalBits);
    printf("  .leader      = {.mark = %u, .space = %u}\r\n", (unsigned int)actMark, (unsigned int)actSpace);

    if (b0Count)
        printf("  .bit0        = {.mark = %u, .space = %u}\r\n",
               (unsigned int)avgMark, (unsigned int)(avgBit0Space / b0Count));
    if (b1Count)
        printf("  .bit1        = {.mark = %u, .space = %u}\r\n",
               (unsigned int)avgMark, (unsigned int)(avgBit1Space / b1Count));

    printf("  .encodeType  = IR_ENCODE_DISTANCE\r\n");
    printf("  .bitOrder    = %s\r\n", (matched && targetCfg != NULL && targetCfg->bitOrder == IR_ORDER_MSB_FIRST)
                                          ? "IR_ORDER_MSB_FIRST"
                                          : "IR_ORDER_LSB_FIRST");

    // 6. IN THỬ DATA HEX GỢI Ý ĐỂ COPY NHANH
    const uint8_t totalBytesNeeded = (totalBits + 7) / 8;
    printf("\r\n📦 SUGGESTED HEX DATA (If you cloning code):\r\n");

    printf("  -> If LSB-First (Common): 0x");
    for (int b = 0; b < totalBytesNeeded; b++) printf("%02X", hexLSB[b]);

    printf("\r\n  -> If MSB-First (Rare)  : 0x");
    for (int b = 0; b < totalBytesNeeded; b++) printf("%02X", hexMSB[b]);

    printf("\r\n===========================\r\n\r\n");
}

void irPrintResult(const IR_Data_t* irData)
{
    if (irData == NULL) return;

    // 1. Tìm tên giao thức dựa vào ID
    const char* pName = "UNKNOWN";
    for (uint8_t p = 0; p < numConfigs; p++)
    {
        if (irConfigs[p].protocol == irData->protocol)
        {
            pName = irConfigs[p].name;
            break;
        }
    }

    printf("🎉 SUCCESS -> [%s] | Bits: %u\r\n", pName, (unsigned int)irData->bits);

    // 2. IN CHUỖI DỮ LIỆU HEX (Thay thế cho cách in rawHigh / rawLow cũ)
    // In ngược từ byte cuối về byte đầu để số hiển thị thuận mắt từ Trái sang Phải
    printf("RAW: 0x");
    for (int i = IR_RAW_DATA_BYTES - 1; i >= 0; i--)
    {
        printf("%02X", irData->rawData[i]);
    }
    printf("\r\n");

    // 3. LOGIC RIÊNG CHO HỆ 32-BIT (NEC / SAMSUNG / ...) - GIỮ NGUYÊN Ý ĐỒ CỦA BẠN
    if (irData->bits == 32)
    {
        // Vì là mảng byte, ta nhặt trực tiếp 4 byte đầu tiên cực kỳ nhàn, không cần dịch >> nữa!
        const uint8_t b1 = irData->rawData[0]; // Byte thấp nhất (LSB)
        const uint8_t b2 = irData->rawData[1];
        const uint8_t b3 = irData->rawData[2];
        const uint8_t b4 = irData->rawData[3]; // Byte cao nhất (MSB của khung 32-bit)

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

        // Kiểm tra Checksum (Byte đảo nghịch)
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

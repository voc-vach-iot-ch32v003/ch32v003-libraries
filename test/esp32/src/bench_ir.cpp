#include "bench_ir.h"
#include "bench_channels.h"
#include "bench_pins.h"
#include <soc/gpio_struct.h>

// ============================================================================
// THÔNG SỐ GIAO THỨC (theo đặc tả, độc lập với thư viện cần kiểm thử)
// ============================================================================

struct IrTiming
{
    const char* name;
    uint16_t leaderMark, leaderSpace;
    uint16_t mark0, space0, mark1, space1;
    uint16_t stopMark; // 0 = không có
    uint8_t bits;
    uint32_t periodUs; // Chu kỳ lặp khung
};

static const IrTiming IR_TIMINGS[] = {
    {"NEC", 9000, 4500, 560, 560, 560, 1690, 560, 32, 108000},
    {"SAMSUNG", 4500, 4500, 560, 560, 560, 1690, 560, 32, 108000},
    {"SONY", 2400, 600, 600, 600, 1200, 600, 0, 12, 45000},
    {"TCL", 4000, 4000, 590, 1050, 590, 2450, 590, 76, 0},
};

// Bắt tín hiệu: LOW lâu hơn mức này coi là hết một Mark (sóng mang 38kHz chỉ LOW ~13us giữa 2 xung)
static const uint32_t ENVELOPE_GAP_US = 100;
// Giới hạn tổng thời gian bắt; ngắt được mở chớp nhoáng trong mỗi Space (>= 100us LOW, chưa có sườn kế tiếp
// trong vài trăm us) nên watchdog ngắt (~300ms) vẫn được phục vụ
static const uint32_t CAPTURE_MAX_US = 2000000;

static inline uint32_t cycles()
{
    return ESP.getCycleCount();
}

static inline int fastRead(const uint8_t gpio)
{
    return gpio < 32 ? (GPIO.in >> gpio) & 1 : (GPIO.in1.data >> (gpio - 32)) & 1;
}

static inline void fastWrite(const uint8_t gpio, const int level)
{
    if (gpio < 32)
    {
        if (level) GPIO.out_w1ts = 1UL << gpio;
        else GPIO.out_w1tc = 1UL << gpio;
    }
    else
    {
        if (level) GPIO.out1_w1ts.val = 1UL << (gpio - 32);
        else GPIO.out1_w1tc.val = 1UL << (gpio - 32);
    }
}

// ============================================================================
// PHÁT (canh mốc tuyệt đối theo bộ đếm chu kỳ CPU, không khóa ngắt: TCL dài tới ~230ms)
// ============================================================================

struct IrSender
{
    uint8_t gpio;
    uint32_t cyclesPerUs;
    uint32_t edge;

    void segment(const int level, const uint32_t us)
    {
        while ((int32_t)(cycles() - edge) < 0)
        {
        }
        fastWrite(gpio, level);
        edge += us * cyclesPerUs;
    }
    void mark(const uint32_t us) { segment(0, us); }
    void space(const uint32_t us) { segment(1, us); }
};

bool benchIrSend(const int channel, const String& protocol, const uint8_t* data, const size_t length,
                 const uint8_t frames)
{
    const IrTiming* t = nullptr;
    for (const IrTiming& candidate : IR_TIMINGS)
    {
        if (protocol.equalsIgnoreCase(candidate.name)) t = &candidate;
    }
    if (t == nullptr || length * 8 < t->bits || frames == 0 || (frames > 1 && t->periodUs == 0)) return false;

    const uint8_t gpio = BENCH_CHANNELS[channel].gpio;
    benchWrite(channel, 1); // Rảnh = HIGH
    delay(2);

    IrSender s{gpio, ESP.getCpuFreqMHz(), cycles() + 50 * ESP.getCpuFreqMHz()};
    const bool isNec = strcmp(t->name, "NEC") == 0;
    for (uint8_t f = 0; f < frames; f++)
    {
        const uint32_t frameStart = s.edge;
        if (f > 0 && isNec)
        {
            s.mark(9000); // Mã lặp NEC
            s.space(2250);
            s.mark(560);
        }
        else
        {
            s.mark(t->leaderMark);
            s.space(t->leaderSpace);
            for (uint8_t i = 0; i < t->bits; i++)
            {
                const bool one = (data[i >> 3] >> (i & 7)) & 1;
                s.mark(one ? t->mark1 : t->mark0);
                s.space(one ? t->space1 : t->space0);
            }
            if (t->stopMark) s.mark(t->stopMark);
        }
        s.space(0); // Kết thúc Mark cuối (Sony: Space của bit cuối đã nằm trong vòng lặp)
        if (f + 1 < frames) s.edge = frameStart + t->periodUs * s.cyclesPerUs;
    }
    while ((int32_t)(cycles() - s.edge) < 0)
    {
    }
    fastWrite(gpio, 1);
    return true;
}

// ============================================================================
// BẮT TÍN HIỆU CÓ SÓNG MANG
// ============================================================================

uint16_t benchIrCapture(const int channel, uint32_t* durations, const uint16_t maxCount, const uint32_t timeoutMs,
                        const uint32_t gapUs, uint32_t& carrierHz, uint32_t& dutyPercent)
{
    const uint8_t gpio = BENCH_CHANNELS[channel].gpio;
    const uint32_t cyclesPerUs = ESP.getCpuFreqMHz();
    const uint32_t envelopeGap = ENVELOPE_GAP_US * cyclesPerUs;
    const uint32_t endGap = gapUs * cyclesPerUs;
    carrierHz = 0;
    dutyPercent = 0;

    // 1. Chờ sườn lên đầu tiên (không khóa ngắt)
    const uint32_t waitStart = millis();
    while (fastRead(gpio) == 0)
    {
        if (millis() - waitStart >= timeoutMs) return 0;
    }

    // 2. Khóa ngắt, theo dõi từng xung sóng mang
    portDISABLE_INTERRUPTS();
    bool serviced = false; // Đã mở ngắt trong Space hiện tại?
    uint32_t markStart = cycles();
    uint32_t lastRise = markStart, lastFall = markStart;
    const uint32_t captureStart = markStart;
    const uint32_t captureLimit = CAPTURE_MAX_US * cyclesPerUs;
    uint64_t periodSum = 0, highSum = 0;
    uint32_t periods = 0, pulses = 0;
    int level = 1;
    uint16_t count = 0;

    while (count + 2 < maxCount && cycles() - captureStart < captureLimit)
    {
        const int now = fastRead(gpio);
        const uint32_t t = cycles();
        if (now != level)
        {
            if (now)
            {
                if (t - lastFall > envelopeGap)
                {
                    durations[count++] = lastFall - markStart; // Mark vừa xong
                    durations[count++] = t - lastFall; // Space
                    markStart = t;
                    serviced = false;
                }
                else
                {
                    periodSum += t - lastRise;
                    periods++;
                }
                lastRise = t;
            }
            else
            {
                highSum += t - lastRise;
                pulses++;
                lastFall = t;
            }
            level = now;
        }
        else if (!level && t - lastFall > endGap)
        {
            break;
        }
        else if (!level && !serviced && t - lastFall > envelopeGap)
        {
            portENABLE_INTERRUPTS(); // Cho ngắt đang chờ chạy (vài us) giữa Space
            portDISABLE_INTERRUPTS();
            serviced = true;
        }
    }
    if (level == 0) durations[count++] = lastFall - markStart; // Mark cuối
    portENABLE_INTERRUPTS();

    for (uint16_t i = 0; i < count; i++) durations[i] /= cyclesPerUs;
    if (periods && periodSum)
    {
        carrierHz = (uint32_t)((uint64_t)periods * cyclesPerUs * 1000000ULL / periodSum);
        dutyPercent = (uint32_t)(highSum * 100 * periods / pulses / periodSum);
    }
    return count;
}

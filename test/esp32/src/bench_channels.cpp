#include "bench_channels.h"
#include "bench_pins.h"
#include <driver/dac.h>
#include <soc/gpio_struct.h>

// ============================================================================
// TIỆN ÍCH NỘI BỘ
// ============================================================================

// Chế độ hiện tại của từng kênh (để biết khi nào cần chuyển sang OUT trước khi ghi)
enum ChannelMode : uint8_t { MODE_IN, MODE_OUT, MODE_OD };
static ChannelMode channelModes[BENCH_CHANNEL_COUNT];

// Giới hạn thời gian khóa ngắt khi bắt sườn (watchdog ngắt của ESP32 ~300ms)
static const uint32_t CAPTURE_MAX_LOCKED_US = 200000;

static inline uint8_t gpioOf(const int channel)
{
    return BENCH_CHANNELS[channel].gpio;
}

// Đọc thẳng thanh ghi GPIO_IN (nhanh hơn digitalRead) cho vòng thăm dò bận
static inline int fastRead(const uint8_t gpio)
{
    return gpio < 32 ? (GPIO.in >> gpio) & 1 : (GPIO.in1.data >> (gpio - 32)) & 1;
}

static inline uint32_t cycles()
{
    return ESP.getCycleCount();
}

static void disableDac(const uint8_t gpio)
{
    if (gpio == 25) dac_output_disable(DAC_CHANNEL_1);
    if (gpio == 26) dac_output_disable(DAC_CHANNEL_2);
}

// ============================================================================
// API
// ============================================================================

int benchFindChannel(const String& token)
{
    String t = token;
    t.toUpperCase();
    if (t.startsWith("CH")) t = t.substring(2);

    if (t.length() > 0 && isDigit(t[0]))
    {
        const int number = t.toInt();
        return (number >= 1 && number <= BENCH_CHANNEL_COUNT) ? number - 1 : -1;
    }
    for (uint8_t i = 0; i < BENCH_CHANNEL_COUNT; i++)
    {
        if (t == BENCH_CHANNELS[i].net) return i;
    }
    return -1;
}

void benchReleaseAll()
{
    for (uint8_t i = 0; i < BENCH_CHANNEL_COUNT; i++)
    {
        disableDac(gpioOf(i));
        pinMode(gpioOf(i), INPUT);
        channelModes[i] = MODE_IN;
    }
}

bool benchSetMode(const int channel, const String& mode)
{
    const uint8_t gpio = gpioOf(channel);
    disableDac(gpio);

    ChannelMode next = MODE_IN;
    if (mode.equalsIgnoreCase("IN")) pinMode(gpio, INPUT);
    else if (mode.equalsIgnoreCase("PU")) pinMode(gpio, INPUT_PULLUP);
    else if (mode.equalsIgnoreCase("PD")) pinMode(gpio, INPUT_PULLDOWN);
    else if (mode.equalsIgnoreCase("OUT"))
    {
        pinMode(gpio, OUTPUT);
        next = MODE_OUT;
    }
    else if (mode.equalsIgnoreCase("OD"))
    {
        pinMode(gpio, OUTPUT_OPEN_DRAIN);
        next = MODE_OD;
    }
    else return false;
    channelModes[channel] = next;
    return true;
}

void benchWrite(const int channel, const int level)
{
    const uint8_t gpio = gpioOf(channel);
    // Kênh đang là input -> chuyển sang OUT đẩy kéo trước khi ghi (OD giữ nguyên chế độ)
    if (channelModes[channel] == MODE_IN)
    {
        disableDac(gpio);
        pinMode(gpio, OUTPUT);
        channelModes[channel] = MODE_OUT;
    }
    digitalWrite(gpio, level ? HIGH : LOW);
}

int benchRead(const int channel)
{
    return fastRead(gpioOf(channel));
}

uint32_t benchReadMilliVolts(const int channel)
{
    return analogReadMilliVolts(gpioOf(channel));
}

bool benchWriteDac(const int channel, const uint8_t value)
{
    const uint8_t gpio = gpioOf(channel);
    if (gpio != 25 && gpio != 26) return false;
    dacWrite(gpio, value);
    return true;
}

uint16_t benchCapture(const int channel, int& startLevel, uint32_t* durations, const uint16_t maxCount,
                      const uint32_t timeoutMs, const uint32_t idleUs, const bool nanoseconds)
{
    const uint8_t gpio = gpioOf(channel);
    const uint32_t cyclesPerUs = ESP.getCpuFreqMHz();
    const uint32_t idleCycles = idleUs * cyclesPerUs;

    startLevel = fastRead(gpio);
    int level = startLevel;
    uint16_t count = 0;

    // 1. Chờ sườn đầu tiên (không khóa ngắt: có thể chờ lâu)
    const uint32_t waitStart = millis();
    while (fastRead(gpio) == level)
    {
        if (millis() - waitStart >= timeoutMs) return 0;
    }

    // 2. Ghi lại độ dài từng đoạn mức logic, khóa ngắt để không lỡ sườn
    portDISABLE_INTERRUPTS();
    level = !level;
    uint32_t edge = cycles();
    const uint32_t lockStart = edge;
    const uint32_t lockLimit = CAPTURE_MAX_LOCKED_US * cyclesPerUs;
    while (count < maxCount && cycles() - lockStart < lockLimit)
    {
        const int now = fastRead(gpio);
        const uint32_t t = cycles();
        if (now != level)
        {
            durations[count++] = t - edge; // Lưu số chu kỳ, quy đổi sau khi mở ngắt
            level = now;
            edge = t;
        }
        else if (t - edge > idleCycles)
        {
            break; // Im lặng quá lâu: kết thúc khung tín hiệu
        }
    }
    portENABLE_INTERRUPTS();

    for (uint16_t i = 0; i < count; i++)
    {
        durations[i] = nanoseconds ? (uint32_t)((uint64_t)durations[i] * 1000 / cyclesPerUs) : durations[i] / cyclesPerUs;
    }

    // Đoạn cuối (đang giữ mức) không có sườn kết thúc nên không tính; startLevel là mức TRƯỚC sườn đầu tiên
    return count;
}

uint32_t benchMeasureFrequency(const int channel, const uint32_t windowMs)
{
    const uint8_t gpio = gpioOf(channel);
    uint32_t rising = 0;
    int last = fastRead(gpio);
    const uint32_t start = millis();
    while (millis() - start < windowMs)
    {
        const int now = fastRead(gpio);
        if (now && !last) rising++;
        last = now;
    }
    return (uint32_t)((uint64_t)rising * 1000 / windowMs);
}

// ============================================================================
// ĐO KHOẢNG GIỮA 2 SƯỜN LÊN (NGẮT)
// ============================================================================

static volatile int64_t intervalEdges[2];
static volatile uint8_t intervalCount = 0;

static void IRAM_ATTR onIntervalEdge()
{
    if (intervalCount < 2)
    {
        intervalEdges[intervalCount++] = esp_timer_get_time();
    }
}

int64_t benchMeasureInterval(const int channel, const uint32_t timeoutMs)
{
    const uint8_t gpio = gpioOf(channel);
    intervalCount = 0;
    attachInterrupt(gpio, onIntervalEdge, RISING);

    const uint32_t start = millis();
    while (intervalCount < 2 && millis() - start < timeoutMs)
    {
        delay(1); // Nhường CPU: tránh watchdog khi chờ lâu (tới vài phút)
    }
    detachInterrupt(gpio);
    return intervalCount < 2 ? -1 : intervalEdges[1] - intervalEdges[0];
}

#include <Arduino.h>
#include <driver/gpio.h>
#include "bench_pins.h"
#include "bench_channels.h"
#include "bench_i2c.h"

// Firmware bàn test HIL: nhận lệnh văn bản qua Serial 115200, mỗi lệnh 1 dòng, trả lời 1 dòng "OK ..." / "ERR ...".
// <ch> nhận số kênh (3), tên kênh (CH3) hoặc tên net (PD2).
//
//   PING                         -> OK PONG
//   PWR <chip|ALL> <ON|OFF>      -> bật/tắt nguồn chip (F4P6 / A4M6 / J4M6)
//   RST <chip> [ms]              -> tắt nguồn ms (mặc định 50) rồi bật lại: chip vào cửa sổ nạp 1 giây
//   STATUS                       -> trạng thái nguồn từng chip
//   Z ALL | Z <ch>               -> đưa kênh về Input thả nổi (Hi-Z)
//   MODE <ch> <IN|PU|PD|OUT|OD>  -> đặt chế độ kênh
//   W <ch> <0|1>                 -> ghi mức logic (tự chuyển sang OUT nếu đang là input)
//   ON <ch> / OFF <ch>           -> viết tắt của W <ch> 1 / W <ch> 0
//   R <ch>                       -> OK R <net> <0|1>
//   RA                           -> OK RA <16 ký tự 0/1, CH1 bên trái>
//   ADC <ch>                     -> OK ADC <net> <mV>
//   DAC <ch> <0..255>            -> xuất điện áp DAC (chỉ CH10 = GPIO26, CH11 = GPIO25)
//   CAP <ch> [timeoutMs] [idleUs] [NS] -> OK CAP <net> <mức trước sườn đầu> <n> <us1> <us2> ... (độ dài từng đoạn mức, NS = đơn vị ns)
//   FREQ <ch> [ms]               -> OK FREQ <net> <Hz>
//   INTERVAL <ch> [timeoutMs]    -> OK INTERVAL <net> <us giữa 2 sườn lên> (-1 nếu hết giờ), đo bằng ngắt
//   I2CS ON <addr hex> [Hz]      -> bật I2C slave trên CH13 (SDA) / CH14 (SCL); Z ALL sẽ tắt
//   I2CS OFF                     -> tắt I2C slave
//   I2CS LOAD <hex>              -> nạp sẵn dữ liệu cho lần master đọc kế tiếp (VD: LOAD A55A01, tối đa 32 byte)
//   I2CS RX                      -> OK I2CS RX <số giao dịch> <hex các byte master đã ghi, '-' nếu không có> (rồi xóa)

static const uint8_t MAX_WORDS = 5;
static String lineBuffer;
static uint32_t captureBuffer[BENCH_CAPTURE_MAX_EDGES];

// ============================================================================
// NGUỒN CHIP
// ============================================================================

static int findChip(const String& name)
{
    for (uint8_t i = 0; i < BENCH_CHIP_COUNT; i++)
    {
        if (name.equalsIgnoreCase(BENCH_CHIPS[i].name)) return i;
    }
    return -1;
}

static void setChipPower(const uint8_t index, const bool on)
{
    digitalWrite(BENCH_CHIPS[index].powerPin, on ? HIGH : LOW);
}

static void cmdStatus()
{
    Serial.print("OK STATUS");
    for (uint8_t i = 0; i < BENCH_CHIP_COUNT; i++)
    {
        Serial.printf(" %s=%s", BENCH_CHIPS[i].name, digitalRead(BENCH_CHIPS[i].powerPin) ? "ON" : "OFF");
    }
    Serial.println();
}

static void cmdPower(const String& target, const String& state)
{
    bool on;
    if (state.equalsIgnoreCase("ON")) on = true;
    else if (state.equalsIgnoreCase("OFF")) on = false;
    else
    {
        Serial.println("ERR PWR <chip|ALL> <ON|OFF>");
        return;
    }

    if (target.equalsIgnoreCase("ALL"))
    {
        for (uint8_t i = 0; i < BENCH_CHIP_COUNT; i++) setChipPower(i, on);
    }
    else
    {
        const int index = findChip(target);
        if (index < 0)
        {
            Serial.printf("ERR unknown chip '%s'\n", target.c_str());
            return;
        }
        setChipPower(index, on);
    }
    Serial.printf("OK PWR %s %s\n", target.c_str(), on ? "ON" : "OFF");
}

static void cmdReset(const String& target, const String& offMs)
{
    const int index = findChip(target);
    if (index < 0)
    {
        Serial.printf("ERR unknown chip '%s'\n", target.c_str());
        return;
    }
    const uint32_t ms = offMs.length() ? offMs.toInt() : BENCH_RESET_OFF_MS;
    setChipPower(index, false);
    delay(ms);
    setChipPower(index, true);
    Serial.printf("OK RST %s %lu\n", BENCH_CHIPS[index].name, (unsigned long)ms);
}

// ============================================================================
// KÊNH TÍN HIỆU
// ============================================================================

// Trả về chỉ số kênh, in lỗi và trả -1 nếu không hợp lệ
static int channelArg(const String& token)
{
    const int channel = benchFindChannel(token);
    if (channel < 0) Serial.printf("ERR unknown channel '%s'\n", token.c_str());
    return channel;
}

static void cmdRelease(const String& target)
{
    if (target.equalsIgnoreCase("ALL"))
    {
        benchI2cStop();
        benchReleaseAll();
        Serial.println("OK Z ALL");
        return;
    }
    const int ch = channelArg(target);
    if (ch < 0) return;
    benchSetMode(ch, "IN");
    Serial.printf("OK Z %s\n", BENCH_CHANNELS[ch].net);
}

static void cmdMode(const String& target, const String& mode)
{
    const int ch = channelArg(target);
    if (ch < 0) return;
    if (!benchSetMode(ch, mode))
    {
        Serial.println("ERR MODE <ch> <IN|PU|PD|OUT|OD>");
        return;
    }
    Serial.printf("OK MODE %s %s\n", BENCH_CHANNELS[ch].net, mode.c_str());
}

static void cmdWrite(const String& target, const int level)
{
    const int ch = channelArg(target);
    if (ch < 0) return;
    benchWrite(ch, level);
    Serial.printf("OK W %s %d\n", BENCH_CHANNELS[ch].net, level ? 1 : 0);
}

static void cmdRead(const String& target)
{
    const int ch = channelArg(target);
    if (ch < 0) return;
    Serial.printf("OK R %s %d\n", BENCH_CHANNELS[ch].net, benchRead(ch));
}

static void cmdReadAll()
{
    char bits[BENCH_CHANNEL_COUNT + 1];
    for (uint8_t i = 0; i < BENCH_CHANNEL_COUNT; i++) bits[i] = benchRead(i) ? '1' : '0';
    bits[BENCH_CHANNEL_COUNT] = '\0';
    Serial.printf("OK RA %s\n", bits);
}

static void cmdAdc(const String& target)
{
    const int ch = channelArg(target);
    if (ch < 0) return;
    Serial.printf("OK ADC %s %lu\n", BENCH_CHANNELS[ch].net, (unsigned long)benchReadMilliVolts(ch));
}

static void cmdDac(const String& target, const String& value)
{
    const int ch = channelArg(target);
    if (ch < 0) return;
    const int v = value.toInt();
    if (v < 0 || v > 255 || !benchWriteDac(ch, (uint8_t)v))
    {
        Serial.println("ERR DAC chỉ có ở CH10/CH11, giá trị 0..255");
        return;
    }
    Serial.printf("OK DAC %s %d\n", BENCH_CHANNELS[ch].net, v);
}

static void cmdCapture(const String& target, const String& timeoutMs, const String& idleUs, const String& unit)
{
    const int ch = channelArg(target);
    if (ch < 0) return;
    int startLevel = 0;
    const uint16_t n = benchCapture(ch, startLevel, captureBuffer, BENCH_CAPTURE_MAX_EDGES,
                                    timeoutMs.length() ? timeoutMs.toInt() : 1000,
                                    idleUs.length() ? idleUs.toInt() : 20000, unit.equalsIgnoreCase("NS"));
    Serial.printf("OK CAP %s %d %u", BENCH_CHANNELS[ch].net, startLevel, n);
    for (uint16_t i = 0; i < n; i++) Serial.printf(" %lu", (unsigned long)captureBuffer[i]);
    Serial.println();
}

static void cmdInterval(const String& target, const String& timeoutMs)
{
    const int ch = channelArg(target);
    if (ch < 0) return;
    const uint32_t ms = timeoutMs.length() ? timeoutMs.toInt() : 10000;
    Serial.printf("OK INTERVAL %s %lld\n", BENCH_CHANNELS[ch].net, (long long)benchMeasureInterval(ch, ms));
}

static void cmdFrequency(const String& target, const String& windowMs)
{
    const int ch = channelArg(target);
    if (ch < 0) return;
    const uint32_t ms = windowMs.length() ? windowMs.toInt() : 1000;
    Serial.printf("OK FREQ %s %lu\n", BENCH_CHANNELS[ch].net, (unsigned long)benchMeasureFrequency(ch, ms));
}

// ============================================================================
// I2C SLAVE
// ============================================================================

static int hexNibble(const char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static void cmdI2cSlave(const String& action, const String& arg1, const String& arg2)
{
    if (action.equalsIgnoreCase("ON"))
    {
        const uint8_t address = strtoul(arg1.c_str(), NULL, 16);
        const uint32_t hz = arg2.length() ? arg2.toInt() : 100000;
        if (address < 0x08 || address > 0x77 || !benchI2cStart(address, hz))
        {
            Serial.println("ERR I2CS ON <addr hex 08..77> [Hz]");
            return;
        }
        Serial.printf("OK I2CS ON %02X\n", address);
    }
    else if (action.equalsIgnoreCase("OFF"))
    {
        benchI2cStop();
        Serial.println("OK I2CS OFF");
    }
    else if (action.equalsIgnoreCase("LOAD"))
    {
        uint8_t data[32];
        size_t n = 0;
        for (size_t i = 0; i + 1 < arg1.length() && n < sizeof(data); i += 2)
        {
            const int hi = hexNibble(arg1[i]);
            const int lo = hexNibble(arg1[i + 1]);
            if (hi < 0 || lo < 0) break;
            data[n++] = (hi << 4) | lo;
        }
        if (!benchI2cActive() || n * 2 != arg1.length())
        {
            Serial.println("ERR I2CS LOAD <hex> (cần I2CS ON trước)");
            return;
        }
        Serial.printf("OK I2CS LOAD %u\n", (unsigned)benchI2cLoad(data, n));
    }
    else if (action.equalsIgnoreCase("RX"))
    {
        uint8_t data[BENCH_I2C_RX_MAX];
        uint16_t transactions = 0;
        const uint16_t n = benchI2cTakeReceived(data, sizeof(data), transactions);
        Serial.printf("OK I2CS RX %u ", transactions);
        if (n == 0) Serial.print('-');
        for (uint16_t i = 0; i < n; i++) Serial.printf("%02X", data[i]);
        Serial.println();
    }
    else
    {
        Serial.println("ERR I2CS <ON|OFF|LOAD|RX>");
    }
}

// ============================================================================
// BỘ PHÂN TÍCH LỆNH
// ============================================================================

static void handleLine(String line)
{
    line.trim();
    if (line.isEmpty()) return;

    String w[MAX_WORDS];
    uint8_t count = 0;
    int start = 0;
    while (count < MAX_WORDS && start < (int)line.length())
    {
        int end = line.indexOf(' ', start);
        if (end < 0) end = line.length();
        if (end > start) w[count++] = line.substring(start, end);
        start = end + 1;
    }

    String cmd = w[0];
    cmd.toUpperCase();
    if (cmd == "PING") Serial.println("OK PONG");
    else if (cmd == "PWR") cmdPower(w[1], w[2]);
    else if (cmd == "RST") cmdReset(w[1], w[2]);
    else if (cmd == "STATUS") cmdStatus();
    else if (cmd == "Z") cmdRelease(w[1]);
    else if (cmd == "MODE") cmdMode(w[1], w[2]);
    else if (cmd == "W") cmdWrite(w[1], w[2].toInt());
    else if (cmd == "ON") cmdWrite(w[1], 1);
    else if (cmd == "OFF") cmdWrite(w[1], 0);
    else if (cmd == "R") cmdRead(w[1]);
    else if (cmd == "RA") cmdReadAll();
    else if (cmd == "ADC") cmdAdc(w[1]);
    else if (cmd == "DAC") cmdDac(w[1], w[2]);
    else if (cmd == "CAP") cmdCapture(w[1], w[2], w[3], w[4]);
    else if (cmd == "FREQ") cmdFrequency(w[1], w[2]);
    else if (cmd == "INTERVAL") cmdInterval(w[1], w[2]);
    else if (cmd == "I2CS") cmdI2cSlave(w[1], w[2], w[3]);
    else Serial.printf("ERR unknown command '%s'\n", w[0].c_str());
}

void setup()
{
    Serial.begin(115200);
    benchReleaseAll();

    // Cấp nguồn cả 3 chip ngay khi khởi động: chip không test sẽ tự đỗ (Hi-Z, tắt SWD sau 1 giây)
    for (uint8_t i = 0; i < BENCH_CHIP_COUNT; i++)
    {
        pinMode(BENCH_CHIPS[i].powerPin, OUTPUT);
        gpio_set_drive_capability((gpio_num_t)BENCH_CHIPS[i].powerPin, GPIO_DRIVE_CAP_3); // ~40mA
        setChipPower(i, true);
    }

    Serial.println("OK READY HIL-BENCH");
}

void loop()
{
    while (Serial.available())
    {
        const char c = (char)Serial.read();
        if (c == '\n' || c == '\r')
        {
            handleLine(lineBuffer);
            lineBuffer = "";
        }
        else if (lineBuffer.length() < 120)
        {
            lineBuffer += c;
        }
    }
}

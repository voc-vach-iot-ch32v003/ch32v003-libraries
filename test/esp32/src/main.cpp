#include <Arduino.h>
#include <driver/gpio.h>
#include "bench_pins.h"
#include "bench_channels.h"
#include "bench_i2c.h"
#include "bench_spi.h"
#include "bench_ir.h"

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
//   SPIS ON <sck> <mosi> <miso> <cs> -> bật SPI slave Mode 0 trên 4 kênh; Z ALL sẽ tắt
//   SPIS OFF                     -> tắt SPI slave
//   SPIS LOAD <hex|->            -> nạp 1 giao dịch: dữ liệu đẩy ra MISO (tối đa 64 byte, '-' = toàn 0x00)
//   SPIS RX [timeoutMs]          -> OK SPIS RX <số bit> <hex các byte master đã gửi> (ERR nếu giao dịch chưa xong)
//   SNIFF <sck> <mosi> <cs> [timeoutMs] [idleUs] -> OK SNIFF <số khung> <hex khung 1> ... gap=<ns lượt lấy mẫu chậm nhất>
//                                   nghe lén SPI Mode 0 (thụ động, SCK tới ~750kHz), chờ CS xuống tối đa 100ms,
//                                   dừng khi CS nghỉ HIGH quá idleUs (mặc định 5000);
//                                   khung lẻ bit ghi thêm ":<số bit>"
//   IRTX <ch> <NEC|SAMSUNG|SONY|TCL> <hex> [khung] -> phát khung IR đã giải điều chế như mắt thu (rảnh HIGH, Mark = LOW),
//                                   bit 0 của byte đầu phát trước; khung > 1: NEC thêm mã lặp, SAMSUNG/SONY lặp nguyên khung
//   IRCAP <ch> [timeoutMs] [gapUs] -> OK IRCAP <net> <Hz sóng mang> <duty %> <n> <mark> <space> <mark>... (us),
//                                   bắt tín hiệu có sóng mang (rảnh LOW), dừng khi LOW quá gapUs (mặc định 20000)

static const uint8_t MAX_WORDS = 6;
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
        benchSpiStop();
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

// Đổi chuỗi hex (VD "A55A01") thành mảng byte. Trả -1 nếu sai định dạng hoặc quá dài.
static int parseHex(const String& text, uint8_t* out, const size_t maxLength)
{
    if (text == "-") return 0;
    if (text.length() % 2 || text.length() / 2 > maxLength) return -1;
    for (size_t i = 0; i < text.length(); i += 2)
    {
        const int hi = hexNibble(text[i]);
        const int lo = hexNibble(text[i + 1]);
        if (hi < 0 || lo < 0) return -1;
        out[i / 2] = (hi << 4) | lo;
    }
    return text.length() / 2;
}

static void printHex(const uint8_t* data, const size_t length)
{
    if (length == 0) Serial.print('-');
    for (size_t i = 0; i < length; i++) Serial.printf("%02X", data[i]);
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
        const int n = parseHex(arg1, data, sizeof(data));
        if (!benchI2cActive() || n < 0)
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
        printHex(data, n);
        Serial.println();
    }
    else
    {
        Serial.println("ERR I2CS <ON|OFF|LOAD|RX>");
    }
}

// ============================================================================
// SPI SLAVE
// ============================================================================

static void cmdSpiSlave(const String* w)
{
    const String& action = w[1];
    if (action.equalsIgnoreCase("ON"))
    {
        int ch[4];
        for (uint8_t i = 0; i < 4; i++)
        {
            ch[i] = channelArg(w[2 + i]);
            if (ch[i] < 0) return;
        }
        benchI2cStop();
        if (!benchSpiStart(BENCH_CHANNELS[ch[0]].gpio, BENCH_CHANNELS[ch[1]].gpio,
                           BENCH_CHANNELS[ch[2]].gpio, BENCH_CHANNELS[ch[3]].gpio))
        {
            Serial.println("ERR SPIS ON thất bại");
            return;
        }
        Serial.printf("OK SPIS ON SCK=%s MOSI=%s MISO=%s CS=%s\n", BENCH_CHANNELS[ch[0]].net,
                      BENCH_CHANNELS[ch[1]].net, BENCH_CHANNELS[ch[2]].net, BENCH_CHANNELS[ch[3]].net);
    }
    else if (action.equalsIgnoreCase("OFF"))
    {
        benchSpiStop();
        Serial.println("OK SPIS OFF");
    }
    else if (action.equalsIgnoreCase("LOAD"))
    {
        uint8_t data[BENCH_SPI_MAX_BYTES];
        const int n = parseHex(w[2], data, sizeof(data));
        if (n < 0 || !benchSpiLoad(data, n))
        {
            Serial.println("ERR SPIS LOAD <hex|-> (cần SPIS ON trước, tối đa 64 byte)");
            return;
        }
        Serial.printf("OK SPIS LOAD %d\n", n);
    }
    else if (action.equalsIgnoreCase("RX"))
    {
        uint8_t data[BENCH_SPI_MAX_BYTES];
        uint32_t bits = 0;
        const int n = benchSpiTakeReceived(data, sizeof(data), w[2].length() ? w[2].toInt() : 100, bits);
        if (n < 0)
        {
            Serial.println("ERR SPIS RX chưa có giao dịch hoàn tất");
            return;
        }
        Serial.printf("OK SPIS RX %lu ", (unsigned long)bits);
        printHex(data, n);
        Serial.println();
    }
    else
    {
        Serial.println("ERR SPIS <ON|OFF|LOAD|RX>");
    }
}

static void cmdSpiSniff(const String* w)
{
    int ch[3];
    for (uint8_t i = 0; i < 3; i++)
    {
        ch[i] = channelArg(w[1 + i]);
        if (ch[i] < 0) return;
    }
    static uint8_t bytes[BENCH_SNIFF_MAX_BYTES];
    static uint16_t frameBits[BENCH_SNIFF_MAX_FRAMES];
    uint32_t maxGapNs = 0;
    const uint8_t frames = benchSpiSniff(BENCH_CHANNELS[ch[0]].gpio, BENCH_CHANNELS[ch[1]].gpio, BENCH_CHANNELS[ch[2]].gpio,
                                         w[4].length() ? w[4].toInt() : 100, w[5].length() ? w[5].toInt() : 5000,
                                         bytes, frameBits, maxGapNs);
    Serial.printf("OK SNIFF %u", frames);
    uint16_t offset = 0;
    for (uint8_t f = 0; f < frames; f++)
    {
        const uint16_t n = (frameBits[f] + 7) / 8;
        Serial.print(' ');
        printHex(bytes + offset, n);
        if (frameBits[f] % 8) Serial.printf(":%u", frameBits[f]);
        offset += n;
    }
    Serial.printf(" gap=%lu\n", (unsigned long)maxGapNs);
}

// ============================================================================
// HỒNG NGOẠI
// ============================================================================

static void cmdIrSend(const String& target, const String& protocol, const String& hex, const String& frames)
{
    const int ch = channelArg(target);
    if (ch < 0) return;
    uint8_t data[16];
    const int n = parseHex(hex, data, sizeof(data));
    if (n <= 0 || !benchIrSend(ch, protocol, data, n, frames.length() ? frames.toInt() : 1))
    {
        Serial.println("ERR IRTX <ch> <NEC|SAMSUNG|SONY|TCL> <hex đủ số bit> [khung]");
        return;
    }
    Serial.printf("OK IRTX %s\n", BENCH_CHANNELS[ch].net);
}

static void cmdIrCapture(const String& target, const String& timeoutMs, const String& gapUs)
{
    const int ch = channelArg(target);
    if (ch < 0) return;
    uint32_t hz = 0, duty = 0;
    const uint16_t n = benchIrCapture(ch, captureBuffer, BENCH_CAPTURE_MAX_EDGES, timeoutMs.length() ? timeoutMs.toInt() : 1000,
                                      gapUs.length() ? gapUs.toInt() : 20000, hz, duty);
    Serial.printf("OK IRCAP %s %lu %lu %u", BENCH_CHANNELS[ch].net, (unsigned long)hz, (unsigned long)duty, n);
    for (uint16_t i = 0; i < n; i++) Serial.printf(" %lu", (unsigned long)captureBuffer[i]);
    Serial.println();
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
    else if (cmd == "SPIS") cmdSpiSlave(w);
    else if (cmd == "SNIFF") cmdSpiSniff(w);
    else if (cmd == "IRTX") cmdIrSend(w[1], w[2], w[3], w[4]);
    else if (cmd == "IRCAP") cmdIrCapture(w[1], w[2], w[3]);
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

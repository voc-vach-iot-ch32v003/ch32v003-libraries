#include <Arduino.h>
#include <EEPROM.h>

// Máy đo dòng tiêu thụ cho bàn test HIL. Dòng của chip đi từ chân GND của chip (nút A, A0) qua R2 (22Ω) xuống
// nút X (A1), qua R1 (10kΩ) xuống GND chung. Q1 (8050, C ở X, E ở GND) nối tắt R1 khi D2 = HIGH: dòng base của Q1
// đi thẳng xuống GND, KHÔNG qua trở đo nào (bản đầu đặt R2 dưới Q1 nên đo dư ~3.9mA dòng base).
//   Thang µA (Q1 tắt):  I = V_A1 / R1               ~0.5 .. 40µA
//   Thang mA (Q1 dẫn):  I = (V_A0 - V_A1) / R2      ~40µA .. 45mA
// Mặc định Q1 dẫn (thang mA) để chip khởi động / nạp code an toàn; trở kéo lên ở D2 giữ Q1 dẫn cả khi Uno reset.
//
// Lệnh Serial 115200 (mỗi lệnh 1 dòng, trả lời "OK ..." / "ERR ..."):
//   PING                   -> OK PONG METER
//   I [ms] [AUTO|UA|MA]    -> đo trung bình trong ms (mặc định 500): OK I <µA> <UA|MA> <mV_A0> <mV_A1> <số mẫu>
//                             AUTO: đo thang mA trước, < 40µA thì chuyển sang thang µA. Quá tải thang µA -> OK I ... OVER
//   CAL                    -> OK CAL <mV AREF> <R1 Ω> <R2 Ω>
//   CAL AREF <mV>          -> lưu điện áp tham chiếu nội đo được ở chân AREF (đồng hồ thang DCV)
//   CAL R1 <Ω> / CAL R2 <Ω> -> lưu giá trị thật của trở đo (đồng hồ thang Ω)

static const uint8_t PIN_NODE_A = A0;
static const uint8_t PIN_NODE_B = A1;
static const uint8_t PIN_BYPASS = 2;

static const float UA_RANGE_LIMIT = 40.0f; // Dưới mức này mới dùng thang µA
static const float OVERLOAD_MV = 400.0f; // Điện áp trên R1 (A1) vượt mức này -> chip đang thức, bật lại Q1 ngay
static const uint16_t SETTLE_MS = 20; // Chờ sau khi đổi thang

struct Calibration
{
    uint16_t magic;
    float arefMilliVolts;
    float r1Ohms;
    float r2Ohms;
};

static const uint16_t CAL_MAGIC = 0xC1A1;
static Calibration cal = {CAL_MAGIC, 1100.0f, 10000.0f, 22.2f};
static String lineBuffer;

// ============================================================================
// ĐO
// ============================================================================

static void setBypass(const bool on)
{
    digitalWrite(PIN_BYPASS, on ? HIGH : LOW);
}

static float toMilliVolts(const uint32_t sum, const uint32_t count)
{
    return (float)sum / count * cal.arefMilliVolts / 1024.0f;
}

struct Reading
{
    float microAmps;
    float milliVoltsA;
    float milliVoltsB;
    uint32_t samples;
    bool overload;
};

// Lấy trung bình A0 / A1 trong durationMs. Thang µA: dừng ngay nếu quá tải (chip thức) để bảo vệ chip
static Reading sample(const bool microRange, const uint32_t durationMs)
{
    uint32_t sumA = 0, sumB = 0, count = 0;
    const uint16_t overloadCounts = (uint16_t)(OVERLOAD_MV / cal.arefMilliVolts * 1024.0f);
    Reading r = {0, 0, 0, 0, false};

    const uint32_t start = millis();
    do
    {
        const uint16_t a = analogRead(PIN_NODE_A);
        const uint16_t b = analogRead(PIN_NODE_B);
        if (microRange && b > overloadCounts)
        {
            setBypass(true);
            r.overload = true;
            break;
        }
        sumA += a;
        sumB += b;
        count++;
    } while (millis() - start < durationMs);

    if (count == 0) count = 1;
    r.samples = count;
    r.milliVoltsA = toMilliVolts(sumA, count);
    r.milliVoltsB = toMilliVolts(sumB, count);
    r.microAmps = microRange ? r.milliVoltsB / cal.r1Ohms * 1000.0f
                             : (r.milliVoltsA - r.milliVoltsB) / cal.r2Ohms * 1000.0f;
    return r;
}

static void cmdMeasure(const String& durationText, String mode)
{
    const uint32_t durationMs = durationText.length() ? durationText.toInt() : 500;
    mode.toUpperCase();
    if (!mode.length()) mode = "AUTO";

    bool microRange = mode == "UA";
    if (mode == "AUTO")
    {
        // Ước lượng nhanh ở thang mA (an toàn) rồi mới quyết định
        setBypass(true);
        delay(SETTLE_MS);
        microRange = sample(false, 50).microAmps < UA_RANGE_LIMIT;
    }

    setBypass(!microRange);
    delay(SETTLE_MS);
    const Reading r = sample(microRange, durationMs);
    setBypass(true); // Luôn trả về thang mA (an toàn) sau khi đo

    Serial.print(F("OK I "));
    Serial.print(r.microAmps, 2);
    Serial.print(microRange ? F(" UA ") : F(" MA "));
    Serial.print(r.milliVoltsA, 2);
    Serial.print(' ');
    Serial.print(r.milliVoltsB, 2);
    Serial.print(' ');
    Serial.print(r.samples);
    if (r.overload) Serial.print(F(" OVER"));
    Serial.println();
}

// ============================================================================
// HIỆU CHUẨN
// ============================================================================

static void printCalibration()
{
    Serial.print(F("OK CAL "));
    Serial.print(cal.arefMilliVolts, 1);
    Serial.print(' ');
    Serial.print(cal.r1Ohms, 1);
    Serial.print(' ');
    Serial.println(cal.r2Ohms, 2);
}

static void cmdCalibrate(String key, const String& value)
{
    key.toUpperCase();
    const float v = value.toFloat();
    if (key.length() == 0)
    {
        printCalibration();
        return;
    }
    if (v <= 0)
    {
        Serial.println(F("ERR CAL AREF <mV> | CAL R1 <ohm> | CAL R2 <ohm>"));
        return;
    }
    if (key == "AREF") cal.arefMilliVolts = v;
    else if (key == "R1") cal.r1Ohms = v;
    else if (key == "R2") cal.r2Ohms = v;
    else
    {
        Serial.println(F("ERR CAL <AREF|R1|R2> <giá trị>"));
        return;
    }
    EEPROM.put(0, cal);
    printCalibration();
}

// ============================================================================
// BỘ PHÂN TÍCH LỆNH
// ============================================================================

static void handleLine(String line)
{
    line.trim();
    if (!line.length()) return;

    String w[3];
    uint8_t count = 0;
    int start = 0;
    while (count < 3 && start < (int)line.length())
    {
        int end = line.indexOf(' ', start);
        if (end < 0) end = line.length();
        if (end > start) w[count++] = line.substring(start, end);
        start = end + 1;
    }
    w[0].toUpperCase();

    if (w[0] == "PING") Serial.println(F("OK PONG METER"));
    else if (w[0] == "I") cmdMeasure(w[1], w[2]);
    else if (w[0] == "CAL") cmdCalibrate(w[1], w[2]);
    else
    {
        Serial.print(F("ERR unknown command '"));
        Serial.print(w[0]);
        Serial.println('\'');
    }
}

void setup()
{
    pinMode(PIN_BYPASS, OUTPUT);
    setBypass(true); // Thang mA: chip khởi động / nạp code an toàn

    analogReference(INTERNAL); // Tham chiếu nội 1.1V: ~1.07mV mỗi bước
    analogRead(PIN_NODE_A); // Lần đọc đầu sau khi đổi tham chiếu không chính xác
    delay(10);

    Calibration stored;
    EEPROM.get(0, stored);
    if (stored.magic == CAL_MAGIC) cal = stored;

    Serial.begin(115200);
    Serial.println(F("OK READY METER"));
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
        else if (lineBuffer.length() < 40)
        {
            lineBuffer += c;
        }
    }
}

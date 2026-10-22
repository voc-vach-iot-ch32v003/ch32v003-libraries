#include "ch32fun.h"
#include "ch32v003_gpio.h"
#include "ch32v003_ir.h"
#include "hil.h"

// Test HIL cho ch32v003_ir (cả 3 gói vỏ):
// - Thu: ESP32 giả lập ngõ ra mắt thu (IRTX, rảnh HIGH, Mark = LOW) trên PC4 -> irRead / irReadRaw.
// - Phát: chip phát có sóng mang trên PC1 -> ESP32 bắt đường bao + tần số sóng mang (IRCAP), runner so với chuẩn.
// - Học lệnh: thu thô một khung NEC và 11 khung Sony (285 sườn, vượt giới hạn 256 sườn của bản cũ) rồi phát lại.
// Bước "rx <tag> <giao thức> <hex> <số khung>": runner trả lời rồi bảo ESP32 phát.
// Bước "tx <tag> <giao thức> <hex> <số khung> <Hz>": runner bật IRCAP, trả lời, chip phát, runner so sánh.

#define PIN_C(n) ((MCU_PORT_INDEX_C << 4) | (n))
#define RX_PIN PIN_C(4)
#define TX_PIN PIN_C(1)

#define RECEIVE_TIMEOUT_MS 2000

// Khung NEC address 0x04, command 0x08 (04 FB 08 F7) đặt sẵn trong Flash, như câu khởi tạo irAnalyzeRaw() in ra
static const uint16_t necCodeDurations[] = {9000, 4500, 560, 1690};
static const uint8_t necCodeSymbols[] = {
    0x10, 0x22, 0x22, 0x32, 0x22, 0x22, 0x22, 0x22, 0x22, 0x32, 0x32, 0x22, 0x32, 0x32, 0x32, 0x32, 0x32,
    0x22, 0x22, 0x22, 0x32, 0x22, 0x22, 0x22, 0x22, 0x32, 0x32, 0x32, 0x22, 0x32, 0x32, 0x32, 0x32, 0x02,
};
static const IR_RawCode_t necCode = {necCodeDurations, necCodeSymbols, 67};

static IR_RawData_t learned; // Toàn cục: tránh ~300 byte trên stack của test
static char text[48];
static char name[24];
static char detail[64];

typedef struct
{
    const char* tag;
    const char* protocolName;
    const char* hex;
    uint8_t frames;
    IR_Protocol_t protocol;
    uint16_t address;
    uint32_t command;
} RxCase_t;

static const RxCase_t rxCases[] = {
    {"nec", "NEC", "04FB08F7", 1, IR_NEC, 0x04, 0x08},
    {"nec_ext", "NEC", "341256A9", 1, IR_NEC, 0x1234, 0x56},
    {"nec_repeat", "NEC", "04FB08F7", 3, IR_NEC, 0x04, 0x08},
    {"samsung", "SAMSUNG", "070702FD", 1, IR_SAMSUNG, 0x0707, 0x02},
    {"sony", "SONY", "9500", 1, IR_SONY, 0x01, 0x15},
    {"sony_x3", "SONY", "9500", 3, IR_SONY, 0x01, 0x15},
    {"tcl", "TCL", "123456789ABCDEF01203", 1, IR_TCL, 0x3412, 0xBC9A7856},
};

static void toHex(const uint8_t* data, const uint8_t length, char* out)
{
    static const char digits[] = "0123456789ABCDEF";
    for (uint8_t i = 0; i < length; i++)
    {
        out[2 * i] = digits[data[i] >> 4];
        out[2 * i + 1] = digits[data[i] & 0x0F];
    }
    out[2 * length] = '\0';
}

static uint8_t elapsedMs(const uint32_t start, const uint32_t ms)
{
    return SysTick->CNT - start >= ms * DELAY_MS_TIME;
}

static void stepRx(const char* tag, const char* protocolName, const char* hex, const uint8_t frames)
{
    mini_snprintf(text, sizeof(text), "rx %s %s %s %u", tag, protocolName, hex, frames);
    hilWait(text);
}

static void stepTx(const char* tag, const char* protocolName, const char* hex, const uint8_t frames, const uint32_t hz)
{
    mini_snprintf(text, sizeof(text), "tx %s %s %s %u %lu", tag, protocolName, hex, frames, (unsigned long)hz);
    hilWait(text);
}

static void check(const char* what, const char* tag, const int passed, const char* info)
{
    mini_snprintf(name, sizeof(name), "%s_%s", what, tag);
    hilCheck(name, passed, info);
}

// ============================================================================
// THU + GIẢI MÃ
// ============================================================================

static void testReceive(const RxCase_t* c)
{
    IR_Data_t data;
    stepRx(c->tag, c->protocolName, c->hex, c->frames);
    uint8_t ok = 0;
    const uint32_t start = SysTick->CNT;
    while (!ok && !elapsedMs(start, RECEIVE_TIMEOUT_MS))
    {
        ok = irRead(RX_PIN, &data);
    }
    if (!ok)
    {
        check("rx", c->tag, 0, "khong giai ma duoc");
        return;
    }

    char hex[2 * IR_RAW_DATA_BYTES + 1];
    toHex(data.rawData, (uint8_t)((data.bits + 7) >> 3), hex);
    // So rawData với chuỗi hex đã gửi (TCL 76 bit: 4 bit cao của byte cuối luôn 0, chuỗi gửi cũng vậy)
    uint8_t sameBytes = 1;
    for (uint8_t i = 0; c->hex[i]; i++)
    {
        if (hex[i] != c->hex[i]) sameBytes = 0;
    }
    mini_snprintf(detail, sizeof(detail), "proto=%u addr=0x%X cmd=0x%lX data=%s", data.protocol, data.address,
                  (unsigned long)data.command, hex);
    check("rx", c->tag,
          data.protocol == c->protocol && data.address == c->address && data.command == c->command && sameBytes,
          detail);
}

// ============================================================================
// HỌC LỆNH (THU THÔ) + PHÁT LẠI
// ============================================================================

static uint8_t learn(const char* tag, const char* protocolName, const char* hex, const uint8_t frames)
{
    stepRx(tag, protocolName, hex, frames);
    uint8_t ok = 0;
    const uint32_t start = SysTick->CNT;
    while (!ok && !elapsedMs(start, RECEIVE_TIMEOUT_MS))
    {
        ok = irReadRaw(RX_PIN, &learned);
    }
    return ok;
}

static uint8_t near(const uint16_t value, const uint16_t target)
{
    // Clock HSI của chip lệch tới ±2% so với thạch anh ESP32, thêm 30us cho độ trễ phát hiện sườn
    const uint16_t delta = target / 25 + 30;
    return value + delta >= target && value <= target + delta;
}

static void testLearnNec(void)
{
    if (!learn("learn_nec", "NEC", "04FB08F7", 1))
    {
        check("learn", "nec", 0, "irReadRaw khong thu duoc");
        return;
    }
    mini_snprintf(detail, sizeof(detail), "edges=%u durations=%u leader=%u/%u", learned.rawLen, learned.durationCount,
                  irRawGetDuration(&learned, 0), irRawGetDuration(&learned, 1));
    check("learn", "nec", learned.rawLen == 67 && learned.durationCount <= 4 && near(irRawGetDuration(&learned, 0), 9000) &&
                              near(irRawGetDuration(&learned, 1), 4500) && near(irRawGetDuration(&learned, 2), 560) &&
                              near(irRawGetDuration(&learned, 66), 560),
          detail);

    IR_Data_t data;
    const uint8_t decoded = irDecodeRaw(&learned, &data);
    mini_snprintf(detail, sizeof(detail), "proto=%u addr=0x%X cmd=0x%lX", data.protocol, data.address,
                  (unsigned long)data.command);
    check("decode_raw", "nec", decoded && data.protocol == IR_NEC && data.address == 0x04 && data.command == 0x08, detail);

    irAnalyzeRaw(&learned); // Kết quả in nằm trong log của runner (kiểm tra bằng mắt)

    stepTx("replay_nec", "NEC", "04FB08F7", 1, IR_RAW_CARRIER_FREQUENCY_HZ);
    irSendRaw(TX_PIN, &learned);
}

static void testLearnSonyBurst(void)
{
    if (!learn("learn_sony11", "SONY", "9500", 11))
    {
        check("learn", "sony11", 0, "irReadRaw khong thu duoc");
        return;
    }
    // 11 khung x 25 sườn + 10 khoảng nghỉ giữa các khung
    mini_snprintf(detail, sizeof(detail), "edges=%u durations=%u", learned.rawLen, learned.durationCount);
    check("learn", "sony11", learned.rawLen == 285 && learned.durationCount <= 5, detail);

    IR_Data_t data;
    const uint8_t decoded = irDecodeRaw(&learned, &data);
    mini_snprintf(detail, sizeof(detail), "proto=%u addr=0x%X cmd=0x%lX", data.protocol, data.address,
                  (unsigned long)data.command);
    check("decode_raw", "sony11", decoded && data.protocol == IR_SONY && data.address == 0x01 && data.command == 0x15,
          detail);

    stepTx("replay_sony11", "SONY", "9500", 11, IR_RAW_CARRIER_FREQUENCY_HZ);
    irSendRaw(TX_PIN, &learned);
}

// ============================================================================
// PHÁT
// ============================================================================

static void testSend(void)
{
    stepTx("send_nec", "NEC", "04FB08F7", 1, 38000);
    irSend(TX_PIN, IR_NEC, 0x04, 0x08);

    stepTx("send_nec_ext", "NEC", "341256A9", 1, 38000);
    irSend(TX_PIN, IR_NEC, 0x1234, 0x56);

    stepTx("send_samsung", "SAMSUNG", "070702FD", 1, 38000);
    irSend(TX_PIN, IR_SAMSUNG, 0x0707, 0x02);

    stepTx("send_sony", "SONY", "9500", 1, 40000);
    irSend(TX_PIN, IR_SONY, 0x01, 0x15);

    // TCL 76 bit: phát nguyên gói bằng irSendData()
    static const IR_Data_t tcl = {
        .protocol = IR_TCL,
        .rawData = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x12, 0x03},
        .bits = 76,
    };
    stepTx("send_tcl", "TCL", "123456789ABCDEF01203", 1, 38000);
    irSendData(TX_PIN, &tcl);

    stepTx("code_nec", "NEC", "04FB08F7", 1, IR_RAW_CARRIER_FREQUENCY_HZ);
    irSendRawCode(TX_PIN, &necCode);
}

int main(void)
{
    SystemInit();
    Delay_Ms(1000); // Cửa sổ nạp

    pinMode(RX_PIN, INPUT_PULLUP);
    pinMode(TX_PIN, OUTPUT);
    digitalWrite(TX_PIN, LOW);

    hilBegin("ir");
    hilWait("setup");

    for (uint8_t i = 0; i < sizeof(rxCases) / sizeof(rxCases[0]); i++)
    {
        testReceive(&rxCases[i]);
    }
    testLearnNec();
    testLearnSonyBurst();
    testSend();

    hilWait("done");
    hilEnd();
}

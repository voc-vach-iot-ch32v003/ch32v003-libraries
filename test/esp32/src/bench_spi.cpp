#include "bench_spi.h"
#include <driver/spi_slave.h>
#include <soc/gpio_struct.h>

static const spi_host_device_t HOST = HSPI_HOST;

static bool active = false;
static bool pending = false;
static uint8_t pins[4];
static WORD_ALIGNED_ATTR uint8_t txBuffer[BENCH_SPI_MAX_BYTES];
static WORD_ALIGNED_ATTR uint8_t rxBuffer[BENCH_SPI_MAX_BYTES];
static spi_slave_transaction_t transaction;

bool benchSpiStart(const uint8_t sckGpio, const uint8_t mosiGpio, const uint8_t misoGpio, const uint8_t csGpio)
{
    benchSpiStop();

    spi_bus_config_t bus = {};
    bus.mosi_io_num = mosiGpio;
    bus.miso_io_num = misoGpio;
    bus.sclk_io_num = sckGpio;
    bus.quadwp_io_num = -1;
    bus.quadhd_io_num = -1;

    spi_slave_interface_config_t slave = {};
    slave.mode = 0;
    slave.spics_io_num = csGpio;
    slave.queue_size = 1;

    // CS thả nổi khi master chưa khởi tạo: kéo lên để slave không tự bắt đầu giao dịch
    gpio_set_pull_mode((gpio_num_t)csGpio, GPIO_PULLUP_ONLY);
    active = spi_slave_initialize(HOST, &bus, &slave, SPI_DMA_DISABLED) == ESP_OK;
    pending = false;
    pins[0] = sckGpio;
    pins[1] = mosiGpio;
    pins[2] = misoGpio;
    pins[3] = csGpio;
    return active;
}

void benchSpiStop()
{
    if (!active) return;
    spi_slave_free(HOST);
    for (uint8_t i = 0; i < 4; i++) pinMode(pins[i], INPUT);
    active = false;
    pending = false;
}

bool benchSpiActive()
{
    return active;
}

bool benchSpiLoad(const uint8_t* data, const size_t length)
{
    if (!active || length > BENCH_SPI_MAX_BYTES) return false;
    // Giao dịch cũ chưa diễn ra thì không rút khỏi hàng đợi được: khởi động lại slave để hủy
    if (pending && !benchSpiStart(pins[0], pins[1], pins[2], pins[3])) return false;
    memset(txBuffer, 0, sizeof(txBuffer));
    memset(rxBuffer, 0, sizeof(rxBuffer));
    memcpy(txBuffer, data, length);

    transaction = {};
    transaction.length = BENCH_SPI_MAX_BYTES * 8;
    transaction.tx_buffer = txBuffer;
    transaction.rx_buffer = rxBuffer;
    pending = spi_slave_queue_trans(HOST, &transaction, 0) == ESP_OK;
    return pending;
}

int benchSpiTakeReceived(uint8_t* buffer, const uint16_t maxLength, const uint32_t timeoutMs, uint32_t& bits)
{
    bits = 0;
    if (!pending) return -1;
    spi_slave_transaction_t* done = NULL;
    if (spi_slave_get_trans_result(HOST, &done, pdMS_TO_TICKS(timeoutMs)) != ESP_OK) return -1;
    pending = false;

    bits = done->trans_len;
    const uint16_t bytes = min<uint32_t>(bits / 8, maxLength);
    memcpy(buffer, rxBuffer, bytes);
    return bytes;
}

// ============================================================================
// NGHE LÉN SPI (THỤ ĐỘNG)
// ============================================================================

static inline uint32_t readPin(const uint8_t gpio)
{
    return gpio < 32 ? (GPIO.in >> gpio) & 1 : (GPIO.in1.data >> (gpio - 32)) & 1;
}

// Đọc mức 3 chân trong 1 lượt: WIDE = có chân >= GPIO32 (đọc thêm thanh ghi in1, chậm hơn).
// Bit 0 = SCK, bit 1 = MOSI, bit 2 = CS.
template <bool WIDE>
static inline uint32_t IRAM_ATTR sniffSample(const uint8_t sck, const uint8_t mosi, const uint8_t cs)
{
    if (!WIDE)
    {
        const uint32_t v = GPIO.in;
        return ((v >> sck) & 1) | (((v >> mosi) & 1) << 1) | (((v >> cs) & 1) << 2);
    }
    const uint64_t v = GPIO.in | ((uint64_t)GPIO.in1.data << 32);
    return ((v >> sck) & 1) | (((v >> mosi) & 1) << 1) | (((v >> cs) & 1) << 2);
}

// Vòng thăm dò đặt trong IRAM: không bị khựng vì cache Flash trong lúc khóa ngắt.
// Chờ CS xuống cũng nằm trong vòng này (đã khóa ngắt) để không lỡ các bit đầu khung.
template <bool WIDE>
static uint8_t IRAM_ATTR sniffLoop(const uint8_t sck, const uint8_t mosi, const uint8_t cs, const uint32_t waitCycles,
                                   const uint32_t idleCycles, const uint32_t limitCycles, uint8_t* bytes,
                                   uint16_t* frameBits, uint32_t& maxGapCycles)
{
    const uint32_t waitStart = xthal_get_ccount();
    while (sniffSample<WIDE>(sck, mosi, cs) & 4)
    {
        if (xthal_get_ccount() - waitStart > waitCycles) return 0;
    }

    uint8_t frames = 0;
    uint16_t totalBits = 0, bitsInFrame = 0;
    uint32_t last = 0; // CS đang LOW, SCK LOW
    uint32_t csHighSince = 0;
    const uint32_t start = xthal_get_ccount();
    uint32_t previous = start;
    maxGapCycles = 0;
    while (frames < BENCH_SNIFF_MAX_FRAMES)
    {
        const uint32_t now = sniffSample<WIDE>(sck, mosi, cs);
        const uint32_t t = xthal_get_ccount();
        if (t - previous > maxGapCycles) maxGapCycles = t - previous; // Lượt lấy mẫu bị chậm (CPU bị dừng tạm)
        previous = t;
        if (!(now & 4))
        {
            if (last & 4) bitsInFrame = 0; // CS vừa xuống: khung mới
            if ((now & 1) && !(last & 1) && totalBits < BENCH_SNIFF_MAX_BYTES * 8)
            {
                // Mode 0: dữ liệu hợp lệ ở sườn lên SCK, bit cao trước
                uint8_t& b = bytes[totalBits >> 3];
                if ((totalBits & 7) == 0) b = 0;
                b |= ((now >> 1) & 1) << (7 - (totalBits & 7));
                totalBits++;
                bitsInFrame++;
            }
        }
        else if (!(last & 4))
        {
            frameBits[frames++] = bitsInFrame; // CS lên: kết thúc khung
            totalBits = (totalBits + 7) & ~7; // Khung sau bắt đầu ở byte mới
            csHighSince = t;
        }
        else if (t - csHighSince > idleCycles)
        {
            break;
        }
        last = now;
        if (t - start > limitCycles) break;
    }
    return frames;
}

uint8_t benchSpiSniff(const uint8_t sckGpio, const uint8_t mosiGpio, const uint8_t csGpio, const uint32_t timeoutMs,
                      const uint32_t idleUs, uint8_t* bytes, uint16_t* frameBits, uint32_t& maxGapNs)
{
    pinMode(sckGpio, INPUT);
    pinMode(mosiGpio, INPUT);
    pinMode(csGpio, INPUT);

    // Khóa ngắt suốt lúc chờ + bắt: tổng <= 200ms (watchdog ngắt ~300ms)
    const uint32_t cyclesPerUs = ESP.getCpuFreqMHz();
    const uint32_t waitCycles = (timeoutMs > 100 ? 100 : timeoutMs) * 1000 * cyclesPerUs;
    const uint32_t limitCycles = 100000 * cyclesPerUs;
    const bool wide = sckGpio >= 32 || mosiGpio >= 32 || csGpio >= 32;
    uint32_t maxGapCycles = 0;
    portDISABLE_INTERRUPTS();
    const uint8_t frames =
        wide ? sniffLoop<true>(sckGpio, mosiGpio, csGpio, waitCycles, idleUs * cyclesPerUs, limitCycles, bytes, frameBits, maxGapCycles)
             : sniffLoop<false>(sckGpio, mosiGpio, csGpio, waitCycles, idleUs * cyclesPerUs, limitCycles, bytes, frameBits, maxGapCycles);
    portENABLE_INTERRUPTS();
    maxGapNs = maxGapCycles * 1000 / cyclesPerUs;
    return frames;
}

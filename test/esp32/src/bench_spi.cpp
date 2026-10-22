#include "bench_spi.h"
#include <driver/spi_slave.h>

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

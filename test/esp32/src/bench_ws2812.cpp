#include "bench_ws2812.h"
#include "bench_pins.h"
#include <driver/rmt.h>

static const rmt_channel_t CHANNEL = RMT_CHANNEL_0;
static const uint32_t TICK_NS_X10 = 125; // APB 80MHz, chia 1 -> 12.5ns / tick
static const uint32_t BIT_THRESHOLD_NS = 600; // Datasheet: T0H tối đa 550ns, T1H tối thiểu 650ns
static const uint32_t FRAME_GAP_NS = 50000; // LOW > 50us = chốt khung (WS2812 đời đầu)

static uint32_t toNs(const uint32_t ticks)
{
    return ticks * TICK_NS_X10 / 10;
}

bool benchWsCapture(const int channel, const uint32_t timeoutMs, uint32_t idleUs, BenchWsCapture_t& r)
{
    memset(&r, 0, sizeof(r));
    r.high0Min = r.high1Min = r.lowMin = r.gapMin = UINT32_MAX;
    if (idleUs > 400) idleUs = 400;

    rmt_config_t cfg = RMT_DEFAULT_CONFIG_RX((gpio_num_t)BENCH_CHANNELS[channel].gpio, CHANNEL);
    cfg.clk_div = 1;
    cfg.mem_block_num = 8; // Mượn bộ nhớ của cả 8 kênh: 512 phần tử
    cfg.rx_config.filter_en = true;
    cfg.rx_config.filter_ticks_thresh = 4; // Bỏ gai < 50ns
    cfg.rx_config.idle_threshold = idleUs * 80;
    if (rmt_config(&cfg) != ESP_OK || rmt_driver_install(CHANNEL, 4096, 0) != ESP_OK) return false;

    RingbufHandle_t ring = NULL;
    rmt_get_ringbuf_handle(CHANNEL, &ring);
    rmt_rx_start(CHANNEL, true);

    size_t length = 0;
    rmt_item32_t* items = (rmt_item32_t*)xRingbufferReceive(ring, &length, pdMS_TO_TICKS(timeoutMs));
    rmt_rx_stop(CHANNEL);

    uint16_t bits = 0;
    if (items)
    {
        const size_t count = length / sizeof(rmt_item32_t);
        r.frameCount = 1;
        for (size_t i = 0; i < count && bits < BENCH_WS_MAX_BITS; i++)
        {
            // Mỗi bit WS2812 = 1 phần tử: HIGH rồi LOW (bỏ phần tử bắt đầu bằng LOW, VD nhiễu lúc chân đổi chế độ)
            if (items[i].level0 != 1) continue;
            const uint32_t high = toNs(items[i].duration0);
            const uint32_t low = toNs(items[i].duration1); // 0 = phần tử cuối (rảnh tới hết)
            const uint8_t bit = high >= BIT_THRESHOLD_NS;
            if (bit)
            {
                r.data[bits / 8] |= 0x80 >> (bits % 8);
                r.high1Min = min(r.high1Min, high);
                r.high1Max = max(r.high1Max, high);
            }
            else
            {
                r.high0Min = min(r.high0Min, high);
                r.high0Max = max(r.high0Max, high);
            }
            bits++;
            r.frameBits[r.frameCount - 1]++;

            if (low == 0) break;
            if (low > FRAME_GAP_NS)
            {
                r.gapMin = min(r.gapMin, low);
                if (r.frameCount < 4) r.frameCount++;
            }
            else
            {
                r.lowMin = min(r.lowMin, low);
                r.lowMax = max(r.lowMax, low);
            }
        }
        vRingbufferReturnItem(ring, items);
    }
    rmt_driver_uninstall(CHANNEL);
    pinMode(BENCH_CHANNELS[channel].gpio, INPUT); // Trả kênh về Hi-Z
    if (r.gapMin == UINT32_MAX) r.gapMin = 0;
    return bits > 0;
}

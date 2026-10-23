"""Kịch bản phía máy tính cho test HIL ch32v003_ws2812 (firmware: test/ch32v003/src/ws2812/main.c).

ESP32 bắt khung WS2812 bằng RMT (lệnh WSCAP, 12.5ns) trên chân DIN: bit-bang PC4 (CH16), SPI DMA PC6 (CH6).
Byte mong đợi dựng bằng mô hình độc lập (mẫu màu, thứ tự màu, độ sáng v × (b + 1) / 256);
độ rộng xung so với datasheet WS2812B (T0H 0.4 ±0.15us, T1H 0.8 ±0.15us, T0L / T1L >= 0.3us).
"""
import time

TIMEOUT_S = 90

LEDS, LONG_LEDS = 8, 16
ORDERS = {"grb": (1, 0, 2), "rgb": (0, 1, 2), "bgr": (2, 1, 0)}

HIGH0_NS = (250, 550)
HIGH1_NS = (650, 950)
LOW_MIN_NS = 250
LOW_MAX_NS = {"bb": 5_000, "dma": 1_200}  # bit-bang: khoảng giữa 2 byte (tính độ sáng byte kế) dài hơn; LED có thể chốt sớm nếu LOW > ~5us
LATCH_MIN_NS = 280_000


def pattern(i):
    return ((i * 37 + 5) & 0xFF, (i * 91 + 200) & 0xFF, (255 - i * 29) & 0xFF)


def frame(count, order="grb", brightness=255, color=None):
    factor = brightness + 1
    out = bytearray()
    for i in range(count):
        rgb = color or pattern(i)
        rgb = [v if factor >= 256 else (v * factor) >> 8 for v in rgb]
        out += bytes(rgb[k] for k in ORDERS[order])
    return out.hex().upper()


EXPECTED = {
    "grb": [frame(LEDS)],
    "rgb": [frame(LEDS, "rgb")],
    "bgr": [frame(LEDS, "bgr")],
    "bright128": [frame(LEDS, brightness=128)],
    "bright0": [frame(LEDS, brightness=0)],
    "fill": [frame(LEDS, color=(0x12, 0x34, 0x56))],
    "twice": [frame(4), frame(4)],
    "long": [frame(LONG_LEDS)],
}


def parse(reply):
    """OK WSCAP <net> <số khung> <hex>... h0=a-b h1=a-b lo=a-b gap=n -> (khung, {tên: (min, max)}, gap)."""
    words = reply.split()
    count = int(words[3])
    frames = words[4:4 + count]
    stats = {}
    gap = 0
    for word in words[4 + count:]:
        key, value = word.split("=")
        if key == "gap":
            gap = int(value)
        elif value != "-":
            low, high = value.split("-")
            stats[key] = (int(low), int(high))
    return frames, stats, gap


def within(stat, low, high):
    return stat is None or (low <= stat[0] and stat[1] <= high)


def verify(case, reply, ctx):
    tag, scene = case.split("_", 1)
    frames, stats, gap = parse(reply)
    want = EXPECTED[scene]
    ctx.check(f"data_{case}", frames == want, f"nhận {' '.join(frames)}" if frames != want else f"{len(want)} khung")

    h0, h1, lo = stats.get("h0"), stats.get("h1"), stats.get("lo")
    detail = f"h0={h0} h1={h1} lo={lo}"
    ok = within(h0, *HIGH0_NS) and within(h1, *HIGH1_NS) and within(lo, LOW_MIN_NS, LOW_MAX_NS[tag])
    ctx.check(f"timing_{case}", ok, detail)
    if scene == "twice":
        ctx.check(f"latch_{case}", gap >= LATCH_MIN_NS, f"khoảng chốt {gap / 1000:.1f}us")


def on_wait(step, bench, ctx):
    kind = step[0]
    if kind == "setup":
        bench.cmd("Z ALL")
    elif kind == "capture":
        net, case, idle = step[1], step[2], step[3]
        bench.send(f"WSCAP {net} 2000 {idle}")
        time.sleep(0.05)  # RMT bắt đầu nghe trước khi chip phát
        ctx.data["case"] = case
    elif kind == "verify":
        case = step[1]
        try:
            reply = bench.reply("WSCAP", timeout=4)
        except RuntimeError as error:
            ctx.check(f"data_{case}", False, str(error))
            return ""
        verify(case, reply, ctx)
    return ""

"""Kịch bản phía máy tính cho test HIL ch32v003_max7219 (firmware: test/ch32v003/src/max7219/main.c).

ESP32 nghe lén SPI (SNIFF, xem spi_sniff.py), runner so từng khung CS với khung dựng theo datasheet:
chuỗi 3 IC, mỗi IC nhận 2 byte (thanh ghi, dữ liệu), IC xa vi điều khiển nhất nhận trước.
"""
from spi_sniff import frame, handle, max7219_init_frames, same

TIMEOUT_S = 60
CASCADE = 3
NOOP = (0x00, 0x00)

EXPECTED = {
    "init": max7219_init_frames(CASCADE),
    "write_register_1": [frame([NOOP, (0x03, 0xA5), NOOP])],
    "write_register_0": [frame([(0x01, 0x81), NOOP, NOOP])],
    "write_register_out": [],
    "write_all": [same(0x08, 0x3C, CASCADE)],
    "write_chain": [frame([(0x05, 0x11), (0x05, 0x22), (0x05, 0x33)])],
    "write_chain_stride": [frame([(0x05, 0xA0), (0x05, 0xA8), (0x05, 0xB0)])],
    "intensity_clamp": [same(0x0A, 0x0F, CASCADE)],
    "power_off": [same(0x0C, 0x00, CASCADE)],
    "power_on": [same(0x0C, 0x01, CASCADE)],
    "clear_all": [same(digit, 0x00, CASCADE) for digit in range(1, 9)],
}


def on_wait(step, bench, ctx):
    return handle(step, bench, ctx, EXPECTED.__getitem__) or ""

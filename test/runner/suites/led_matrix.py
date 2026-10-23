"""Kịch bản phía máy tính cho test HIL ch32v003_led_matrix (firmware: test/ch32v003/src/led_matrix/main.c).

ESP32 nghe lén SPI (spi_sniff.py). Frame Buffer mong đợi dựng bằng mô hình độc lập theo hợp đồng của thư viện:
- module = x / 8, bit 7 của byte hàng y là cột trái của module;
- 90/270 xoay từng module tại chỗ, 180 lật cả dải;
- font 5x7: mỗi byte là 1 cột, bit 0 là hàng trên; mỗi ký tự rộng 6 pixel.
"""
from spi_sniff import frame, handle, max7219_init_frames, same

TIMEOUT_S = 90
MODULES = 2
WIDTH = MODULES * 8

FONT = {
    "H": [0x7F, 0x08, 0x08, 0x08, 0x7F],
    "i": [0x00, 0x44, 0x7D, 0x40, 0x00],
    "I": [0x00, 0x41, 0x7F, 0x41, 0x00],
}


class Canvas:
    def __init__(self, rotation=0):
        self.rows = [[0] * 8 for _ in range(MODULES)]  # [module][hàng]
        self.rotation = rotation

    def pixel(self, x, y, on=True):
        if not (0 <= x < WIDTH and 0 <= y < 8):
            return
        module, col, row = x // 8, x % 8, y
        if self.rotation == 90:
            col, row = row, 7 - col
        elif self.rotation == 180:
            module, col, row = MODULES - 1 - module, 7 - col, 7 - row
        elif self.rotation == 270:
            col, row = 7 - row, col
        if on:
            self.rows[module][row] |= 0x80 >> col
        else:
            self.rows[module][row] &= ~(0x80 >> col) & 0xFF

    def text(self, x, y, string):
        for ch in string:
            for col, bits in enumerate(FONT[ch]):
                for row in range(7):
                    if bits >> row & 1:
                        self.pixel(x + col, y + row)
            x += 6

    def frames(self):
        """8 khung của ledMatrixUpdate(): hàng r của mọi module trong 1 khung."""
        return [frame([(row + 1, self.rows[m][row]) for m in range(MODULES)]) for row in range(8)]


def scene_draw():
    c = Canvas()
    c.pixel(0, 0)
    c.pixel(WIDTH - 1, 7)
    for i in range(8):  # Đường thẳng (0,7) -> (7,0)
        c.pixel(i, 7 - i)
    c.text(8, 0, "Hi")
    c.pixel(9, 0, False)
    return c.frames()


def scene_rotate(rotation):
    c = Canvas(rotation)
    c.pixel(1, 0)
    c.pixel(9, 3)
    return c.frames()


def scene_text(x, y):
    c = Canvas()
    c.text(x, y, "I")
    return c.frames()


EXPECTED = {
    "init": max7219_init_frames(MODULES) + Canvas().frames(),
    "draw": scene_draw(),
    "rotate90": scene_rotate(90),
    "rotate180": scene_rotate(180),
    "rotate270": scene_rotate(270),
    "scroll_left": scene_text(4, 0),
    "scroll_right": scene_text(2, 0),
    "scroll_up": scene_text(0, 6),
    "scroll_down": scene_text(0, -4),
    "brightness": [same(0x0A, 9, MODULES)],
    "brightness_clamp": [same(0x0A, 0x0F, MODULES)],
    "clear": Canvas().frames(),
}


def on_wait(step, bench, ctx):
    return handle(step, bench, ctx, EXPECTED.__getitem__) or ""

#!/usr/bin/env python3
"""Sinh bản vẽ nối dây bàn test HIL từ MỘT bảng dữ liệu duy nhất.

Bố cục: ESP32 DevKit 30 chân vắt qua 2 breadboard mini (ER/EL), bên phải là mini F4P6 (F),
ngay dưới mini F4P6 là mini chip (C) chứa A4M6 + J4M6. Mọi chip luôn có nguồn (cấp từ GPIO);
chip không test chạy firmware "đỗ" (chân Hi-Z, tắt SWD) nên dây giữa các chip nối thẳng.

    python3 gen.py html template.html > page.html
    python3 gen.py svg <esp|ladder|board> > x.svg     # màu cứng để xem thử
    python3 gen.py md                                  # bảng kênh cho WIRING.md
"""
import sys
from html import escape

MODE = "css"
LIGHT = {
    "bg": "#f4f5f1", "fg": "#1c2126", "muted": "#68707a", "line": "#c9ccc4", "board": "#e9e8df",
    "hole": "#b5b3a7", "chip": "#2a2f34", "chipfg": "#eef0ea", "railp": "#d23b3b", "railm": "#2f5fbf",
    "i2c": "#2563d6", "spi": "#d9701a", "uart": "#178a50", "dac": "#7c3fcf", "gpio": "#0e8587",
    "pwr": "#d23b3b", "gnd": "#3b3f44", "swio": "#a88600", "res": "#d4b07a", "resband": "#6b4a1f",
}


def col(name):
    return f"var(--{name})" if MODE == "css" else LIGHT[name]


# ---------------------------------------------------------------------------
# ESP32 DevKit 30 chân, USB HƯỚNG LÊN (cùng chiều Type-C của F4P6), thứ tự từ trên xuống.
# L = cột xa F4P6 (cắm mini EL, cột j) · R = cột gần F4P6 (cắm mini ER, cột a)
# ---------------------------------------------------------------------------
ESP_L = ["3V3", "GND", "15", "2", "4", "16", "17", "5", "18", "19", "21", "RX", "TX", "22", "23"]
ESP_R = ["VIN", "GND", "13", "12", "14", "27", "26", "25", "33", "32", "35", "34", "VN", "VP", "EN"]
ESP_ROW0 = 2  # chân trên cùng cắm ở hàng 2 của mini (17 hàng)
POWER = {  # chân ESP32 -> chip được cấp nguồn (HIGH = có điện)
    ("L", "2"): "F4P6", ("L", "22"): "A4M6", ("L", "23"): "J4M6",
}


def esp_row(side, name):
    return ESP_ROW0 + (ESP_L if side == "L" else ESP_R).index(name)


# ---------------------------------------------------------------------------
# DỮ LIỆU KÊNH — nguồn sự thật duy nhất
# (ch, gpio, cột ESP32, net, nhãn F4P6, cột F4P6 L=hàng 2 / R=hàng 1, vị trí F4P6 1..11, chân A4M6, chân J4M6, nhóm, vai trò)
# ---------------------------------------------------------------------------
CH = [
    (1, 13, "R", "PD4", "D4", "R", 4, 8, None, "gpio", "GPIO / ADC A7"),
    (2, 12, "R", "PD3", "D3", "R", 5, None, None, "gpio", "GPIO / ADC A4"),
    (3, 15, "L", "PD2", "D2", "R", 6, None, None, "gpio", "GPIO / ADC A3"),
    (4, 4, "L", "PD0", "D0", "R", 8, None, None, "gpio", "GPIO"),
    (5, 16, "L", "PC7", "C7", "R", 9, 6, None, "spi", "SPI MISO"),
    (6, 17, "L", "PC6", "C6", "R", 10, 5, None, "spi", "SPI MOSI"),
    (7, 5, "L", "PC5", "C5", "R", 11, None, None, "spi", "SPI SCK"),
    (8, 27, "R", "PD6", "RX", "L", 4, 10, None, "uart", "UART – CH32 RX"),
    (9, 14, "R", "PD5", "TX", "L", 3, 9, None, "uart", "UART – CH32 TX"),
    (10, 26, "R", "PA1", "A1", "L", 5, 12, 1, "dac", "DAC2 → ADC A1"),
    (11, 25, "R", "PA2", "A2", "L", 6, 13, 3, "dac", "DAC1 → ADC A0"),
    (12, 33, "R", "PC0", "C0", "L", 7, 16, None, "spi", "SPI CS / GPIO"),
    (13, 32, "R", "PC1", "C1", "L", 8, 1, 5, "i2c", "I2C SDA"),
    (14, 18, "L", "PC2", "C2", "L", 9, 2, 6, "i2c", "I2C SCL"),
    (15, 19, "L", "PC3", "C3", "L", 10, 3, None, "gpio", "GPIO / PWM"),
    (16, 21, "L", "PC4", "C4", "L", 11, 4, 7, "gpio", "GPIO / ADC A2"),
]
NET = {c[3]: c for c in CH}

F4_L = ["G", "V", "TX", "RX", "A1", "A2", "C0", "C1", "C2", "C3", "C4"]  # hàng 2 (Type-C hướng lên)
F4_R = ["G", "V", "D7", "D4", "D3", "D2", "D1", "D0", "C7", "C6", "C5"]  # hàng 1
A4_PINS = {1: "PC1", 2: "PC2", 3: "PC3", 4: "PC4", 5: "PC6", 6: "PC7", 7: "SWIO", 8: "PD4", 9: "PD5", 10: "PD6",
           11: "NRST", 12: "PA1", 13: "PA2", 14: "GND", 15: "VDD", 16: "PC0"}
J4_PINS = {1: "PA1", 2: "GND", 3: "PA2", 4: "VDD", 5: "PC1", 6: "PC2", 7: "PC4", 8: "SWIO"}
F4_ROW0, A4_ROW0, J4_ROW0, BUS3V3_ROW = 1, 3, 12, 17
A4_ROTATED = True  # A4M6 xoay 180°: chân 1 ở dưới-phải -> dây từ F4P6 ít vắt chéo nhất


def dip(pin, count, row0, rotated=False):
    half = count // 2
    side, i = ("L", pin - 1) if pin <= half else ("R", count - pin)
    if rotated:
        side, i = ("R" if side == "L" else "L"), half - 1 - i
    return side, row0 + i


def a4(pin):
    return dip(pin, 16, A4_ROW0, A4_ROTATED)


def j4(pin):
    return dip(pin, 8, J4_ROW0)


def h(board, row, colname):
    return f"{board}:{row}{colname}"


def esp_in(side):   # lỗ của chân ESP32 trên mini
    return "a" if side == "R" else "j"


def esp_out(side):  # lỗ dây đi ra (nửa bên kia rãnh, sau trở)
    return "j" if side == "R" else "a"


def f4_in(c):
    return "a" if c[5] == "L" else "j"


def f4_down(c):
    return "b" if c[5] == "L" else "i"


def chip_hole(side, used):  # lỗ trống đầu tiên phía ngoài của một hàng chân chip
    order = "abc" if side == "L" else "jih"
    for x in order:
        if x not in used:
            used.add(x)
            return x
    raise RuntimeError("hết lỗ")


# ---------------------------------------------------------------------------
# DANH SÁCH DÂY THEO BƯỚC (dùng cho bảng HTML và nhãn trên hình)
# ---------------------------------------------------------------------------
def build_steps():
    used = {}  # (board,row,side) -> set(cột đã dùng)

    def take(board, row, side):
        return chip_hole(side, used.setdefault((board, row, side), set()))

    steps = []
    rows = []
    for c in sorted(CH, key=lambda c: (c[2], esp_row(c[2], str(c[1])))):
        r = esp_row(c[2], str(c[1]))
        board = "ER" if c[2] == "R" else "EL"
        a, b = ("e", "f") if c[2] == "R" else ("f", "e")
        rows.append((c[9], f"Trở 1kΩ {h(board, r, a)} ↔ {h(board, r, b)}", f"GPIO{c[1]}", f"CH{c[0]}"))
    steps.append(("16 trở 1kΩ phía ESP32 (vắt qua rãnh, ngay tại hàng của chân)", rows))

    rows = []
    for c in sorted(CH, key=lambda c: (c[5], c[6])):
        r = esp_row(c[2], str(c[1]))
        board = "ER" if c[2] == "R" else "EL"
        fr = F4_ROW0 + c[6] - 1
        rows.append((c[9], f"{h(board, r, esp_out(c[2]))} (sau trở GPIO{c[1]})", f"{h('F', fr, f4_in(c))} · F4P6 {c[4]}",
                     f"CH{c[0]} {c[10]}"))
    steps.append(("16 dây tín hiệu ESP32 → F4P6", rows))

    rows = []
    for c in sorted([c for c in CH if c[7]], key=lambda c: (c[5], c[6])):
        side, r = a4(c[7])
        fr = F4_ROW0 + c[6] - 1
        rows.append((c[9], f"{h('F', fr, f4_down(c))} · F4P6 {c[4]}", f"{h('C', r, take('C', r, side))} · A4M6 chân {c[7]}",
                     c[3]))
    steps.append(("12 dây F4P6 → A4M6 (nối thẳng, không trở)", rows))

    rows = []
    for c in [c for c in CH if c[8]]:
        sa, ra = a4(c[7])
        sj, rj = j4(c[8])
        rows.append((c[9], f"{h('C', ra, take('C', ra, sa))} · A4M6 chân {c[7]}",
                     f"{h('C', rj, take('C', rj, sj))} · J4M6 chân {c[8]}", c[3]))
    steps.append(("5 dây A4M6 → J4M6", rows))

    sa, ra = a4(7)
    sj, rj = j4(8)
    rows = [
        ("swio", f"{h('F', F4_ROW0 + 6, 'i')} · F4P6 D1", f"{h('C', ra, take('C', ra, sa))} · A4M6 chân 7", "SWIO"),
        ("swio", f"{h('C', ra, take('C', ra, sa))} · A4M6 chân 7", f"{h('C', rj, take('C', rj, sj))} · J4M6 chân 8", "SWIO"),
        ("swio", "WCH-LinkE SWDIO", f"{h('C', ra, take('C', ra, sa))} · A4M6 chân 7", "Đường nạp chung cho cả 3 chip"),
    ]
    steps.append(("SWIO: nối chung 3 chip vào WCH-LinkE", rows))

    gnd_r = esp_row("R", "GND")
    v33_r = esp_row("L", "3V3")
    rows = []
    for (side, name), chip in POWER.items():
        r = esp_row(side, name)
        board = "ER" if side == "R" else "EL"
        src = h(board, r, "b" if side == "R" else "i")
        if chip == "F4P6":
            dst = f"{h('F', F4_ROW0 + 1, 'a')} · F4P6 V (3V3)"
            note = "Cấp nguồn F4P6. ⚠ Nếu chân 3V3 là V bên phải → F:2j"
        elif chip == "A4M6":
            s, rr = a4(15)
            dst = f"{h('C', rr, take('C', rr, s))} · A4M6 chân 15 (VDD)"
            note = "Cấp nguồn A4M6"
        else:
            s, rr = j4(4)
            dst = f"{h('C', rr, take('C', rr, s))} · J4M6 chân 4 (VDD)"
            note = "Cấp nguồn J4M6"
        rows.append(("pwr", f"{src} · GPIO{name}", dst, note))
    s, rr = a4(15)
    s2, rr2 = a4(14)
    rows.append(("pwr", f"{h('C', rr, take('C', rr, s))} (A4M6 VDD)", f"{h('C', rr2, take('C', rr2, s2))} (A4M6 GND)", "Tụ 100nF"))
    s, rr = j4(4)
    s2, rr2 = j4(2)
    rows.append(("pwr", f"{h('C', rr, take('C', rr, s))} (J4M6 VDD)", f"{h('C', rr2, take('C', rr2, s2))} (J4M6 GND)", "Tụ 100nF"))
    steps.append(("Nguồn từng chip từ GPIO (HIGH = có điện)", rows))

    rows = [("gnd", f"{h('ER', gnd_r, 'b')} · GND", f"{h('F', F4_ROW0, 'a')} · F4P6 G", "")]
    s, rr = a4(14)
    rows.append(("gnd", f"{h('ER', gnd_r, 'c')} · GND", f"{h('C', rr, take('C', rr, s))} · A4M6 chân 14", ""))
    s, rr = j4(2)
    rows.append(("gnd", f"{h('ER', gnd_r, 'd')} · GND", f"{h('C', rr, take('C', rr, s))} · J4M6 chân 2", ""))
    rows.append(("gnd", f"{h('ER', gnd_r, 'e')} · GND", "WCH-LinkE GND", "KHÔNG nối chân 3V3/5V của LinkE"))
    rows.append(("pwr", f"{h('EL', v33_r, 'i')} · 3V3", f"{h('C', BUS3V3_ROW, 'j')}",
                 f"Hàng {BUS3V3_ROW} (nửa phải) mini chip = 3V3 cố định, chỉ dùng cho pull-up"))
    s, rr = j4(5)
    rows.append(("i2c", f"{h('C', BUS3V3_ROW, 'i')}", f"{h('C', rr, take('C', rr, s))} · J4M6 PC1", "Trở 4.7kΩ pull-up SDA"))
    s, rr = j4(6)
    rows.append(("i2c", f"{h('C', BUS3V3_ROW, 'h')}", f"{h('C', rr, take('C', rr, s))} · J4M6 PC2", "Trở 4.7kΩ pull-up SCL"))
    steps.append(("GND chung, 3V3 cố định và pull-up I2C", rows))
    return steps


STEPS = build_steps()


# ---------------------------------------------------------------------------
# TIỆN ÍCH SVG
# ---------------------------------------------------------------------------
def text(x, y, s, size=10, anchor="start", fill=None, cls="t", weight=None):
    fill = fill or col("fg")
    attrs = f'x="{x:.1f}" y="{y:.1f}" text-anchor="{anchor}" font-size="{size}" fill="{fill}"'
    attrs += f' class="{cls}"' if MODE == "css" else ' font-family="DejaVu Sans"'
    if weight:
        attrs += f' font-weight="{weight}"'
    return f"<text {attrs}>{escape(str(s))}</text>"


def path(d, color, width=2.0, extra=""):
    return (f'<path d="{d}" fill="none" stroke="{color}" stroke-width="{width}" stroke-linecap="round" '
            f'stroke-linejoin="round" {extra}/>')


def line(x1, y1, x2, y2, color, width=1.5):
    return path(f"M{x1},{y1} L{x2},{y2}", color, width)


def dot(x, y, color, r=3.2):
    return f'<circle cx="{x:.1f}" cy="{y:.1f}" r="{r}" fill="{color}"/>'


def resistor(x1, y1, x2, y2, lead=None, label=None):
    mx, my = (x1 + x2) / 2, (y1 + y2) / 2
    horiz = abs(x2 - x1) >= abs(y2 - y1)
    w, hh = (16, 7) if horiz else (7, 16)
    out = line(x1, y1, x2, y2, lead or col("gnd"), 1.3)
    out += (f'<rect x="{mx - w / 2:.1f}" y="{my - hh / 2:.1f}" width="{w}" height="{hh}" rx="3" fill="{col("res")}" '
            f'stroke="{col("resband")}" stroke-width="0.8"/>')
    out += (line(mx - 3, my - hh / 2, mx - 3, my + hh / 2, col("resband"), 1.6) if horiz
            else line(mx - w / 2, my - 3, mx + w / 2, my - 3, col("resband"), 1.6))
    if label:
        out += text(mx + (0 if horiz else 8), my - 6 if horiz else my + 3, label, 8, "middle" if horiz else "start",
                    col("muted"))
    return out


def cap(x1, y1, x2, y2):
    out = line(x1, y1, x2, y2, col("gnd"), 1.2)
    mx, my = (x1 + x2) / 2, (y1 + y2) / 2
    return out + f'<rect x="{mx - 4}" y="{my - 4}" width="8" height="8" rx="2" fill="{col("i2c")}" opacity="0.75"/>'


def svg(w, hgt, body, label):
    bg = f'<rect x="0" y="0" width="{w}" height="{hgt}" fill="{col("bg")}"/>' if MODE != "css" else ""
    return (f'<svg viewBox="0 0 {w} {hgt}" width="{w}" height="{hgt}" role="img" aria-label="{escape(label)}" '
            f'xmlns="http://www.w3.org/2000/svg">{bg}{body}</svg>')


class Mini:
    """Breadboard mini 17 hàng × (a–e | f–j), không thanh nguồn."""
    P = 15

    def __init__(self, x, y, name):
        self.x, self.y, self.name = x, y, name
        self.cx = {c: x + 18 + i * self.P for i, c in enumerate("abcde")}
        self.cx.update({c: x + 18 + (i + 7) * self.P for i, c in enumerate("fghij")})
        self.w = self.cx["j"] + 18 - x
        self.h = 17 * self.P + 22

    def ry(self, r):
        return self.y + 16 + (r - 1) * self.P

    def at(self, r, c):
        return self.cx[c], self.ry(r)

    def draw(self, label_side="top"):
        b = [f'<rect x="{self.x}" y="{self.y}" width="{self.w}" height="{self.h}" rx="5" fill="{col("board")}" '
             f'stroke="{col("line")}"/>']
        gx = (self.cx["e"] + self.cx["f"]) / 2
        b.append(f'<rect x="{gx - 8}" y="{self.y + 6}" width="16" height="{self.h - 12}" fill="{col("line")}" opacity="0.55"/>')
        for r in range(1, 18):
            for x in self.cx.values():
                b.append(f'<circle cx="{x}" cy="{self.ry(r)}" r="1.6" fill="{col("hole")}"/>')
        for r in (1, 5, 10, 15, 17):
            b.append(text(self.x - 4, self.ry(r) + 3, r, 7, "end", col("muted"), "mono"))
        ly = self.y - 8 if label_side == "top" else self.y + self.h + 14
        b.append(text(self.x, ly, self.name, 10, "start", col("muted"), "h", "600"))
        for c in ("a", "e", "f", "j"):
            b.append(text(self.cx[c], self.y + self.h - 2, c, 6.5, "middle", col("muted"), "mono"))
        return "".join(b)


def chip_body(m, row0, rows, cl, cr, pins_l, pins_r, pad=7):
    x0, x1 = m.cx[cl] - pad, m.cx[cr] + pad
    y0, y1 = m.ry(row0) - 7, m.ry(row0 + rows - 1) + 7
    b = [f'<rect x="{x0}" y="{y0}" width="{x1 - x0}" height="{y1 - y0}" rx="3" fill="{col("chip")}"/>']
    for i, n in enumerate(pins_l):
        b.append(dot(m.cx[cl], m.ry(row0 + i), col("chipfg"), 2.5))
        b.append(text(m.cx[cl] + 6, m.ry(row0 + i) + 3, n, 7.2, "start", col("chipfg"), "mono"))
    for i, n in enumerate(pins_r):
        b.append(dot(m.cx[cr], m.ry(row0 + i), col("chipfg"), 2.5))
        b.append(text(m.cx[cr] - 6, m.ry(row0 + i) + 3, n, 7.2, "end", col("chipfg"), "mono"))
    return "".join(b)


# ---------------------------------------------------------------------------
# HÌNH 1: TOÀN CẢNH — ESP32 trên 2 mini → F4P6 → mini chip
# ---------------------------------------------------------------------------
def fig_bench():
    EL = Mini(60, 90, "Mini ESP32 trái (EL)")
    ER = Mini(EL.x + EL.w + 84, 90, "Mini ESP32 phải (ER)")
    # F thấp hơn ER đúng 3 hàng: chân ER hàng 6..11 thẳng hàng với F hàng 3..8 -> 6 dây R→L nằm ngang tuyệt đối
    F = Mini(ER.x + ER.w + 250, ER.y + 3 * Mini.P, "Mini F4P6 (F)")
    C = Mini(F.x, F.y + F.h + 50, "Mini chip (C)")
    W, H = F.x + F.w + 250, C.y + C.h + 60
    b = [EL.draw(), ER.draw(), F.draw(), C.draw("bottom")]

    # ESP32: chân trái ở cột j của EL, chân phải ở cột a của ER
    ex0, ex1 = EL.cx["j"] - 9, ER.cx["a"] + 9
    b.append(f'<rect x="{ex0}" y="{EL.ry(1) - 4}" width="{ex1 - ex0}" height="{EL.ry(17) - EL.ry(1) + 18}" rx="6" '
             f'fill="{col("chip")}" opacity="0.94"/>')
    b.append(text((ex0 + ex1) / 2, EL.ry(1) + 8, "USB ▲", 8, "middle", col("chipfg")))
    b.append(text((ex0 + ex1) / 2, EL.ry(17) + 10, "ANTEN ▼", 8, "middle", col("chipfg")))
    for side, names, m, cname in (("L", ESP_L, EL, "j"), ("R", ESP_R, ER, "a")):
        for i, n in enumerate(names):
            x, y = m.at(ESP_ROW0 + i, cname)
            power = POWER.get((side, n))
            used = any(c[2] == side and str(c[1]) == n for c in CH)
            fill = col("pwr") if power else (col("chipfg") if used or n in ("GND", "3V3") else col("muted"))
            b.append(dot(x, y, fill, 3))
            b.append(text(x + (7 if side == "L" else -7), y + 3, n, 8, "start" if side == "L" else "end", fill, "mono"))

    # F4P6 trên mini F (Type-C hướng lên, chân ở cột c / h)
    b.append(chip_body(F, F4_ROW0, 11, "c", "h", F4_L, F4_R, 9))
    gx = (F.cx["e"] + F.cx["f"]) / 2
    b.append(f'<rect x="{gx - 14}" y="{F.ry(1) - 24}" width="28" height="13" rx="3" fill="{col("muted")}"/>')
    b.append(text(gx, F.ry(1) - 14.5, "USB-C", 6.5, "middle", col("chipfg")))

    # Dây ESP32 → F4P6. Bốn kiểu đường đi, mỗi kiểu xếp làn sao cho không cắt nhau:
    #  R→L: ngang bậc thang · R→R: vòng qua đỉnh F · L→R: vòng qua đỉnh cả bàn · L→L: vòng dưới đáy
    def ends(c):
        side = c[2]
        m = ER if side == "R" else EL
        r = esp_row(side, str(c[1]))
        if side == "R":
            b.append(resistor(*m.at(r, "e"), *m.at(r, "f"), lead=col(c[9])))
        else:
            b.append(resistor(*m.at(r, "f"), *m.at(r, "e"), lead=col(c[9])))
        sx, sy = m.at(r, esp_out(side))
        tx, ty = F.at(F4_ROW0 + c[6] - 1, f4_in(c))
        return sx, sy, tx, ty

    def finish(c, d, sx, sy, tx, ty, label_anchor):
        color = col(c[9])
        b.append(path(d, color))
        b.append(dot(sx, sy, color, 2.4))
        b.append(dot(tx, ty, color, 2.4))
        b.append(text(sx + (6 if label_anchor == "start" else -6), sy - 4, f"CH{c[0]}", 7.5, label_anchor, color))

    def esp_key(c):
        return esp_row(c[2], str(c[1]))

    rl = sorted([c for c in CH if c[2] == "R" and c[5] == "L"], key=esp_key)
    rr = sorted([c for c in CH if c[2] == "R" and c[5] == "R"], key=esp_key)
    lr = sorted([c for c in CH if c[2] == "L" and c[5] == "R"], key=esp_key)
    ll = sorted([c for c in CH if c[2] == "L" and c[5] == "L"], key=esp_key)
    mid0 = F.x - 40
    for k, c in enumerate(rl):  # dây trên dùng làn gần F hơn
        sx, sy, tx, ty = ends(c)
        lane = mid0 - k * 11
        finish(c, f"M{sx},{sy} L{lane},{sy} L{lane},{ty} L{tx},{ty}", sx, sy, tx, ty, "start")
    for k, c in enumerate(rr):  # vòng qua đỉnh mini F sang hàng 1
        sx, sy, tx, ty = ends(c)
        lane = mid0 - (len(rl) + k) * 11
        ytop = F.y - 18 - (len(rr) - 1 - k) * 8
        xr = F.x + F.w + 16 + (len(rr) - 1 - k) * 10
        finish(c, f"M{sx},{sy} L{lane},{sy} L{lane},{ytop} L{xr},{ytop} L{xr},{ty} L{tx},{ty}", sx, sy, tx, ty,
               "start")
    for k, c in enumerate(lr):  # vòng qua đỉnh cả bàn sang hàng 1
        sx, sy, tx, ty = ends(c)
        lane = EL.x - 12 - (len(lr) - 1 - k) * 8
        ytop = EL.y - 40 - (len(lr) - 1 - k) * 8
        xr = F.x + F.w + 16 + (len(rr) + k) * 10
        finish(c, f"M{sx},{sy} L{lane},{sy} L{lane},{ytop} L{xr},{ytop} L{xr},{ty} L{tx},{ty}", sx, sy, tx, ty,
               "end")
    for k, c in enumerate(ll):  # vòng dưới đáy rồi lên hàng 2 (dưới các dây ngang)
        sx, sy, tx, ty = ends(c)
        lane = EL.x - 12 - (len(lr) + k) * 8
        ybot = EL.y + EL.h + 14 + k * 8
        xm = mid0 - (len(rl) + len(rr) + len(ll) - 1 - k) * 11
        finish(c, f"M{sx},{sy} L{lane},{sy} L{lane},{ybot} L{xm},{ybot} L{xm},{ty} L{tx},{ty}", sx, sy, tx, ty,
               "end")

    # Chip mini
    a_left = sorted([p for p in A4_PINS if a4(p)[0] == "L"], key=lambda p: a4(p)[1])
    a_right = sorted([p for p in A4_PINS if a4(p)[0] == "R"], key=lambda p: a4(p)[1])
    b.append(chip_body(C, A4_ROW0, 8, "d", "g", [f"{p} {A4_PINS[p]}" for p in a_left],
                       [f"{A4_PINS[p]} {p}" for p in a_right]))
    s1, r1 = a4(1)
    px, py = C.at(r1, "d" if s1 == "L" else "g")
    b.append(f'<circle cx="{px}" cy="{py}" r="5" fill="none" stroke="{col("pwr")}" stroke-width="1.6"/>')
    b.append(text(C.x + C.w + 64, C.ry(A4_ROW0 + 5) + 3, "xoay 180°, chân 1 dưới-phải", 8.5, "start", col("pwr"), "t", "600"))
    s1, r1 = j4(1)
    px, py = C.at(r1, "d" if s1 == "L" else "g")
    b.append(f'<circle cx="{px}" cy="{py}" r="5" fill="none" stroke="{col("pwr")}" stroke-width="1.6"/>')
    b.append(chip_body(C, J4_ROW0, 4, "d", "g", [f"{p} {J4_PINS[p]}" for p in range(1, 5)],
                       [f"{J4_PINS[p]} {p}" for p in range(8, 4, -1)]))
    b.append(text(C.x + C.w + 64, C.ry(A4_ROW0 + 3) + 3, "A4M6", 10, "start", col("muted"), "h", "600"))
    b.append(text(C.x + C.w + 64, C.ry(J4_ROW0 + 1) + 3, "J4M6", 10, "start", col("muted"), "h", "600"))

    # Dây F4P6 → A4M6 / A4M6 → J4M6 vẽ theo làn hai bên mini chip
    def route(m1, r1, c1, m2, r2, c2, color, side, idx, width=1.6):
        x1, y1 = m1.at(r1, c1)
        x2, y2 = m2.at(r2, c2)
        lx = (C.x - 8 - idx * 5) if side == "L" else (C.x + C.w + 8 + idx * 5)
        b.append(path(f"M{x1},{y1} L{lx},{y1} L{lx},{y2} L{x2},{y2}", color, width))
        b.append(dot(x1, y1, color, 2.2))
        b.append(dot(x2, y2, color, 2.2))

    il, ir = 0, 0
    for title, rows in STEPS:
        for g, a, bb, note in rows:
            if not (a.startswith("F:") or a.startswith("C:")) or not bb.startswith("C:"):
                continue
            m1 = F if a.startswith("F:") else C
            r1, c1 = int(a[2:].split(" ")[0][:-1]), a[2:].split(" ")[0][-1]
            r2, c2 = int(bb[2:].split(" ")[0][:-1]), bb[2:].split(" ")[0][-1]
            if "Tụ" in note:
                b.append(cap(*C.at(r1, c1), *C.at(r2, c2)))
                continue
            if "4.7k" in note:
                b.append(resistor(*C.at(r1, c1), *C.at(r2, c2), lead=col("i2c")))
                continue
            side = "L" if c1 in "abcde" else "R"
            if side == "L":
                route(m1, r1, c1, C, r2, c2, col(g), "L", il)
                il += 1
            else:
                route(m1, r1, c1, C, r2, c2, col(g), "R", ir)
                ir += 1

    # Nguồn / GND / 3V3 / LinkE: ghi nhãn tại đầu dây (không vẽ dây dài cho đỡ rối)
    def tag(m, r, cname, s, color, anchor):
        x, y = m.at(r, cname)
        b.append(dot(x, y, color, 2.6))
        tx = x - 10 if anchor == "end" else x + 10
        b.append(line(x, y, tx, y, color, 1.4))
        b.append(text(tx + (-3 if anchor == "end" else 3), y + 3, s, 7.5, anchor, color, "t", "600"))

    for (side, name), chip in POWER.items():
        m = ER if side == "R" else EL
        r = esp_row(side, name)
        tag(m, r, "b" if side == "R" else "i", f"→ VDD {chip}", col("pwr"), "start" if side == "R" else "end")
    tag(ER, esp_row("R", "GND"), "b", "GND chung", col("gnd"), "start")
    tag(EL, esp_row("L", "3V3"), "i", f"→ C:{BUS3V3_ROW}j (3V3)", col("pwr"), "end")
    tag(F, F4_ROW0 + 1, "a", "← GPIO2", col("pwr"), "end")
    tag(F, F4_ROW0, "a", "← GND", col("gnd"), "end")
    s, r = a4(15)
    tag(C, r, "a" if s == "L" else "j", "← GPIO22", col("pwr"), "end" if s == "L" else "start")
    s, r = j4(4)
    tag(C, r, "a", "← GPIO23", col("pwr"), "end")
    s, r = a4(14)
    tag(C, r, "a" if s == "L" else "j", "← GND", col("gnd"), "end" if s == "L" else "start")
    s, r = j4(2)
    tag(C, r, "a", "← GND", col("gnd"), "end")
    tag(C, BUS3V3_ROW, "j", "← 3V3", col("pwr"), "start")
    linke = [r for t, rows in STEPS for r in rows if r[1] == "WCH-LinkE SWDIO"][0][2].split(" ")[0]
    b.append(text(C.x, C.y + C.h + 30, f"SWDIO (WCH-LinkE) → {linke}", 9, "start", col("swio"), "t", "600"))
    return svg(W, H, "".join(b), "Toàn cảnh bàn test: ESP32 trên 2 mini, F4P6, A4M6 và J4M6")


# ---------------------------------------------------------------------------
# HÌNH 2: MỖI KÊNH NỐI TỚI ĐÂU
# ---------------------------------------------------------------------------
def fig_ladder():
    RP = 28
    top = 56
    W, H = 900, top + len(CH) * RP + 16
    X_ESP, X_F4, X_A, X_J = 120, 290, 520, 690
    b = []
    for x, s in ((X_ESP - 10, "ESP32"), ((X_ESP + X_F4) / 2, "1kΩ"), (X_F4, "F4P6"), (X_A, "A4M6"), (X_J, "J4M6")):
        b.append(text(x, 28, s, 11, "middle", col("fg"), "h", "600"))
    b.append(text(800, 28, "Vai trò", 11, "start", col("fg"), "h", "600"))
    for i, c in enumerate(CH):
        y = top + i * RP
        color = col(c[9])
        if i % 2 == 0:
            b.append(f'<rect x="8" y="{y - RP / 2}" width="{W - 16}" height="{RP}" rx="4" fill="{col("line")}" opacity="0.25"/>')
        b.append(text(16, y + 4, f"CH{c[0]}", 10, "start", color, "mono", "600"))
        b.append(text(X_ESP + 14, y + 4, f"GPIO{c[1]}", 10, "end", col("fg"), "mono"))
        b.append(dot(X_ESP + 22, y, color))
        b.append(resistor(X_ESP + 22, y, X_F4 - 6, y, lead=color))
        b.append(dot(X_F4, y, color, 4.4))
        b.append(text(X_F4 + 10, y - 6, c[4], 10, "start", col("fg"), "mono", "600"))
        end = X_J if c[8] else (X_A if c[7] else X_F4)
        if end > X_F4:
            b.append(line(X_F4, y, end, y, color, 2))
        for pin, x in ((c[7], X_A), (c[8], X_J)):
            if pin:
                b.append(dot(x, y, color, 4))
                b.append(text(x + 9, y - 6, f"chân {pin}", 9.5, "start", col("fg"), "mono"))
            else:
                b.append(text(x, y + 4, "–", 10, "middle", col("muted")))
        b.append(text(800, y + 4, c[10], 9.5, "start", col("muted")))
    return svg(W, H, "".join(b), "Mỗi kênh nối tới chân nào trên từng chip")


# ---------------------------------------------------------------------------
# XUẤT RA
# ---------------------------------------------------------------------------
def steps_html():
    out = []
    for i, (title, rows) in enumerate(STEPS, 2):
        out.append(f'<section class="step"><h3><span class="n">Bước {i}</span>{escape(title)}</h3>')
        out.append('<div class="tw"><table><thead><tr><th></th><th>Từ</th><th>Đến</th><th>Ghi chú</th></tr></thead><tbody>')
        for g, a, b2, note in rows:
            out.append(f'<tr><td><span class="sw" style="background:var(--{g})"></span></td><td class="m">{escape(a)}</td>'
                       f'<td class="m">{escape(b2)}</td><td>{escape(note)}</td></tr>')
        out.append("</tbody></table></div></section>")
    return "".join(out)


def channel_table_html():
    rows = []
    for c in CH:
        rows.append(f'<tr><td><span class="sw" style="background:var(--{c[9]})"></span><b class="m">CH{c[0]}</b></td>'
                    f'<td class="m">GPIO{c[1]}</td><td class="m">{c[3]}</td><td class="m">{c[4]}</td>'
                    f'<td class="m">{c[7] or "–"}</td><td class="m">{c[8] or "–"}</td><td>{escape(c[10])}</td></tr>')
    return ("<div class=\"tw\"><table><thead><tr><th>Kênh</th><th>ESP32</th><th>Net</th><th>F4P6</th><th>A4M6 chân</th>"
            "<th>J4M6 chân</th><th>Vai trò</th></tr></thead><tbody>" + "".join(rows) + "</tbody></table></div>")


def markdown_tables():
    out = ["| Kênh | ESP32 GPIO | Net | F4P6 (nhãn) | A4M6 (chân DIP16) | J4M6 (chân DIP8) | Vai trò |",
           "| :-: | :-: | :-: | :-: | :-: | :-: | :-- |"]
    for c in CH:
        out.append(f"| {c[0]} | {c[1]} | {c[3]} | {c[4]} | {c[7] or '–'} | {c[8] or '–'} | {c[10]} |")
    return "\n".join(out)


if __name__ == "__main__":
    cmd = sys.argv[1]
    if cmd == "svg":
        MODE = "literal"
        print({"bench": fig_bench, "ladder": fig_ladder}[sys.argv[2]]())
    elif cmd == "md":
        print(markdown_tables())
    elif cmd == "html":
        tpl = open(sys.argv[2]).read()
        print(tpl.replace("{{FIG_BENCH}}", fig_bench()).replace("{{FIG_LADDER}}", fig_ladder())
              .replace("{{STEPS}}", steps_html()).replace("{{CHANNELS}}", channel_table_html()))

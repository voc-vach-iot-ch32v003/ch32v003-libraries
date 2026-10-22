"""Kịch bản phía máy tính cho test HIL ch32v003_gpio (firmware: test/ch32v003/src/gpio/main.c)."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "wiring"))
import gen  # noqa: E402  bảng kênh duy nhất của bàn test

TIMEOUT_S = 120
NETS = [c[3] for c in sorted(gen.CH, key=lambda c: c[0])]  # thứ tự CH1..CH16 = thứ tự bit của lệnh RA
PULLED_UP_NETS = {"PC1", "PC2"}  # trở kéo lên 4.7kΩ của I2C
# PC0: nút nhấn trên board TWEN32F003 có trở kéo xuống -> firmware bỏ qua test pull-up của PC0


def on_wait(step, bench, ctx):
    kind, net, value = step[0], step[1], int(step[2])

    if kind == "expect" and value == 1:
        # Các kênh khác kéo xuống: net bị chập/cắm nhầm sẽ lộ ra là 1
        for other in NETS:
            bench.cmd(f"MODE {other} {'IN' if other == net else 'PD'}")
        bits = bench.cmd("RA").split()[2]
        got = dict(zip(NETS, bits))
        ctx.check(f"output_high_{net}", got[net] == "1", f"đo={got[net]}")
        leaked = [n for n in NETS if n != net and got[n] == "1" and n not in PULLED_UP_NETS]
        ctx.check(f"isolation_{net}", not leaked, f"net khác bị kéo lên: {leaked}" if leaked else "")
        bench.cmd("Z ALL")
    elif kind == "expect":
        level = bench.cmd(f"R {net}").split()[-1]
        ctx.check(f"output_low_{net}", level == "0", f"đo={level}")
    elif kind == "drive":
        bench.cmd(f"W {net} {value}")
    elif kind == "release":
        bench.cmd(f"Z {net}")
    elif kind == "expect_pull":
        bench.cmd(f"Z {net}")
        level = bench.cmd(f"R {net}").split()[-1]
        ctx.check(f"pull_{'up' if value else 'down'}_{net}", level == str(value), f"đo={level}")
    return ""

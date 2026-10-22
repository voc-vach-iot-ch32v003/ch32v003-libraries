"""Kịch bản phía máy tính cho test HIL ch32v003_i2c (firmware: test/ch32v003/src/i2c/main.c).

ESP32 làm I2C slave 0x42 trên CH13 (PC1 SDA) / CH14 (PC2 SCL), pull-up 4.7kΩ có sẵn trên bus.
"""
import statistics
import time

TIMEOUT_S = 60
SLAVE_ADDRESS = "42"
SCL_NET = "PC2"

# Khoảng tần số SCL chấp nhận (Hz). Thời gian sườn lên qua trở kéo 4.7kΩ kéo dài nửa chu kỳ HIGH
# (master phần cứng chỉ bắt đầu đếm khi thấy SCL đã lên) nên tần số thật thấp hơn danh định.
SCL_RANGE = {
    "hw100": (80_000, 102_000),
    "hw400": (250_000, 410_000),
    "sw": (70_000, 102_000),  # bit-bang ~100kHz (I2C_SOFTWARE_DELAY_LOOPS = 5), không vượt Standard Mode
}
SLAVE_HZ = {"hw100": 100_000, "hw400": 400_000, "sw": 100_000}


def scl_frequency(level, durations_ns):
    """Tần số SCL = 1 / trung vị (LOW + HIGH) của các chu kỳ liền nhau. Đoạn LOW bị giãn giữa 2 byte là ngoại lai."""
    # durations[0] là đoạn sau sườn đầu tiên: mức = đảo của level
    lows = durations_ns[0::2] if level == 1 else durations_ns[1::2]
    highs = durations_ns[1::2] if level == 1 else durations_ns[2::2]
    periods = [lo + hi for lo, hi in zip(lows, highs)]
    if len(periods) < 9:
        return None, periods
    return 1e9 / statistics.median(periods), periods


def on_wait(step, bench, ctx):
    kind, tag = step[0], step[1]
    arg = step[2] if len(step) > 2 else ""

    if kind == "slave_on":
        bench.cmd("Z ALL")
        bench.cmd(f"I2CS ON {SLAVE_ADDRESS} {SLAVE_HZ[tag]}")
        bench.cmd("I2CS RX")  # bỏ rác lúc chip khởi tạo chân
    elif kind == "load":
        bench.cmd(f"I2CS LOAD {arg}")
    elif kind == "expect_rx":
        time.sleep(0.02)  # task slave của ESP32 xử lý xong sự kiện nhận
        words = bench.cmd("I2CS RX").split()
        got = words[4] if len(words) > 4 else ""
        ctx.check(f"slave_rx_{arg[:2]}_{tag}", got == arg, f"mong đợi {arg}, slave nhận {got} ({words[3]} giao dịch)")
    elif kind == "scl":
        bench.send(f"CAP {SCL_NET} 3000 2000 NS")
        time.sleep(0.05)

        def after():
            words = bench.reply("CAP", timeout=5).split()
            level, durations = int(words[3]), [int(x) for x in words[5:]]
            hz, periods = scl_frequency(level, durations)
            low, high = SCL_RANGE[tag]
            if hz is None:
                ctx.check(f"scl_{tag}", False, f"chỉ bắt được {len(periods)} chu kỳ")
                return
            ctx.check(f"scl_{tag}", low <= hz <= high,
                      f"{hz / 1000:.1f}kHz (chấp nhận {low // 1000}..{high // 1000}kHz, {len(periods)} chu kỳ)")

        return "", after
    return ""

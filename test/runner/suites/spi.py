"""Kịch bản phía máy tính cho test HIL ch32v003_spi (firmware: test/ch32v003/src/spi/main.c).

ESP32 làm SPI slave Mode 0, nạp sẵn dữ liệu trả về (SPIS LOAD) trước mỗi giao dịch.
"""
import time

TIMEOUT_S = 60

# Chân slave theo cấu hình: SCK MOSI MISO CS
PINS = {
    "hw16": ("PC5", "PC6", "PC7", "PC0"),
    "hw256": ("PC5", "PC6", "PC7", "PC0"),
    "sw": ("PC4", "PA2", "PA1", "PC1"),
}
HCLK = 48_000_000
NOMINAL_HZ = {"hw16": HCLK // 16, "hw256": HCLK // 256}


def on_wait(step, bench, ctx):
    kind, tag, arg = step[0], step[1], step[2]

    if kind == "slave_on":
        bench.cmd("Z ALL")
        bench.cmd("SPIS ON " + " ".join(PINS[tag]))
    elif kind == "load":
        bench.cmd(f"SPIS LOAD {arg}")
    elif kind == "expect_rx":
        words = bench.cmd("SPIS RX 500").split()
        bits, got = int(words[3]), words[4]
        ctx.check(f"slave_rx_{len(arg) // 2}B_{tag}", got == arg and bits == len(arg) * 4,
                  f"mong đợi {arg}, slave nhận {got} ({bits} bit)")
    elif kind == "check_ramp":
        nbytes = int(arg)
        expected = bytes(range(nbytes)).hex().upper()
        words = bench.cmd("SPIS RX 500").split()
        ctx.check(f"slave_rx_ramp_{tag}", words[4] == expected and int(words[3]) == nbytes * 8,
                  f"slave nhận {words[4]} ({words[3]} bit)")
    elif kind == "cs_time":
        nbytes = int(arg)
        bench.cmd("SPIS LOAD -")
        bench.send(f"CAP {PINS[tag][3]} 3000 2000 NS")
        time.sleep(0.05)

        def after():
            words = bench.reply("CAP", timeout=5).split()
            level, durations = int(words[3]), [int(x) for x in words[5:]]
            if level != 1 or not durations:
                ctx.check(f"speed_{tag}", False, f"không bắt được xung CS: {words[3:]}")
                return
            low_ns = durations[0]  # CS nghỉ HIGH -> đoạn đầu tiên là thời gian CS ở mức LOW
            effective_hz = nbytes * 8 * 1e9 / low_ns
            if tag in NOMINAL_HZ:
                # Phần cứng: CS LOW gồm các byte liền mạch + vài µs chọn/nhả CS -> tốc độ hiệu dụng gần danh định.
                # ESP32 phát hiện sườn xuống đầu tiên trễ vài µs (vòng chờ không khóa ngắt) nên cho phép +5%
                nominal = NOMINAL_HZ[tag]
                ok = 0.85 * nominal <= effective_hz <= 1.05 * nominal
                note = f"danh định {nominal / 1000:.1f}kHz"
            else:
                ok = effective_hz >= 500_000
                note = "bit-bang, yêu cầu ≥ 500kHz"
            ctx.check(f"speed_{tag}", ok, f"CS LOW {low_ns / 1000:.1f}µs cho {nbytes} byte → {effective_hz / 1000:.0f}kHz ({note})")

        return "", after
    return ""

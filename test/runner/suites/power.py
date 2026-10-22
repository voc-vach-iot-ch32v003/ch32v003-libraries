"""Kịch bản phía máy tính cho test HIL ch32v003_power (firmware: test/ch32v003/src/power/main.c).

MARKER = PA1 (chip HIGH khi đang ngủ), WAKE_A = PA2, WAKE_B = PC4 (ESP32 tạo sườn đánh thức).
"""
import time

TIMEOUT_S = 300
MARKER, WAKE_A, WAKE_B = "PA1", "PA2", "PC4"
LSI_NOMINAL = 128_000
AWU_PRESCALERS = [2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 10240, 61440]

# Mức nghỉ của các chân trước khi ngủ và sườn đánh thức (chân, mức sau sườn)
PIN_CASES = {
    "falling": ({WAKE_A: 1}, (WAKE_A, 0)),
    "rising": ({WAKE_A: 0}, (WAKE_A, 1)),
    "change_up": ({WAKE_A: 0}, (WAKE_A, 1)),
    "change_down": ({WAKE_A: 1}, (WAKE_A, 0)),
    "second_pin": ({WAKE_A: 1, WAKE_B: 0}, (WAKE_B, 1)),
    "interrupt_timer": ({WAKE_A: 1}, (WAKE_A, 0)),
}


def awu_ticks(timeout_ms, window_formula=lambda w: w + 1):
    """Tổng số nhịp LSI thư viện cấu hình cho timeout (cùng thuật toán với powerPrepareTimer / powerCalculateAwu)."""
    ticks = -(-timeout_ms * LSI_NOMINAL // 1000)
    longest = 64 * AWU_PRESCALERS[-1]
    full, rest = divmod(ticks, longest)
    total = full * AWU_PRESCALERS[-1] * window_formula(63)
    if rest:
        for p in AWU_PRESCALERS:
            w_plus_one = -(-rest // p)
            if w_plus_one <= 64:
                total += p * window_formula(w_plus_one - 1)
                break
    return total


def read(bench, net):
    return int(bench.cmd(f"R {net}").split()[-1])


def on_wait(step, bench, ctx):
    kind = step[0]

    if kind == "pin":
        what, mode = step[1], step[2]
        idle, (edge_net, edge_level) = PIN_CASES[what]
        for net, level in idle.items():
            bench.cmd(f"W {net} {level}")
        time.sleep(0.02)

        def after():
            time.sleep(0.3)
            asleep = read(bench, MARKER)
            ctx.check(f"no_early_wake_{what}_{mode}", asleep == 1, f"MARKER={asleep} sau 300ms (1 = còn ngủ)")
            bench.cmd(f"W {edge_net} {edge_level}")
            time.sleep(0.05)
            awake = read(bench, MARKER)
            ctx.check(f"wake_{what}_{mode}", awake == 0, f"MARKER={awake} 50ms sau sườn (0 = đã thức)")

        return "", after

    if kind == "timer":
        timeout_ms = int(step[1])
        bench.cmd(f"Z {MARKER}")
        bench.send(f"INTERVAL {MARKER} {timeout_ms * 2 + 3000}")
        time.sleep(0.05)
        return ""

    if kind == "timer_done":
        timeout_ms, mode = int(step[1]), step[2]
        us = int(bench.reply("INTERVAL", timeout=timeout_ms / 1000 * 2 + 5).split()[-1])
        if us < 0:
            ctx.check(f"timer_{timeout_ms}_{mode}", False, "không đo được 2 sườn MARKER")
            return ""
        ticks = awu_ticks(timeout_ms)
        lsi = ticks / (us / 1e6)
        ctx.data[(mode, timeout_ms)] = us
        ctx.check(f"timer_{timeout_ms}_{mode}", us >= timeout_ms * 1000 * 0.75 and 100_000 <= lsi <= 160_000,
                  f"ngủ {us / 1000:.1f}ms (yêu cầu {timeout_ms}ms) → LSI ước tính {lsi / 1000:.1f}kHz")

        if timeout_ms == 32000 and (mode, 2000) in ctx.data:
            # Nhiều lần AWU nối tiếp: LSI ước tính phải khớp lần ngủ 2000ms (cùng chip, cùng chế độ)
            lsi_2000 = awu_ticks(2000) / (ctx.data[(mode, 2000)] / 1e6)
            ctx.check(f"timer_multi_chunk_{mode}", abs(lsi / lsi_2000 - 1) < 0.02,
                      f"LSI ước tính {lsi / 1000:.1f}kHz, lần 2000ms {lsi_2000 / 1000:.1f}kHz")
        if timeout_ms == 5200 and (mode, 2000) in ctx.data:
            # Tỉ lệ 2 lần ngủ loại bỏ sai số LSI, chỉ còn phụ thuộc công thức chu kỳ AWU
            got = us / ctx.data[(mode, 2000)]
            plus_one = awu_ticks(5200) / awu_ticks(2000)
            no_plus = awu_ticks(5200, lambda w: w) / awu_ticks(2000, lambda w: w)
            ctx.check(f"awu_formula_{mode}", abs(got / plus_one - 1) < 0.02,
                      f"tỉ lệ đo {got:.4f}, công thức (W+1)·P = {plus_one:.4f}, W·P = {no_plus:.4f}")
        return ""

    if kind == "swio_gpio":
        def after():
            time.sleep(0.15)
            level = read(bench, MARKER)
            ctx.check("swio_gpio_marker", level == 1, f"MARKER={level} (1 = SWIO vẫn ở chế độ GPIO sau khi thức)")

        return "", after
    return ""

"""Kịch bản phía máy tính cho test HIL ch32v003_delay + ch32v003_timer (firmware: test/ch32v003/src/timing/main.c)."""
import time

TIMEOUT_S = 60
SIGNAL_NET = "PA2"  # CH11, có trên cả 3 gói vỏ


def on_wait(step, bench, ctx):
    if step[0] != "cap":
        return ""
    name, segments, expect_us, tolerance_us = step[1], int(step[2]), int(step[3]), int(step[4])

    # Ra lệnh bắt sườn TRƯỚC, đợi ESP32 sẵn sàng, rồi mới trả lời để chip phát xung
    bench.send(f"Z ALL")
    bench.reply("Z ALL")
    idle_us = max(3 * expect_us, 5000)
    bench.send(f"CAP {SIGNAL_NET} 3000 {idle_us}")
    time.sleep(0.05)

    def after():
        words = bench.reply("CAP", timeout=5).split()
        durations = [int(x) for x in words[5:]]
        got = durations[:segments]
        if len(got) < segments:
            ctx.check(name, False, f"chỉ bắt được {len(durations)} đoạn: {durations}")
            return

        # delayMs là thước đo clock của chip: các phép đo sau quy đổi theo tỉ lệ clock này
        if name.startswith("delayMs"):
            ratio = sum(got) / len(got) / expect_us
            ctx.data["clock_ratio"] = ratio
            expected = expect_us
        else:
            expected = round(expect_us * ctx.data.get("clock_ratio", 1.0))
        if name.startswith("millis"):
            got = got[1:]  # đoạn đầu bắt đầu giữa 1ms nên ngắn hơn

        worst = max(got, key=lambda d: abs(d - expected))
        ok = all(abs(d - expected) <= tolerance_us for d in got)
        note = f" (clock chip lệch {(ctx.data['clock_ratio'] - 1) * 100:+.2f}%)" if name.startswith("delayMs") else ""
        ctx.check(name, ok, f"mong đợi {expected}±{tolerance_us}us, đo {got} (lệch nhất {worst - expected:+d}us){note}")

    return "", after

"""Test HIL dài cho ch32v003_timer: millis()/micros() chạy qua mốc tràn SysTick 89.48s (firmware: src/timer_wrap)."""
import time

TIMEOUT_S = 160
SIGNAL_NET = "PA2"  # CH11


def on_wait(step, bench, ctx):
    if step[0] != "interval":
        return ""
    name, expect_ms = step[1], int(step[2])

    bench.cmd("Z ALL")
    bench.send(f"INTERVAL {SIGNAL_NET} {expect_ms + 15000}")
    time.sleep(0.05)  # ESP32 gắn ngắt xong rồi mới cho chip phát sườn

    def after():
        us = int(bench.reply("INTERVAL", timeout=expect_ms / 1000 + 20).split()[-1])
        if us < 0:
            ctx.check(f"interval_{name}", False, "ESP32 không bắt được đủ 2 sườn")
            return
        measured_ms = us / 1000
        if name == "ref":
            ratio = measured_ms / expect_ms
            ctx.data["clock_ratio"] = ratio
            ctx.check("clock_reference", abs(ratio - 1) < 0.02,
                      f"delayMs(5000) đo {measured_ms:.2f}ms (clock chip lệch {(ratio - 1) * 100:+.2f}%)")
        else:
            expected = expect_ms * ctx.data.get("clock_ratio", 1.0)
            error = measured_ms - expected
            # millis() đã khớp tuyệt đối với bộ đếm SysTick (kiểm tra trên chip); phần lệch còn lại là HSI trôi
            # tần số giữa lúc đo thước và lúc đo 95s -> ngưỡng theo độ ổn định HSI (0.1%)
            ctx.check("millis_across_wrap", abs(error) <= expected * 0.001,
                      f"95000ms theo millis() đo {measured_ms:.1f}ms, mong đợi {expected:.1f}ms (lệch {error:+.2f}ms)")

    return "", after

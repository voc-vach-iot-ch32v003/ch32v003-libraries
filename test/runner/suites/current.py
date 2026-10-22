"""Kịch bản đo dòng tiêu thụ (firmware: test/ch32v003/src/current/main.c, máy đo: test/uno_meter/).

Chỉ chip có mạch đo ở chân GND (J4M6). Mỗi trạng thái chip giữ 3 giây; runner chờ chip vào trạng thái
rồi đo trung bình 1 giây.
"""
import time

from meter import Meter

CHIPS = ["J4M6"]
TIMEOUT_S = 60

# Khoảng chấp nhận (µA). Số đo tham chiếu trên J4M6 ở 3.3V: Standby ~8–10µA, Sleep 381µA, chạy 3.87mA
LIMITS = {
    "run": (1000, 10000),
    "standby_pin": (2, 20),
    "standby": (2, 20),
    "sleep": (150, 800),
}

_meter = None


def meter():
    global _meter
    if _meter is None:
        _meter = Meter()
        print(f"  Máy đo dòng: {_meter.port} ({_meter.cmd('CAL')})")
    return _meter


def on_wait(step, bench, ctx):
    if step[0] != "measure":
        return ""
    state = step[1]
    m = meter()

    def after():
        time.sleep(0.4)  # chip vào trạng thái + chờ trở đo ổn định
        ua, scale, overload = m.current(1000)
        low, high = LIMITS[state]
        ctx.data[state] = ua
        ctx.check(f"current_{state}", not overload and low <= ua <= high,
                  f"{ua:.2f}µA (thang {scale}{', QUÁ TẢI' if overload else ''}; chấp nhận {low}..{high}µA)")
        if state == "standby_pin":
            bench.cmd("OFF PA2")  # đánh thức chip (sườn xuống)
            time.sleep(0.05)
            bench.cmd("Z PA2")
        if state == "standby" and "standby_pin" in ctx.data:
            print(f"    → chi phí hẹn giờ (LSI + AWU): {ua - ctx.data['standby_pin']:+.2f}µA")

    return "", after

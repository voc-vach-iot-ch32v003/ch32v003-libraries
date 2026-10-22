#!/usr/bin/env python3
"""Nạp firmware giữ Standby vào một chip để đo tay dòng tiêu thụ (shunt 10kΩ ở chân GND + đồng hồ DCV).

    python3 test/runner/standby_hold.py [--chip J4M6] [--mode standby|sleep] [--port /dev/ttyUSB0]

Đỗ 2 chip còn lại, nạp test/ch32v003/src/standby_hold/ vào chip cần đo, thả nổi 16 kênh của ESP32.
Nhớ CẮM jumper nối tắt shunt trước khi chạy (chip ăn vài mA lúc nạp), rút ra sau khi chip đã vào Standby.
Đánh thức để đối chứng: gửi "OFF PA2" cho ESP32 (hoặc chạm PA2 xuống GND).
"""
import argparse
import subprocess
import time

from bench_check import Bench
from hil import CHIPS, MINICHLINK, build, park, part_package


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--chip", default="J4M6", choices=CHIPS)
    parser.add_argument("--mode", default="standby", choices=["standby", "sleep"])
    parser.add_argument("--port", default="/dev/ttyUSB0")
    args = parser.parse_args()

    bench = Bench(args.port)
    bench.cmd("PWR ALL ON")
    time.sleep(1.5)
    for chip in CHIPS:
        if chip != args.chip:
            park(bench, chip)

    firmware = build(f"{args.mode}_hold_{args.chip}")
    bench.cmd("Z ALL")
    bench.cmd(f"RST {args.chip}")
    r = subprocess.run([MINICHLINK, "-w", str(firmware), "flash", "-b"], capture_output=True, text=True, timeout=30)
    got = part_package(r.stdout + r.stderr)
    if got != args.chip:
        print(f"LỖI: minichlink nhận '{got}', không phải {args.chip}\n{r.stdout[-800:]}{r.stderr[-800:]}")
        return 1

    bench.cmd("Z ALL")
    print(f"Đã nạp {args.chip}: chip vào {args.mode} sau ~1 giây. Bây giờ rút jumper và đọc điện áp trên shunt (DCV).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

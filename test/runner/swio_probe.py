#!/usr/bin/env python3
"""Thí nghiệm: chân nào ở chế độ ANALOG làm SWIO mất kết nối (firmware test/ch32v003/src/swio_probe/).

    python3 test/runner/swio_probe.py [--chip J4M6]

Mỗi biến thể: nạp -> khởi động lại -> dò minichlink -i ở giây thứ 3 (chân còn bình thường) và giây thứ 7
(sau khi đổi chân). Cuối cùng nạp lại firmware đỗ trong 5 giây đầu.
"""
import argparse
import subprocess
import time

from bench_check import Bench
from hil import CHIPS, MINICHLINK, build, park, part_package

VARIANTS = {0: "không đổi gì (đối chứng)", 1: "PD1 = ANALOG", 2: "PD4 + PD5 = ANALOG", 3: "mọi chân = ANALOG"}


def probe():
    r = subprocess.run([MINICHLINK, "-i"], capture_output=True, text=True, timeout=30)
    return part_package(r.stdout + r.stderr)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--chip", default="J4M6", choices=CHIPS)
    args = parser.parse_args()

    bench = Bench("/dev/ttyUSB0")
    bench.cmd("PWR ALL ON")
    time.sleep(2)
    for chip in CHIPS:
        if chip != args.chip:
            park(bench, chip)

    for variant, label in VARIANTS.items():
        firmware = build(f"swio_probe{variant}_{args.chip}")
        bench.cmd("Z ALL")
        bench.cmd(f"RST {args.chip}")
        subprocess.run([MINICHLINK, "-w", str(firmware), "flash", "-b"], capture_output=True, text=True, timeout=30)
        bench.cmd(f"RST {args.chip}")
        start = time.time()
        time.sleep(3)
        before = probe()
        time.sleep(max(0, start + 7 - time.time()))
        after = probe()
        print(f"{label:28s} giây 3: {before or 'KHÔNG NHẬN':10s} giây 7: {after or 'KHÔNG NHẬN'}")

    park(bench, args.chip)
    print("Đã đỗ lại", args.chip)


if __name__ == "__main__":
    main()

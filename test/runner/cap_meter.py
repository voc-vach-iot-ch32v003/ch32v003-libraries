#!/usr/bin/env python3
"""Máy đo dòng nhỏ bằng thời gian nạp tụ (A4M6 trên bàn test, firmware test/ch32v003/src/cap_meter/).

    python3 test/runner/cap_meter.py [--lines 10] [--no-flash]

Đỗ F4P6 / J4M6, nạp máy đo vào A4M6, rồi đọc các dòng CAPMETER qua SWIO (minichlink -T).
A4M6 chạy máy đo giữ SWD bật: muốn nạp chip khác thì chạy hil.py / standby_hold.py như bình thường
(runner đỗ lại A4M6 trước).
"""
import argparse
import os
import pty
import select
import subprocess
import time

from bench_check import Bench
from hil import CHIPS, MINICHLINK, build, park, part_package


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--lines", type=int, default=10)
    parser.add_argument("--no-flash", action="store_true", help="chỉ đọc, không nạp lại")
    args = parser.parse_args()

    bench = Bench("/dev/ttyUSB0")
    bench.cmd("PWR ALL ON")
    time.sleep(2)
    if not args.no_flash:
        for chip in CHIPS:
            if chip != "A4M6":
                park(bench, chip)
    bench.cmd("Z ALL")  # Các net PA1 / PA2 / PD4 / PC3 của máy đo dùng chung với kênh ESP32

    command = [MINICHLINK, "-T"]
    if not args.no_flash:
        firmware = build("cap_meter_A4M6")
        bench.cmd("RST A4M6")
        command = [MINICHLINK, "-w", str(firmware), "flash", "-b", "-T"]

    master, slave = pty.openpty()
    proc = subprocess.Popen(command, stdin=slave, stdout=slave, stderr=slave, close_fds=True)
    os.close(slave)
    buffer, shown = b"", 0
    deadline = time.time() + 15 + args.lines
    try:
        while shown < args.lines and time.time() < deadline:
            ready, _, _ = select.select([master], [], [], 0.2)
            if not ready:
                continue
            buffer += os.read(master, 4096)
            while b"\n" in buffer:
                raw, buffer = buffer.split(b"\n", 1)
                line = raw.decode(errors="replace").strip()
                if line.startswith("Part Type") and part_package(line) != "A4M6":
                    print(f"LỖI: nạp nhầm chip ({line})")
                    return 1
                start = line.find("CAPMETER")
                if start >= 0:
                    print(line[start:])
                    shown += line[start:].startswith("CAPMETER I")
    finally:
        proc.terminate()
        proc.wait(timeout=3)
        os.close(master)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

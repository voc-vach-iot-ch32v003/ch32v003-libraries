#!/usr/bin/env python3
"""Kiểm tra kết nối bàn test HIL: ESP32 điều khiển nguồn + WCH-LinkE nạp từng chip trong cửa sổ 1 giây.

    python3 test/runner/bench_check.py [--port /dev/ttyUSB0]
"""
import argparse
import os
import subprocess
import sys
import time

import serial  # pyserial (có sẵn trong môi trường của PlatformIO)

MINICHLINK = os.path.expanduser("~/.platformio/packages/tool-minichlink/minichlink")
CHIPS = ["F4P6", "A4M6", "J4M6"]


class Bench:
    def __init__(self, port):
        self.serial = serial.Serial()
        self.serial.port = port
        self.serial.baudrate = 115200
        self.serial.timeout = 1
        self.serial.dtr = False  # Không kích mạch auto-reset của ESP32 khi mở cổng
        self.serial.rts = False
        self.serial.open()
        # Mở cổng vẫn có thể làm ESP32 khởi động lại: chờ dòng READY (tối đa 3 giây) rồi mới gửi lệnh
        deadline = time.time() + 3
        while time.time() < deadline:
            if "READY" in self.serial.readline().decode(errors="replace"):
                break
        self.serial.reset_input_buffer()

    def cmd(self, line):
        self.send(line)
        return self.reply(line)

    def send(self, line):
        """Gửi lệnh nhưng chưa đọc trả lời (dùng cho lệnh chờ tín hiệu như CAP/FREQ)."""
        self.serial.write((line + "\n").encode())

    def reply(self, line="", timeout=1.0):
        """Đọc 1 dòng trả lời của lệnh đã gửi, chờ tối đa timeout giây."""
        self.serial.timeout = timeout
        text = self.serial.readline().decode(errors="replace").strip()
        self.serial.timeout = 1
        if not text.startswith("OK"):
            raise RuntimeError(f"{line} -> {text or 'không phản hồi'}")
        return text


def probe():
    """Trả về (đã nhận chip?, dòng thông tin chip) bằng minichlink -i."""
    out = subprocess.run([MINICHLINK, "-i"], capture_output=True, text=True, timeout=20)
    text = out.stdout + out.stderr
    ok = "Could not setup interface" not in text and "Unknown chip" not in text
    info = [l for l in text.splitlines() if "Part Type" in l or "Part UUID" in l]
    return ok, info


# Mã CHIPID của WCH: 0x0030xxxx = F4P6, 0x0031xxxx = F4U6, 0x0032xxxx = A4M6, 0x0033xxxx = J4M6
PART_PACKAGE = {"30": "F4P6", "31": "F4U6", "32": "A4M6", "33": "J4M6"}


def package_of(info):
    for line in info:
        if line.startswith("Part Type"):
            code = line.split(":")[1].strip().split("-")
            return PART_PACKAGE.get(code[1], "?")
    return "-"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", default="/dev/ttyUSB0")
    args = parser.parse_args()

    bench = Bench(args.port)
    print(bench.cmd("PING"))
    print(bench.cmd("PWR ALL ON"))
    time.sleep(1.5)  # Đủ để cả 3 chip qua cửa sổ 1 giây và tắt SWD
    print(bench.cmd("STATUS"))

    results = {}
    ok, info = probe()
    results["Tất cả đã đỗ (không chip nào được trả lời)"] = not ok
    print(f"[1] Dò khi cả 3 đã đỗ: {f'{package_of(info)} vẫn trả lời (SAI)' if ok else 'không chip nào trả lời (ĐÚNG)'}")

    for chip in CHIPS:
        print(bench.cmd(f"RST {chip}"))
        ok, info = probe()
        got = package_of(info)
        results[f"Nạp được {chip} trong cửa sổ 1 giây"] = ok and got == chip
        print(f"[{chip}] {'nhận ' + got if ok else 'KHÔNG NHẬN'}  " + " | ".join(info))
        if ok and got != chip:
            # Trên SWIO chung, 2 chip cùng trả lời -> dữ liệu đọc về là AND bit của cả hai (sai Part Type/UUID).
            # Thường do chip khác bị reset theo: kiểm tra dây cắm nhầm vào chân NRST (D7) hoặc chung nguồn.
            print(f"    ⚠ Có chip khác cùng trả lời với {chip} (dữ liệu bị trộn). Kiểm tra dây NRST/nguồn.")
        # minichlink có thể để chip ở trạng thái dừng: khởi động lại để chip đỗ lại
        bench.cmd(f"RST {chip}")
        time.sleep(1.5)

    print("\n=== KẾT QUẢ ===")
    for name, passed in results.items():
        print(f"{'ĐẠT ' if passed else 'LỖI '} {name}")
    return 0 if all(results.values()) else 1


if __name__ == "__main__":
    sys.exit(main())

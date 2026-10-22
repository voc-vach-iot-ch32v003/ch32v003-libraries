"""Máy đo dòng Arduino Uno (firmware test/uno_meter/): tự tìm cổng serial bằng lệnh PING.

    python3 test/runner/meter.py [số lần] [ms mỗi lần]   # đọc nhanh, không cần bàn test
"""
import glob
import time

import serial  # pyserial (có sẵn trong môi trường của PlatformIO)

BENCH_PORT = "/dev/ttyUSB0"  # ESP32 của bàn test, không dò


class Meter:
    def __init__(self, port=None):
        self.serial = None
        for candidate in [port] if port else sorted(glob.glob("/dev/ttyACM*") + glob.glob("/dev/ttyUSB*")):
            if candidate == BENCH_PORT:
                continue
            if self._try_open(candidate):
                self.port = candidate
                return
        raise RuntimeError("không tìm thấy máy đo dòng Uno (lệnh PING không ai trả lời 'OK PONG METER')")

    def _try_open(self, port):
        try:
            s = serial.Serial(port, 115200, timeout=0.3)
        except serial.SerialException:
            return False
        # Mở cổng làm Uno khởi động lại (~2 giây): chờ READY rồi xác nhận bằng PING
        deadline = time.time() + 3
        while time.time() < deadline:
            if "READY METER" in s.readline().decode(errors="replace"):
                break
        s.reset_input_buffer()
        s.write(b"PING\n")
        s.timeout = 1
        if "PONG METER" in s.readline().decode(errors="replace"):
            self.serial = s
            return True
        s.close()
        return False

    def cmd(self, line, timeout=5):
        self.serial.reset_input_buffer()
        self.serial.write((line + "\n").encode())
        deadline = time.time() + timeout
        while time.time() < deadline:
            text = self.serial.readline().decode(errors="replace").strip()
            if text.startswith(("OK", "ERR")):
                if text.startswith("ERR"):
                    raise RuntimeError(f"{line} -> {text}")
                return text
        raise RuntimeError(f"{line} -> không phản hồi")

    def current(self, duration_ms=500, mode="AUTO"):
        """Trả về (µA, thang 'UA'|'MA', quá tải?)."""
        words = self.cmd(f"I {duration_ms} {mode}", timeout=duration_ms / 1000 + 3).split()
        return float(words[2]), words[3], words[-1] == "OVER"


if __name__ == "__main__":
    # Đọc nhanh: python3 test/runner/meter.py [số lần] [ms mỗi lần]
    import sys

    count = int(sys.argv[1]) if len(sys.argv) > 1 else 5
    duration = int(sys.argv[2]) if len(sys.argv) > 2 else 1000
    m = Meter()
    print(f"Máy đo: {m.port} ({m.cmd('CAL')})")
    for _ in range(count):
        ua, scale, overload = m.current(duration)
        print(f"{ua:10.2f} µA  thang {scale}{'  QUÁ TẢI' if overload else ''}")

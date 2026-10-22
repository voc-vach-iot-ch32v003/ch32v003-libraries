#!/usr/bin/env python3
"""Runner kiểm thử HIL: nạp firmware test vào từng chip trong cửa sổ 1 giây, điều phối ESP32 theo các dòng HIL:...

    python3 test/runner/hil.py <suite> [--chip F4P6 A4M6 J4M6] [--port /dev/ttyUSB0]

- Firmware test: test/ch32v003/src/<suite>/main.c, env PlatformIO <suite>_<chip>.
- Kịch bản phía máy tính: test/runner/suites/<suite>.py, cung cấp on_wait(step, bench, ctx) -> chuỗi trả lời.
- Kết quả: in ra màn hình + lưu test/runner/reports/<suite>-<thời gian>.json
"""
import argparse
import importlib.util
import json
import os
import pty
import select
import signal
import subprocess
import sys
import time
from pathlib import Path

from bench_check import Bench, PART_PACKAGE

ROOT = Path(__file__).resolve().parents[1]  # test/
FIRMWARE_DIR = ROOT / "ch32v003"
SUITES_DIR = Path(__file__).resolve().parent / "suites"
REPORT_DIR = Path(__file__).resolve().parent / "reports"
MINICHLINK = os.path.expanduser("~/.platformio/packages/tool-minichlink/minichlink")
PIO = "pio"
CHIPS = ["F4P6", "A4M6", "J4M6"]


class Context:
    """Ghi nhận kết quả của một lần chạy (một suite trên một chip)."""

    def __init__(self, suite, chip):
        self.suite, self.chip = suite, chip
        self.checks = []  # (tên, đạt?, chi tiết, nguồn: chip|runner)
        self.data = {}  # chỗ để kịch bản lưu trạng thái giữa các bước

    def check(self, name, passed, detail="", source="runner"):
        self.checks.append((name, bool(passed), detail, source))
        mark = "ĐẠT " if passed else "LỖI "
        print(f"    {mark} {name} {detail}")


def load_suite(name):
    path = SUITES_DIR / f"{name}.py"
    spec = importlib.util.spec_from_file_location(f"suite_{name}", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def build(env):
    r = subprocess.run([PIO, "run", "-d", str(FIRMWARE_DIR), "-e", env], capture_output=True, text=True)
    if r.returncode != 0:
        print(r.stdout[-3000:], r.stderr[-3000:])
        raise RuntimeError(f"build {env} lỗi")
    return FIRMWARE_DIR / ".pio" / "build" / env / "firmware.bin"


def part_package(text):
    for line in text.splitlines():
        if line.startswith("Part Type"):
            return PART_PACKAGE.get(line.split(":")[1].strip().split("-")[1], "?")
    return None


def park(bench, chip):
    """Nạp lại firmware đỗ cho chip vừa test."""
    firmware = build(f"park_{chip}")
    bench.cmd("Z ALL")
    bench.cmd(f"RST {chip}")
    r = subprocess.run([MINICHLINK, "-w", str(firmware), "flash", "-b"], capture_output=True, text=True, timeout=30)
    got = part_package(r.stdout + r.stderr)
    if got != chip:
        print(f"  ⚠ Đỗ lại {chip}: minichlink nhận '{got}'")
    # Firmware đỗ vừa khởi động còn bật SWD trong 1 giây: chờ hết cửa sổ để không chen vào lần nạp chip kế tiếp
    time.sleep(1.5)


def run_on_chip(suite_name, suite, chip, bench, timeout_s):
    ctx = Context(suite_name, chip)
    print(f"\n=== {suite_name} trên {chip} ===")
    firmware = build(f"{suite_name}_{chip}")

    # 1. Bus về Hi-Z, khởi động lại chip -> cửa sổ nạp 1 giây -> nạp + chạy + mở terminal SWIO
    bench.cmd("Z ALL")
    if hasattr(suite, "setup"):
        suite.setup(bench, ctx)
    bench.cmd(f"RST {chip}")
    master, slave = pty.openpty()
    proc = subprocess.Popen([MINICHLINK, "-w", str(firmware), "flash", "-b", "-T"],
                            stdin=slave, stdout=slave, stderr=slave, close_fds=True)
    os.close(slave)

    buffer = b""
    log = []
    detected = None
    finished = False
    deadline = time.time() + timeout_s
    try:
        while time.time() < deadline and not finished:
            ready, _, _ = select.select([master], [], [], 0.2)
            if not ready:
                if proc.poll() is not None:
                    break
                continue
            try:
                chunk = os.read(master, 4096)
            except OSError:
                break
            buffer += chunk
            while b"\n" in buffer:
                raw, buffer = buffer.split(b"\n", 1)
                line = raw.decode(errors="replace").strip()
                if not line:
                    continue
                log.append(line)
                if line.startswith("Part Type") and detected is None:
                    detected = part_package(line)
                    if detected != chip:
                        ctx.check("nạp đúng chip", False, f"minichlink nhận {detected}")
                        finished = True
                        break
                # Sau khi chip bật lại SWD (VD: thức dậy từ chế độ ngủ) có thể dính vài byte rác trước "HIL:"
                start = line.find("HIL:")
                if start < 0:
                    continue
                words = line[start + 4:].split()
                kind, args = words[0], words[1:]
                if kind == "WAIT":
                    # on_wait trả về chuỗi trả lời, hoặc (trả lời, hàm chạy SAU khi đã trả lời chip)
                    # VD: ra lệnh CAP cho ESP32 trước, trả lời chip để nó phát xung, rồi mới đọc kết quả CAP
                    result = suite.on_wait(args, bench, ctx)
                    reply, after = result if isinstance(result, tuple) else (result or "", None)
                    os.write(master, (reply + "\n").encode())
                    if after:
                        after()
                elif kind == "CHECK":
                    ctx.check(args[0], args[1] == "PASS", " ".join(args[2:]), source="chip")
                elif kind == "END":
                    # Số check chip tự đếm phải khớp số dòng CHECK runner nhận được (phát hiện dòng bị mất)
                    counts = dict(w.split("=") for w in args if "=" in w)
                    seen = sum(1 for c in ctx.checks if c[3] == "chip")
                    expected = int(counts.get("pass", 0)) + int(counts.get("fail", 0))
                    if seen != expected:
                        ctx.check("nhận đủ dòng CHECK của chip", False, f"chip báo {expected}, runner nhận {seen}")
                    finished = True
                elif kind == "BEGIN":
                    print(f"  {line}")
        if not finished:
            ctx.check("chạy hết bộ test", False, f"không thấy HIL:END sau {timeout_s}s")
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            proc.kill()
        os.close(master)
        # Luôn đỗ lại chip (kể cả khi lỗi / bị dừng): chip còn chạy firmware test sẽ lái các net dùng chung
        park(bench, chip)

    ctx.data["log"] = log
    return ctx


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("suite")
    parser.add_argument("--chip", nargs="+", default=None)
    parser.add_argument("--port", default="/dev/ttyUSB0")
    args = parser.parse_args()

    suite = load_suite(args.suite)
    chips = args.chip or getattr(suite, "CHIPS", CHIPS)
    timeout_s = getattr(suite, "TIMEOUT_S", 60)

    # SIGTERM (VD: lệnh timeout) -> thoát qua finally để chip đang test vẫn được đỗ lại
    signal.signal(signal.SIGTERM, lambda *_: sys.exit(1))

    bench = Bench(args.port)
    bench.cmd("PWR ALL ON")
    time.sleep(1.5)
    # Lần chạy trước bị dừng giữa chừng có thể để lại chip chạy firmware test: đỗ cả 3 chip trước khi bắt đầu
    for chip in CHIPS:
        park(bench, chip)

    results = [run_on_chip(args.suite, suite, chip, bench, timeout_s) for chip in chips]
    bench.cmd("Z ALL")

    print("\n=== TỔNG KẾT ===")
    total_fail = 0
    for ctx in results:
        fails = [c for c in ctx.checks if not c[1]]
        total_fail += len(fails)
        print(f"{ctx.chip}: {len(ctx.checks) - len(fails)} đạt, {len(fails)} lỗi")
        for name, _, detail, source in fails:
            print(f"   LỖI [{source}] {name} {detail}")

    REPORT_DIR.mkdir(exist_ok=True)
    stamp = time.strftime("%Y%m%d-%H%M%S")
    report = {ctx.chip: {"checks": ctx.checks, "log": ctx.data.get("log", [])} for ctx in results}
    (REPORT_DIR / f"{args.suite}-{stamp}.json").write_text(json.dumps(report, ensure_ascii=False, indent=1))
    return 1 if total_fail else 0


if __name__ == "__main__":
    sys.exit(main())

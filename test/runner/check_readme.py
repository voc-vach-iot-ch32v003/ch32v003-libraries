#!/usr/bin/env python3
"""Biên dịch thử code mẫu Quick Start trong README.md của thư viện trên cả 3 gói vỏ.

    python3 test/runner/check_readme.py gpio timer intmath ...
"""
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]  # libraries/ch32v003
FIRMWARE_DIR = ROOT / "test" / "ch32v003"
OUT = FIRMWARE_DIR / "src" / "_readme" / "main.c"
CHIPS = ["J4M6", "A4M6", "F4P6"]


def quick_start(lib):
    text = (ROOT / f"ch32v003_{lib}" / "README.md").read_text()
    match = re.search(r"## 📝 [^\n]*\n.*?```c\n(.*?)```", text, re.S)
    if not match:
        raise SystemExit(f"{lib}: không tìm thấy khối ```c trong mục Quick Start")
    return match.group(1)


def main():
    failed = 0
    for lib in sys.argv[1:]:
        OUT.parent.mkdir(exist_ok=True)
        OUT.write_text(quick_start(lib))
        for chip in CHIPS:
            r = subprocess.run(["pio", "run", "-d", str(FIRMWARE_DIR), "-e", f"readme_{chip}"],
                               capture_output=True, text=True)
            problems = [l for l in (r.stdout + r.stderr).splitlines()
                        if ("error" in l or "warning:" in l) and "ch32fun.c" not in l]
            ok = r.returncode == 0 and not problems
            failed += not ok
            print(f"{'ĐẠT ' if ok else 'LỖI '} {lib} {chip}")
            for line in problems[:10]:
                print("     ", line)
    OUT.unlink(missing_ok=True)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())

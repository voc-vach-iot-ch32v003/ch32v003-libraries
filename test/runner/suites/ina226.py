"""Kịch bản phía máy tính cho test HIL ch32v003_ina226 (firmware: test/ch32v003/src/ina226/main.c).

ESP32 làm I2C slave 0x40 trên CH13 (PC1 SDA) / CH14 (PC2 SCL), giả lập INA226. Cơ chế như suite bmi160:
- "regs <reg>=<hex>[,<hex>...] ...": khai báo giá trị thanh ghi (mỗi giá trị 2 byte, byte cao trước); nhiều giá trị =
  lần lượt cho từng lần đọc (giá trị cuối giữ nguyên);
- "read <reg>": firmware sắp đọc thanh ghi <reg> -> nạp đúng giá trị (I2CS LOAD);
- "expect_rx <tên> <hex|->": so chuỗi byte chip đã ghi từ lần kiểm tra trước.

Giá trị mong đợi trong firmware được tính độc lập ở đây (self_check) và khớp với bảng ghi cứng trong main.c.
"""
import time

TIMEOUT_S = 90


def lsb_na(max_ua):
    return max(1, (max_ua * 1000 + 32767) // 32768)


def calibration(max_ua, shunt_uohm):
    return 5_120_000_000_000 // (lsb_na(max_ua) * shunt_uohm)


def trunc_div(n, d):
    q = abs(n) // d
    return q if n >= 0 else -q


def self_check():
    """Kiểm tra bảng ghi cứng trong main.c bằng công thức Python (chạy khi nạp suite)."""
    lsb = lsb_na(1_000_000)
    assert lsb == 30518 and calibration(1_000_000, 100_000) == 1677
    assert lsb_na(2_000_000) == 61036 and calibration(2_000_000, 10_000) == 8388
    assert lsb_na(800_000) == 24415 and calibration(800_000, 100_000) == 2097
    assert lsb_na(100_000) == 3052 and calibration(100_000, 100_000) == 16775
    for raw, uv in [(1000, 2500), (-1000, -2500), (-32768, -81920), (32767, 81917), (1, 2), (-1, -3)]:
        assert (raw * 5) // 2 == uv
    for raw, mv in [(0, 0), (1, 1), (32767, 40958), (12000, 15000), (65535, 81918)]:
        assert (raw * 5) // 4 == mv
    for raw, na, ua in [(1000, 30518000, 30518), (-1000, -30518000, -30518), (32767, 999983306, 999983),
                        (-32768, -1000013824, -1000013), (1, 30518, 30), (-1, -30518, -30), (0, 0, 0)]:
        assert raw * lsb == na and trunc_div(na, 1000) == ua
    for raw, uw in [(0, 0), (1, 762), (1000, 762950), (65535, 49999928)]:
        assert raw * 25 * lsb // 1000 == uw


self_check()


def on_wait(step, bench, ctx):
    kind = step[0]
    if kind == "slave_on":
        bench.cmd("Z ALL")
        bench.cmd("I2CS ON 40 400000")
        bench.cmd("I2CS RX")  # bỏ rác lúc chip khởi tạo chân
    elif kind == "regs":
        ctx.data["regs"] = {reg: values.split(",") for reg, values in (word.split("=") for word in step[1:])}
    elif kind == "read":
        values = ctx.data.get("regs", {}).get(step[1])
        if not values:
            ctx.check(f"read_{step[1]}", False, "thanh ghi chưa khai báo giá trị")
            return ""
        bench.cmd(f"I2CS LOAD {values.pop(0) if len(values) > 1 else values[0]}")
    elif kind == "expect_rx":
        name, want = step[1], step[2]
        time.sleep(0.02)  # task slave của ESP32 xử lý xong sự kiện nhận
        words = bench.cmd("I2CS RX").split()
        got = words[4] if len(words) > 4 else "-"
        ctx.check(f"rx_{name}", got == want, f"mong đợi {want}, slave nhận {got} ({words[3]} giao dịch)")
    return ""

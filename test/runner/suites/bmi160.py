"""Kịch bản phía máy tính cho test HIL ch32v003_bmi160 (firmware: test/ch32v003/src/bmi160/main.c).

ESP32 làm I2C slave 0x69 (địa chỉ BMI160 khi SA0 = VCC) trên CH13 (PC1 SDA) / CH14 (PC2 SCL), giả lập cảm biến:
- "regs <reg>=<hex>[,<hex>...] ...": khai báo giá trị các thanh ghi; nhiều giá trị = lần lượt cho từng lần đọc
  (giá trị cuối giữ nguyên cho các lần sau);
- "read <reg>": firmware sắp đọc thanh ghi <reg> (i2cReadBurst bị bọc bằng --wrap) -> nạp đúng giá trị của nó.
  ESP32 không kéo giãn SCL, byte nạp thừa sẽ nằm lại FIFO và làm hỏng các lần ghi sau, nên chỉ nạp ngay trước khi đọc;
- "expect_rx <tên> <hex|->": so chuỗi byte chip đã ghi (địa chỉ thanh ghi + giá trị) từ lần kiểm tra trước.
"""
import time

TIMEOUT_S = 60


def on_wait(step, bench, ctx):
    kind = step[0]
    if kind == "slave_on":
        bench.cmd("Z ALL")
        bench.cmd("I2CS ON 69 400000")
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

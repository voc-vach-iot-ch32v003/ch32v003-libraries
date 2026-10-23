"""Kịch bản phía máy tính cho test HIL ch32v003_max7219 (firmware: test/ch32v003/src/max7219/main.c).

ESP32 nghe lén SPI (SNIFF) trên SCK / MOSI / CS, runner so từng khung CS với khung dựng theo datasheet:
chuỗi 3 IC, mỗi IC nhận 2 byte (thanh ghi, dữ liệu), IC xa vi điều khiển nhất nhận trước.
"""
import time

TIMEOUT_S = 60
CASCADE = 3

# SCK MOSI CS theo cấu hình
PINS = {
    "hw": ("PC5", "PC6", "PC2"),
    "sw": ("PC4", "PA2", "PC2"),
}

NOOP = (0x00, 0x00)


def frame(pairs):
    """pairs[i] = (thanh ghi, dữ liệu) cho IC i (0 = gần vi điều khiển nhất) -> hex của khung CS."""
    return "".join(f"{reg:02X}{data:02X}" for reg, data in reversed(pairs))


def same(reg, data):
    return frame([(reg, data)] * CASCADE)


EXPECTED = {
    # Display test tắt, không giải mã, quét 8 digit, độ sáng 5, xóa 8 digit, rồi mới bật hiển thị
    "init": [same(0x0F, 0x00), same(0x09, 0x00), same(0x0B, 0x07), same(0x0A, 0x05)]
    + [same(digit, 0x00) for digit in range(1, 9)]
    + [same(0x0C, 0x01)],
    "write_register_1": [frame([NOOP, (0x03, 0xA5), NOOP])],
    "write_register_0": [frame([(0x01, 0x81), NOOP, NOOP])],
    "write_register_out": [],
    "write_all": [same(0x08, 0x3C)],
    "write_chain": [frame([(0x05, 0x11), (0x05, 0x22), (0x05, 0x33)])],
    "write_chain_stride": [frame([(0x05, 0xA0), (0x05, 0xA8), (0x05, 0xB0)])],
    "intensity_clamp": [same(0x0A, 0x0F)],
    "power_off": [same(0x0C, 0x00)],
    "power_on": [same(0x0C, 0x01)],
    "clear_all": [same(digit, 0x00) for digit in range(1, 9)],
}


MAX_GAP_NS = 600  # Lượt lấy mẫu chậm hơn mức này (~nửa chu kỳ SCK 750kHz) thì có thể đã lỡ sườn
MAX_TRIES = 3


def on_wait(step, bench, ctx):
    kind = step[0]

    if kind == "setup":
        bench.cmd("Z ALL")
        return ""

    if kind == "sniff":
        tag = step[1]
        sck, mosi, cs = PINS[tag]
        # Lệnh qua cầu USB–UART tới ESP32 chậm hơn trả lời qua SWIO tới chip: chờ 30ms để ESP32 vào vòng nghe
        # trước khi chip phát (ESP32 chờ CS tối đa 100ms, khóa ngắt)
        bench.send(f"SNIFF {sck} {mosi} {cs} 100 5000")
        time.sleep(0.03)

        def after():
            words = bench.reply("SNIFF", timeout=3).split()
            ctx.data["sniff"] = words

        return "", after

    if kind == "verify":
        tag, case = step[1], step[2]
        words = ctx.data.pop("sniff")
        got = [w for w in words[3:] if not w.startswith("gap=")]
        gap = next((int(w[4:]) for w in words if w.startswith("gap=")), 0)
        key = f"{case}_{tag}"
        tries = ctx.data.get(key, 0) + 1
        if gap > MAX_GAP_NS and tries < MAX_TRIES:
            # ESP32 bị dừng tạm giữa lúc bắt (core 0 dừng core 1 bằng ngắt mức cao): bắt lại
            ctx.data[key] = tries
            print(f"    (bắt lại {key}: ESP32 bị dừng {gap}ns)")
            return "retry"

        expected = EXPECTED[case]
        ok = got == expected and gap <= MAX_GAP_NS
        if got == expected:
            detail = f"{len(got)} khung" + (f", VD {got[0]}" if got else "")
        else:
            diff = next((i for i, (g, e) in enumerate(zip(got, expected)) if g != e), min(len(got), len(expected)))
            detail = (f"{len(got)} khung (chuẩn {len(expected)}), khác từ khung {diff}: "
                      f"{got[diff] if diff < len(got) else '-'} / chuẩn {expected[diff] if diff < len(expected) else '-'}")
        if tries > 1:
            detail += f" (lần bắt thứ {tries})"
        if gap > MAX_GAP_NS:
            detail += f", ESP32 bị dừng {gap}ns"
        ctx.check(key, ok, detail)
        return "ok"

    return ""

"""Kiểm tra khung SPI bằng lệnh SNIFF của ESP32, dùng chung cho các suite MAX7219 (max7219, led_matrix).

Firmware dùng test/ch32v003/include/sniff.h:
  "setup <tag>"          runner đưa kênh về Hi-Z
  "sniff <tag> <case>"   runner bật SNIFF rồi trả lời, chip thực hiện thao tác
  "verify <tag> <case>"  runner so khung bắt được với khung mong đợi: trả "ok", hoặc "retry" nếu lần bắt không đáng
                         tin (core 1 ESP32 bị core 0 dừng tạm bằng ngắt mức cao) để chip làm lại thao tác
"""
import time

MAX_GAP_NS = 600  # Lượt lấy mẫu chậm hơn mức này (~nửa chu kỳ SCK 750kHz) thì có thể đã lỡ sườn
MAX_TRIES = 3

# SCK MOSI CS theo cấu hình (CS = PC2: cả 3 chân nằm trong thanh ghi GPIO.in của ESP32)
PINS = {
    "hw": ("PC5", "PC6", "PC2"),
    "sw": ("PC4", "PA2", "PC2"),
}


def frame(pairs):
    """pairs[i] = (thanh ghi, dữ liệu) cho IC i (0 = gần vi điều khiển nhất) -> hex của khung CS (IC xa nhất trước)."""
    return "".join(f"{reg:02X}{data:02X}" for reg, data in reversed(pairs))


def same(reg, data, count):
    """Khung ghi cùng (thanh ghi, dữ liệu) cho cả chuỗi count IC."""
    return frame([(reg, data)] * count)


def max7219_init_frames(count):
    """Các khung của max7219Init(): Display test tắt, không giải mã, quét 8 digit, độ sáng 5, xóa 8 digit, bật."""
    return ([same(0x0F, 0x00, count), same(0x09, 0x00, count), same(0x0B, 0x07, count), same(0x0A, 0x05, count)]
            + [same(digit, 0x00, count) for digit in range(1, 9)]
            + [same(0x0C, 0x01, count)])


def handle(step, bench, ctx, expected_for):
    """Xử lý các bước setup / sniff / verify. expected_for(case) trả danh sách hex khung mong đợi."""
    kind = step[0]

    if kind == "setup":
        bench.cmd("Z ALL")
        return ""

    if kind == "sniff":
        sck, mosi, cs = PINS[step[1]]
        # Lệnh qua cầu USB–UART tới ESP32 chậm hơn trả lời qua SWIO tới chip: chờ 30ms để ESP32 vào vòng nghe
        # trước khi chip phát (ESP32 chờ CS tối đa 100ms, khóa ngắt)
        bench.send(f"SNIFF {sck} {mosi} {cs} 100 5000")
        time.sleep(0.03)

        def after():
            ctx.data["sniff"] = bench.reply("SNIFF", timeout=3).split()

        return "", after

    if kind == "verify":
        tag, case = step[1], step[2]
        words = ctx.data.pop("sniff")
        got = [w for w in words[3:] if not w.startswith("gap=")]
        gap = next((int(w[4:]) for w in words if w.startswith("gap=")), 0)
        key = f"{case}_{tag}"
        tries = ctx.data.get(key, 0) + 1
        if gap > MAX_GAP_NS and tries < MAX_TRIES:
            ctx.data[key] = tries
            print(f"    (bắt lại {key}: ESP32 bị dừng {gap}ns)")
            return "retry"

        expected = expected_for(case)
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
        ctx.check(key, got == expected and gap <= MAX_GAP_NS, detail)
        return "ok"

    return None

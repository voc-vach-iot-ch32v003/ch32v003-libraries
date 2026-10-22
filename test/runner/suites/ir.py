"""Kịch bản phía máy tính cho test HIL ch32v003_ir (firmware: test/ch32v003/src/ir/main.c).

- rx: ESP32 phát khung đã giải điều chế (IRTX) vào PC4 của chip, chip tự giải mã và báo CHECK.
- tx: ESP32 bắt tín hiệu có sóng mang trên PC1 (IRCAP), runner so đường bao với khung chuẩn dựng từ đặc tả
  (độc lập với thư viện) và kiểm tra tần số sóng mang.
"""
import time

TIMEOUT_S = 90
RX_NET = "PC4"
TX_NET = "PC1"

# leaderMark, leaderSpace, mark0, space0, mark1, space1, stopMark, bits, chu kỳ lặp khung (us)
TIMINGS = {
    "NEC": (9000, 4500, 560, 560, 560, 1690, 560, 32, 108000),
    "SAMSUNG": (4500, 4500, 560, 560, 560, 1690, 560, 32, 108000),
    "SONY": (2400, 600, 600, 600, 1200, 600, 0, 12, 45000),
    "TCL": (4000, 4000, 590, 1050, 590, 2450, 590, 76, 0),
}


def envelope(protocol, hex_data, frames):
    """Đường bao chuẩn: [Mark, Space, Mark, ...] (us), các khung cách nhau theo chu kỳ lặp."""
    lead_m, lead_s, m0, s0, m1, s1, stop, bits, period = TIMINGS[protocol]
    data = bytes.fromhex(hex_data)
    sequence = []
    for f in range(frames):
        if f and protocol == "NEC":
            frame = [9000, 2250, 560]  # Mã lặp NEC
        else:
            frame = [lead_m, lead_s]
            for i in range(bits):
                one = (data[i >> 3] >> (i & 7)) & 1
                frame += [m1 if one else m0, s1 if one else s0]
            if stop:
                frame.append(stop)
            else:
                frame.pop()  # Sony: Space của bit cuối là khoảng nghỉ sau khung
        if sequence:
            sequence.append(period - previous)
        previous = sum(frame)
        sequence += frame
    return sequence


def on_wait(step, bench, ctx):
    kind = step[0]

    if kind == "setup":
        bench.cmd("Z ALL")
        bench.cmd(f"W {RX_NET} 1")  # Mắt thu rảnh = HIGH
        return ""

    if kind == "rx":
        _, protocol, hex_data, frames = step[1], step[2], step[3], step[4]

        def after():
            time.sleep(0.03)  # Chip đã nhận trả lời và đang chờ tín hiệu
            bench.send(f"IRTX {RX_NET} {protocol} {hex_data} {frames}")
            bench.reply("IRTX", timeout=3)

        return "", after

    if kind == "tx":
        tag, protocol, hex_data, frames, hz = step[1], step[2], step[3], int(step[4]), int(step[5])
        expected = envelope(protocol, hex_data, frames)
        bench.send(f"IRCAP {TX_NET} 3000 30000")
        time.sleep(0.05)

        def after():
            words = bench.reply("IRCAP", timeout=8).split()
            carrier, duty, count = int(words[3]), int(words[4]), int(words[5])
            got = [int(x) for x in words[6:6 + count]]

            ctx.check(f"carrier_{tag}", abs(carrier - hz) <= hz * 0.03 and 40 <= duty <= 60,
                      f"{carrier}Hz duty {duty}% (chuẩn {hz}Hz ±3%, duty 40..60%)")

            # Mỗi đoạn: lệch ≤ 3% (clock HSI của chip) + 40us (đường bao Mark ngắn hơn tới 1 chu kỳ sóng mang)
            if len(got) != len(expected):
                ctx.check(f"envelope_{tag}", False, f"{len(got)} đoạn, chuẩn {len(expected)}: {got[:8]}...")
                return
            worst, worst_index = 0.0, 0
            for i, (g, e) in enumerate(zip(got, expected)):
                ratio = abs(g - e) / (e * 0.03 + 40)
                if ratio > worst:
                    worst, worst_index = ratio, i
            ctx.check(f"envelope_{tag}", worst <= 1.0,
                      f"{len(got)} đoạn, lệch nhất đoạn {worst_index}: {got[worst_index]}us / chuẩn "
                      f"{expected[worst_index]}us ({worst * 100:.0f}% dung sai)")

        return "", after

    return ""

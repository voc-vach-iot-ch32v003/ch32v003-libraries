"""Test HIL ch32v003_intmath: chip tự so với bảng kết quả của máy tính, không cần ESP32."""
TIMEOUT_S = 30


def on_wait(step, bench, ctx):
    return ""

import os

from SCons.Script import DefaultEnvironment

# Ép PlatformIO luôn bật màu ANSI trong mọi luồng con
os.environ["PLATFORMIO_FORCE_ANSI"] = "true"

env = DefaultEnvironment()
current_env = env["PIOENV"]

env.AddCustomTarget(
    name="upload_and_sdi_monitor",
    title="Upload and Monitor SDI",
    dependencies=[],
    actions=[
        f"pio run -t upload -e {current_env}",
        f"pio run -t sdi_printf_monitor -e {current_env}"
    ],
)

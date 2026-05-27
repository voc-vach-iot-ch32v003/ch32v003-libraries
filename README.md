# CH32V003 Project Template for PlatformIO (ch32fun)

A lightweight, modular, and Arduino-like (`setup`/`loop`) project template for the ultra-low-cost **CH32V003** RISC-V
MCU. Built on top of the ultra-fast **ch32fun** framework and optimized for **PlatformIO**.

*Một dự án mẫu gọn nhẹ, kiến trúc module, hỗ trợ cấu trúc giống Arduino (`setup`/`loop`) dành cho dòng vi điều khiển
RISC-V siêu rẻ **CH32V003**. Phát triển dựa trên framework **ch32fun** và tối ưu sẵn cho **PlatformIO**.*

---

## 🚀 Features (Tính năng nổi bật)

- **PlatformIO Ready**: No complex toolchain setup. Just open with VS Code + PlatformIO and click Build/Upload.
- **Arduino-style Workflow**: Simplified `setup()` and `loop()` structure in `main.c`.
- **Anti-Brick Delay**: Built-in 5-second startup delay to ensure you can *always* re-flash the MCU, even if your code
  reconfigures the SWIO pin.
- **`ch32v003_gpio`**: Comprehensive GPIO control supporting multiple packages (J4M6, A4M6, F4P6). Includes single-pin
  and full-port operations (`pinMode`, `pinModePort`, `digitalWrite/Read`).
- **`ch32v003_delay` & `ch32v003_timer`**: Accurate timing wrappers (`delayMs`, `delayUs`, `ticks`, `micros`, `millis`,
  `elapsedUs/Ms`).
- **`ch32v003_debug`**: `printf` debugging directly over the 1-wire SWIO interface.
- **`ch32v003_ir`**: Infrared remote control library supporting **NEC, Samsung, and Sony** protocols (transmit and
  receive).
- **`ch32v003_power`**: Low-power management utilities (wake-up pin registration, Standby/Sleep modes).
- **Highly Configurable**: Optimize flash/RAM usage by enabling/disabling modules via `sys_config.h` and customizing
  setups via `user_config.h`.

---

## 📂 Project Structure (Cấu trúc thư mục)

```text
├── .pio/                  # PlatformIO internal build folder (auto-generated)
├── include/               # Header files (.h)
│   ├── funconfig.h        # ch32fun configuration
│   ├── sys_config.h       # Enable/disable libraries & system parameters
│   ├── user_config.h      # User hardware pin definitions & macros
│   ├── ch32v003_debug.h   # SWIO printf debugging header
│   ├── ch32v003_delay.h   # Delay wrappers header
│   ├── ch32v003_gpio.h    # Pin & Port manipulation header
│   ├── ch32v003_timer.h   # System ticks & counters header
│   ├── ch32v003_ir.h      # IR Tx/Rx header
│   └── ch32v003_power.h   # Sleep & Standby header
├── src/                   # Source files (.c)
│   ├── main.c             # Application entry point (setup/loop)
│   ├── ch32v003_delay.c   # Delay wrappers implementation
│   ├── ch32v003_gpio.c    # Pin & Port manipulation implementation
│   ├── ch32v003_timer.c   # System ticks & counters implementation
│   ├── ch32v003_ir.c      # IR Tx/Rx implementation
│   └── ch32v003_power.c   # Sleep & Standby implementation
├── platformio.ini         # PlatformIO configuration file
├── LICENSE                # MIT License
└── README.md              # This documentation
```

---

## 🛠️ Getting Started (Hướng dẫn bắt đầu)

### Prerequisite (Yêu cầu hệ thống)

1. Install [VS Code](https://visualstudio.com).
2. Install the **PlatformIO IDE** extension inside VS Code.
3. Connect your **WCH-LinkE** (or minichlink-compatible programmer) to the CH32V003 chip.

### Configuration (Cấu hình)

1. Open `include/sys_config.h` to enable/disable the modules you need to save Flash memory.
2. Open `include/user_config.h` to define your specific target board layout (`CH32V003_J4M6`, `CH32V003_A4M6`, or
   `CH32V003_F4P6`) and map your custom functional pins.

### Code Example (`src/main.c`)

```c
#include "ch32fun.h"
#include "ch32v003_delay.h"
#include "ch32v003_gpio.h"
#include "ch32v003_debug.h"

#define LED_PIN  MCU_PIN3 // Depend on your user_config.h target board

void setup()
{
    pinMode(LED_PIN, OUTPUT);
    printf("System initialized!\n");
}

void loop()
{
    digitalWrite(LED_PIN, HIGH);
    delayMs(1000);
    digitalWrite(LED_PIN, LOW);
    delayMs(1000);
}

int main()
{
    SystemInit(); 
    delayMs(5000); // CRITICAL: Always wait 5s for minichlink to catch connection before user loop.
    setup();
    while (1)
    {
        loop();
    }
}
```

### Build & Flash (Biên dịch và Nạp)

- **Build**: Click the ✔ (Checkmark) icon on the PlatformIO bottom status bar.
- **Upload**: Click the ➔ (Right arrow) icon to flash using `minichlink`.
- **Serial Monitor**: Click the 🔌 (Plug) icon to read `printf` data from SWIO.

---

## 📜 License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

## 🤝 Acknowledgments

Special thanks to Charles Lohr ([@cnlohr](https://github.com/cnlohr/ch32fun)) and the Community for creating the
incredible **ch32fun**
framework.
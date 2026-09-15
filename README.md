# Bộ Thư Viện Dùng Chung Cho CH32V003 (PlatformIO & ch32v003fun)

Kho lưu trữ tổng hợp các thư viện driver và module phần cứng được thiết kế tối ưu theo phong cách Arduino (`setup`/`loop`) dành cho vi điều khiển RISC-V **CH32V003**. Dự án xây dựng trên nền tảng framework **ch32v003fun** và quản lý theo mô hình **Git Submodules** giúp linh hoạt trong việc tích hợp tập trung hoặc kéo độc lập từng thư viện qua URL.

🔗 **Organization GitHub chính thức**: **[voc-vach-iot-ch32v003](https://github.com/voc-vach-iot-ch32v003)**  
🔗 **Repository tổng hợp (Mono-repo)**: **[voc-vach-iot-ch32v003/ch32v003-libraries](https://github.com/voc-vach-iot-ch32v003/ch32v003-libraries)**

> 📌 **Dự án mẫu đi kèm**: Kho thư viện này được thiết kế để kết hợp trực tiếp với template dự án tại:  
> 🔗 **[voc-vach-iot-ch32v003/ch32v003-template](https://github.com/voc-vach-iot-ch32v003/ch32v003-template)**

---

## 🚀 Danh Sách Thư Viện (Submodules)

Mỗi thư viện bên dưới được quản lý dưới dạng một Submodule độc lập. Chi tiết về API, sơ đồ đấu nối và ví dụ sử dụng được trình bày tại `README.md` riêng trong từng thư mục submodule:

| Module Thư Viện           | Mô Tả Tóm Tắt                                                 | Repository Độc Lập                                                                  |
| :------------------------ | :------------------------------------------------------------ | :---------------------------------------------------------------------------------- |
| **`ch32v003_gpio`**       | Điều khiển GPIO & Port cho các package (SOP8, SOP16, TSSOP20) | [ch32v003_gpio](https://github.com/voc-vach-iot-ch32v003/ch32v003_gpio)             |
| **`ch32v003_delay`**      | Tạo trễ chính xác Microsecond & Millisecond                   | [ch32v003_delay](https://github.com/voc-vach-iot-ch32v003/ch32v003_delay)           |
| **`ch32v003_timer`**      | Bộ đếm thời gian hệ thống (`millis`, `micros`, `elapsed`)     | [ch32v003_timer](https://github.com/voc-vach-iot-ch32v003/ch32v003_timer)           |
| **`ch32v003_debug`**      | Debug `printf` trực tiếp qua 1-wire SWIO                      | [ch32v003_debug](https://github.com/voc-vach-iot-ch32v003/ch32v003_debug)           |
| **`ch32v003_power`**      | Quản lý chế độ tiết kiệm điện (Sleep, Standby) & Wakeup       | [ch32v003_power](https://github.com/voc-vach-iot-ch32v003/ch32v003_power)           |
| **`ch32v003_spi`**        | Driver giao tiếp Bus SPI phần cứng                            | [ch32v003_spi](https://github.com/voc-vach-iot-ch32v003/ch32v003_spi)               |
| **`ch32v003_max7219`**    | Driver điều khiển IC quét LED MAX7219 qua SPI                 | [ch32v003_max7219](https://github.com/voc-vach-iot-ch32v003/ch32v003_max7219)       |
| **`ch32v003_led_matrix`** | Quản lý lớp đồ họa LED Matrix (Frame Buffer, Font 5x7, Line)  | [ch32v003_led_matrix](https://github.com/voc-vach-iot-ch32v003/ch32v003_led_matrix) |
| **`ch32v003_ir`**         | Thu/phát và giải mã tín hiệu IR (NEC, Sony, Samsung, TCL)     | [ch32v003_ir](https://github.com/voc-vach-iot-ch32v003/ch32v003_ir)                 |

---

## 📂 Cấu Trúc Repository

```text
ch32v003-libraries/
├── ch32v003_debug/       # [Submodule] Debug printf qua SWIO
├── ch32v003_delay/       # [Submodule] Hàm delay us & ms
├── ch32v003_gpio/        # [Submodule] Thao tác GPIO & ánh xạ pin
├── ch32v003_ir/          # [Submodule] Thu/phát IR đa giao thức
├── ch32v003_led_matrix/  # [Submodule] Tầng đồ họa LED Matrix
├── ch32v003_max7219/     # [Submodule] Driver IC MAX7219
├── ch32v003_power/       # [Submodule] Quản lý nguồn & Sleep mode
├── ch32v003_spi/         # [Submodule] Giao tiếp SPI phần cứng
├── ch32v003_timer/       # [Submodule] Bộ đếm thời gian SysTick
├── .gitmodules           # Cấu hình liên kết Submodules
├── LICENSE               # Giấy phép MIT
└── README.md             # Tài liệu tổng quan dự án

```

---

## 📥 Hướng Dẫn Clone Repository (Kèm Submodules)

Nếu bạn muốn quản lý tất cả các thư viện cục bộ tại máy cá nhân:

```bash
# Clone toàn bộ repo cùng tất cả các submodules
git clone --recursive [https://github.com/voc-vach-iot-ch32v003/ch32v003-libraries.git](https://github.com/voc-vach-iot-ch32v003/ch32v003-libraries.git)

# Nếu lỡ clone thông thường, chạy lệnh sau để cập nhật submodules
git submodule update --init --recursive

```

---

## 🛠️ Hướng Dẫn Tích Hợp Vào Dự Án PlatformIO

Bạn có thể sử dụng bộ thư viện theo **2 cách**:

### Cách 1: Sử dụng URL Git trực tiếp qua `lib_deps` (Khuyên dùng cho dự án độc lập)

Không cần clone toàn bộ kho thư viện về máy, PlatformIO sẽ tự động tải các thư viện cần thiết từ GitHub về dự án khi biên dịch:

```ini
[env:ch32v003_power_manager]
extends = _base_ch32v003
board = genericCH32V003J4M6
build_src_filter = -<*> +<ch32v003/*.c>
build_flags = -D CH32V003_J4M6

# Khai báo trực tiếp Git URL của các thư viện độc lập
lib_deps =
    https://github.com/voc-vach-iot-ch32v003/ch32v003_delay.git
    https://github.com/voc-vach-iot-ch32v003/ch32v003_gpio.git
    https://github.com/voc-vach-iot-ch32v003/ch32v003_debug.git
    https://github.com/voc-vach-iot-ch32v003/ch32v003_power.git

```

### Cách 2: Trỏ thư mục cục bộ qua `lib_extra_dirs` (Dành cho làm việc Offline / Monorepo)

Khi làm việc với kho mã nguồn tập trung (đã clone toàn bộ về máy):

```ini
[env]
platform = ch32v
framework = ch32v003fun
upload_protocol = minichlink
debug_tool = minichlink

# Trỏ tới thư mục chứa bộ thư viện tập trung
lib_extra_dirs = ../../libraries/ch32v003-libraries/
lib_ldf_mode = deep+

# Khai báo tên thư viện sử dụng
lib_deps =
    ch32v003_gpio
    ch32v003_delay
    ch32v003_power

```

---

## 📜 Giấy Phép (License)

Dự án được phân phối dưới dạng **Giấy phép MIT** - xem file [LICENSE](https://www.google.com/search?q=LICENSE) để biết thêm chi tiết.

## 🤝 Lời Cảm Ơn

Trân trọng gửi lời cảm ơn đến Charles Lohr ([@cnlohr](https://github.com/cnlohr/ch32fun)) và cộng đồng đã phát triển framework **ch32v003fun**.

---

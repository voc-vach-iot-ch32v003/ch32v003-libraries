# Thư Viện Dùng Chung Cho CH32V003 (PlatformIO & ch32v003fun)

Bộ thư viện chia sẻ tập trung, thiết kế dạng module tối ưu theo phong cách Arduino (`setup`/`loop`) dành cho dòng vi
điều khiển RISC-V **CH32V003**. Dự án được xây dựng trên nền tảng framework **ch32v003fun** và tối ưu cho mô hình quản
lý thư viện tập trung trong **PlatformIO** (`lib_extra_dirs`).

🔗 **Repository chính thức**: **[voc-vach-iot/ch32v003-libraries](https://github.com/voc-vach-iot/ch32v003-libraries)**

> 📌 **Dự án mẫu đi kèm**: Kho thư viện này được thiết kế để kết hợp trực tiếp với template dự án thực thi tại:
> 🔗 **[voc-vach-iot/ch32v003-template](https://github.com/voc-vach-iot/ch32v003-template)**

---

## 🚀 Các Thư Viện Hỗ Trợ

* **`ch32v003_gpio`**: Tối ưu điều khiển GPIO cho các dòng đóng vỏ (J4M6 - SOP8, A4M6 - SOP16, F4P6 - TSSOP20). Hỗ trợ
  thao tác từng pin hoặc cả Port (`pinMode`, `pinModePort`, `digitalWrite/Read`).
* **`ch32v003_delay` & `ch32v003_timer**`: Thư viện quản lý thời gian chính xác (`delayMs`, `delayUs`, `ticks`,
  `micros`, `millis`, `elapsedUs/Ms`).
* **`ch32v003_debug`**: Hỗ trợ debug `printf` trực tiếp qua giao diện 1-wire SWIO.
* **`ch32v003_ir`**: Thư viện phát và nhận thu hồng ngoại hỗ trợ đa giao thức (**NEC, Sony, Samsung, TCL**).
* **`ch32v003_power`**: Quản lý các chế độ tiết kiệm năng lượng (Sleep/Standby) và đăng ký chân Wake-up.

---

## 📂 Cấu Trúc Kho Thư Viện Tập Trung

```text
/mnt/learning/programing-language/code/iot/libraries/ch32v003/
├── ch32v003_debug/        # Module debug printf qua SWIO
│   ├── library.json
│   ├── ch32v003_debug.c
│   └── ch32v003_debug.h
├── ch32v003_delay/        # Các hàm delay microsecond & millisecond
│   ├── library.json
│   ├── ch32v003_delay.c
│   └── ch32v003_delay.h
├── ch32v003_gpio/         # Thao tác GPIO & ánh xạ chân MCU theo package vỏ
│   ├── library.json
│   ├── ch32v003_gpio.c
│   └── ch32v003_gpio.h
├── ch32v003_ir/           # Driver thu/phát IR (NEC, Sony, Samsung, TCL)
│   ├── library.json
│   ├── ch32v003_ir.c
│   └── ch32v003_ir.h
├── ch32v003_power/        # Quản lý nguồn (Sleep, Standby, Wakeup)
│   ├── library.json
│   ├── ch32v003_power.c
│   └── ch32v003_power.h
├── ch32v003_timer/        # Bộ đếm thời gian hệ thống (millis/micros)
│   ├── library.json
│   ├── ch32v003_timer.c
│   └── ch32v003_timer.h
├── LICENSE                # Giấy phép MIT
└── README.md              # Tài liệu hướng dẫn

```

---

## ⚙️ Cơ Chế Tương Tác Giữa Template & Thư Viện

Kho thư viện tập trung tự động thẩm thấu cấu hình phần cứng và tính năng được định nghĩa trong
repo [ch32v003-template](https://github.com/voc-vach-iot/ch32v003-template) thông qua chỉ thị preprocessor
`__has_include`:

### 1. File Cấu Hình Tại Template (`include/`)

* **`funconfig.h`**: Cấu hình phần cứng gốc của framework `ch32v003fun` (xung nhịp SysTick HCLK 48MHz, bật debug
  `printf` qua 1-wire SWIO).
* **`sys_config.h`**: Quản lý bật/tắt (1/0) các driver như `DELAY`, `TIMER`, `IR`, `POWER` để tối ưu dung lượng bộ nhớ
  Flash theo từng dự án.
* **`user_config.h`**: Định nghĩa sơ đồ chân phần cứng cụ thể cho từng mạch ứng dụng.

### 2. File Mã Nguồn Dự Án (`src/main.c`)

Mã nguồn tại ứng dụng được tổ chức chuẩn theo phong cách Arduino (`setup`/`loop`), tích hợp sẵn cơ chế chống brick chip
khi nạp qua SWIO:

```c
#include "ch32v003fun.h"
#include <ch32v003_gpio.h>
#include <ch32v003_delay.h>
#include <ch32v003_debug.h>

#define LED_PIN MCU_PIN3 // Tự động nhận diện theo cờ -DCH32V003_xxx trong platformio.ini

void setup() {
    pinMode(LED_PIN, OUTPUT);
    printf("Chay thu nghiem thu vien tap trung!\n");
}

void loop() {
    digitalWrite(LED_PIN, HIGH);
    delayMs(1000);
    digitalWrite(LED_PIN, LOW);
    delayMs(1000);
}

int main() {
    SystemInit(); 
    delayMs(5000); // LƯU Ý: Chờ 5s giúp chống brick chip khi re-flash qua SWIO
    setup();
    while (1) {
        loop();
    }
}

```

---

## 🛠️ Hướng Dẫn Tích Hợp Chi Tiết

### Cấu hình `platformio.ini` tại dự án thực thi

Khai báo đường dẫn tương đối trỏ về thư mục kho thư viện tập trung bằng thuộc tính `lib_extra_dirs` và liên kết thư viện
bằng `lib_deps`:

```ini
[platformio]
# Dòng này quyết định xem mặc định khi nhấn nút Build/Upload thì con chip nào sẽ chạy
default_envs = CH32V003J4M6

# Cấu hình chung cho cả 3 con chip để tránh lặp lại code (DRY)
[env]
platform = ch32v
framework = ch32v003fun
upload_protocol = minichlink
debug_tool = minichlink
extra_scripts = ch32v003_extra_scripts.py
lib_extra_dirs = ../../libraries/ch32v003/
lib_ldf_mode = deep+
lib_deps =
    ch32v003_delay

# 1. Cấu hình cho bản 8 chân (SOP8)
[env:CH32V003J4M6]
board = genericCH32V003J4M6
build_flags = -DCH32V003_J4M6

# 2. Cấu hình cho bản 16 chân (SOP16)
[env:CH32V003A4M6]
board = genericCH32V003A4M6
build_flags = -DCH32V003_A4M6

# 3. Cấu hình cho bản 20 chân (TSSOP20)
[env:CH32V003F4P6]
board = genericCH32V003F4P6
build_flags = -DCH32V003_F4P6

```

---

## 📜 Giấy Phép (License)

Dự án được phân phối dưới dạng **Giấy phép MIT** - xem file [LICENSE](https://www.google.com/search?q=LICENSE) để biết
thêm chi tiết.

## 🤝 Lời Cảm Ơn

Trân trọng gửi lời cảm ơn đến Charles Lohr ([@cnlohr](https://github.com/cnlohr/ch32fun)) và cộng đồng đã phát triển
framework **ch32v003fun** tuyệt vời.

Xây dựng và phát triển bởi **Vọc Vạch IoT**.
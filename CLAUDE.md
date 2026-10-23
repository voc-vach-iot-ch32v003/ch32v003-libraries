# CLAUDE.md — Bộ thư viện CH32V003 (Vọc Vạch IoT)

Mono-repo gom các thư viện `ch32v003_*` cho CH32V003 (framework **ch32v003fun**, PlatformIO). Mỗi thư mục `ch32v003_*` là **một git submodule riêng** (remote `git@vocvachiot:voc-vach-iot-ch32v003/<tên>.git`): commit/tag trong từng submodule, rồi cập nhật con trỏ submodule ở repo gốc.

Giao tiếp với người dùng bằng **tiếng Việt**. Việc đang làm và còn lại: **`TASKS.md`** (cập nhật sau mỗi bước).

## Quy chuẩn bắt buộc — đọc trước khi sửa thư viện

Template nằm ở `../template/` (đọc `embedded-library-router.md` để chọn khâu):

| Khâu | File |
| :-- | :-- |
| Refactor code | `../template/embedded-library-refactor-core.md` |
| Ví dụ `examples/` | `../template/embedded-library-examples-core.md` |
| README | `../template/embedded-library-docs-core.md` |
| Release | `../template/embedded-library-release-core.md` |
| Đặc thù CH32V003 | `../template/embedded-profile-ch32v003.md` |

Tóm tắt những điểm hay sai:

- **Cấu trúc**: mã nguồn trong `src/` (`<lib>.h`, `<lib>.c`, `<lib>_types.h`, `<lib>_defs.h`), `library.json` có `dependencies` dạng **object** `{ "name": ..., "version": "<git url>#v<x.y.z>" }`.
- **Ghim phiên bản phụ thuộc**: mọi phụ thuộc (trong `library.json` và `lib_deps` của `examples/*/platformio.ini`) trỏ tới **tag phát hành ổn định gần nhất** (`...ch32v003_gpio.git#v2.0.1`), không trỏ nhánh mới nhất, để thư viện tầng dưới ra bản phá vỡ tương thích không làm hỏng thư viện tầng trên. Ví dụ ghim chính thư viện của nó theo tag đang phát hành. Khi phát hành thư viện tầng dưới, xem lại thư viện tầng trên có nên nâng tag phụ thuộc không.
- **Doxygen**: chỉ ở file `.h` (`@file`, `@author Vọc Vạch IoT`, `@brief`); **không** `@version` / `@date`; **không** khối header ở đầu `.c`. Mọi hàm/enum/struct/trường public có doc tiếng Việt.
- **Đặt tên**: type `<MODULE>_<FullPascalCase>_t` (không viết tắt: `Configuration`, `DriverHandle`), hàm `moduleVerb()` camelCase, macro `UPPER_SNAKE`. **Ngoại lệ có chủ đích**: `gpio`, `delay`, `timer` giữ tên hàm kiểu Arduino (`pinMode`, `delayMs`, `millis`...).
- **Handle pattern**: driver nhận con trỏ handle (`I2C_DriverHandle_t`, `SPI_DriverHandle_t`...), không dùng biến `static` toàn cục cho trạng thái thiết bị (phải hỗ trợ nhiều thiết bị).
- **Gói vỏ**: phân nhánh bằng `#if defined(CH32V003_J4M6 | _A4M6 | _F4P6)`; chân lấy từ `ch32v003_gpio_pins.h` (`MCU_PINx`, `MCU_I2C_SDA`...).
- **Cấu hình người dùng**: macro mặc định trong `<lib>_defs.h` bọc `#ifndef`, cho phép ghi đè qua `sys_config.h` của dự án bằng `__has_include("sys_config.h")`. Log debug (`printf`) chỉ bật qua `<LIB>_DEBUG_LOG_ENABLE`, mặc định 0.
- **Thay đổi API (breaking)**: hỏi người dùng trước; ghi vào `TASKS.md` và release note (code cũ vs mới).

## Phong cách code (giữ giống code sẵn có)

Allman braces, thụt 4 space, chú thích tiếng Việt ngắn gọn, khối phân đoạn `// ====...`. Ví dụ/ứng dụng dùng `static void setup(void)` / `static void loop(void)` và `main()`:

```c
int main(void)
{
    SystemInit();
    delayMs(1000); // BẮT BUỘC: cửa sổ nạp 1 giây (bàn test HIL dựa vào đây để nạp lại chip)
    setup();
    while (1) { loop(); }
}
```

## Ràng buộc phần cứng CH32V003 cần nhớ khi viết code

- Lõi **RV32EC: không có lệnh nhân/chia phần cứng** → `*`, `/`, `%` với biến là gọi hàm libgcc (hàng chục–hàng trăm chu kỳ). Ưu tiên dịch bit, tính sẵn ngoài vòng lặp, tránh chia trong ngắt/vòng lặp nóng.
- RAM 2KB, Flash 16KB. Tránh buffer lớn trên stack, tránh kéo `printf` vào thư viện khi không cần.
- Build có `-flto`, `-ffunction-sections`, `--gc-sections`: hàm không dùng bị loại bỏ.
- SysTick: `ch32v003_timer` chiếm `SysTick_Handler` (ngắt 1ms). `Delay_Ms/Us` của ch32fun đọc `SysTick->CNT` chạy tự do — không reset CNT, không bật auto-reload.

## Kiểm thử

| Loại | Lệnh | Ghi chú |
| :-- | :-- | :-- |
| Logic thuần trên máy tính | `test/host/run.sh` | gcc, không cần phần cứng (intmath...) |
| Build mọi thư viện × 3 chip | `pio run -d test/ch32v003` | env `smoke_*`; build phải sạch warning của thư viện |
| Kiểm tra bàn test | `$PY test/runner/bench_check.py` | ESP32 + WCH-LinkE + 3 chip |
| Test HIL một thư viện | `$PY test/runner/hil.py <suite> [--chip F4P6 A4M6 J4M6]` | firmware `test/ch32v003/src/<suite>/` + kịch bản `test/runner/suites/<suite>.py`; suite hiện có: `gpio`, `timing`, `intmath`, `i2c`, `spi`, `max7219`, `led_matrix`, `ir`, `bmi160`, `power` (~1 phút/chip), `timer_wrap` (~2 phút/chip), `current` (J4M6, cần máy đo Uno) |
| Code mẫu README biên dịch được | `$PY test/runner/check_readme.py <lib...>` | trích khối ```c ở mục Quick Start, build 3 chip |

`$PY` = `/mnt/data/.system/environments/penv/bin/python` (python của PlatformIO, có sẵn `pyserial`). `minichlink` ở `~/.platformio/packages/tool-minichlink/minichlink`.

### Bàn test HIL (đọc `test/WIRING.md` trước khi đụng phần cứng)

- ESP32 DevKit 30 chân (`/dev/ttyUSB0`, firmware `test/esp32/`) nối **16 kênh** tới 3 chip qua trở 1kΩ; bảng kênh duy nhất ở **`test/wiring/gen.py`** (`CH`), sinh lại tài liệu/bản vẽ từ đó.
- Nguồn chip lấy thẳng từ GPIO ESP32: **GPIO2 → F4P6, GPIO22 → A4M6, GPIO23 → J4M6** (HIGH = có điện). Không dùng GPIO12 làm nguồn (chân strapping).
- SWIO của 3 chip **nối chung** vào một WCH-LinkE. Chip không test chạy **firmware đỗ** (`test/ch32v003/src/park/`): chân Hi-Z, tắt SWD sau 1 giây. Muốn nạp chip X: đưa 16 kênh về Hi-Z → `RST X` (tắt nguồn 50ms) → nạp trong cửa sổ 1 giây → sau khi test xong nạp lại firmware đỗ.
- Nhận diện chip qua `minichlink -i` → `Part Type`: `00-30` F4P6, `00-32` A4M6, `00-33` J4M6. Part Type/UUID "lạ" = **2 chip cùng trả lời** (dữ liệu là AND bit) → có chip bị reset theo / dây cắm nhầm.
- Lệnh ESP32 (Serial 115200, mỗi lệnh 1 dòng, trả lời `OK ...`/`ERR ...`): xem đầu `test/esp32/src/main.cpp`.
- I2C slave của ESP32 (`I2CS`, CH13/CH14): ESP32 bản gốc **không kéo giãn SCL** → dữ liệu master sẽ đọc phải nạp trước bằng `I2CS LOAD`; `Z ALL` tự tắt slave. Byte nạp thừa nằm lại FIFO và làm hỏng các lần ghi sau → mỗi lần nạp phải được đọc hết. Thư viện đọc nhiều thanh ghi trong một hàm: link firmware test với `-Wl,--wrap=i2cReadBurst` (+ `build_unflags = -flto`) để báo runner nạp ngay trước từng lần đọc (xem suite `bmi160`). Bus cần 2 trở kéo lên 4.7kΩ tại J4M6: `MODE PC1 PD` rồi `R PC1` phải đọc 1.
- IR (`IRTX` / `IRCAP`): ESP32 phát khung đã giải điều chế như mắt thu (rảnh HIGH) và bắt tín hiệu có sóng mang (đường bao + Hz + duty); thời gian chuẩn của giao thức đặt độc lập trong `test/esp32/src/bench_ir.cpp` và `suites/ir.py`.
- Nghe lén SPI (`SNIFF <sck> <mosi> <cs>`): bắt nhiều khung CS liền nhau (SPI slave chỉ nhận 1 giao dịch nạp sẵn). Đặt cả 3 chân trên GPIO < 32 của ESP32 (VD CS = PC2), SCK ≤ ~750kHz. Kết quả kèm `gap=<ns>`: lớn hơn nửa chu kỳ SCK là ESP32 bị dừng tạm (core 0 dừng core 1 bằng ngắt mức cao, không khóa được) → làm lại thao tác. Dùng chung: `test/runner/spi_sniff.py` + `test/ch32v003/include/sniff.h` (macro `SNIFFED`).
- SPI slave của ESP32 (`SPIS`): mỗi giao dịch (CS xuống-lên) phải `SPIS LOAD` trước. SPI1 phần cứng chỉ có trên F4P6 (PC5/PC6/PC7, CS PC0); SPI mềm test trên PC4/PA2/PA1, CS PC1 (có trên cả 3 chip).
- Đo thời gian bằng `CAP`: sườn đầu tiên bị phát hiện trễ vài µs → đo khoảng ≥ 50µs, hoặc dùng `CAP ... NS` + trung vị nhiều chu kỳ.
- Các net dùng chung cho cả 3 chip: chip nào còn chạy firmware test (runner bị dừng giữa chừng) sẽ lái net và làm sai phép đo. Runner đỗ cả 3 chip lúc bắt đầu; đừng giết runner bằng `kill -9`.
- Firmware test tắt/bật lại SWD (VD: ngủ với `POWER_SLEEP_DISABLE_SWD`): chờ ~50ms trước khi in `HIL:` để dòng không bị rác SWIO làm mất.
- Đo dòng tiêu thụ: máy đo Arduino Uno ở chân GND của J4M6 (`test/uno_meter/`, `WIRING.md` mục 6), runner `meter.py` tự dò cổng. Đo tay: `test/runner/standby_hold.py --chip J4M6 --mode standby|sleep`. Chip ngủ vĩnh viễn với SWD tắt → VDD phải đi qua GPIO ESP32 thì mới nạp lại được (`RST`); nếu không: `rvprog -e` + cấp nguồn tay.
- Firmware test in qua SWIO thì KHÔNG đặt PD1 về `ANALOG` (mất `printf`, dù vẫn nạp được).
- Firmware test trên CH32 báo kết quả qua SWIO bằng dòng `HIL:...` (xem `test/ch32v003/include/hil.h`). Phép đo thời gian phải so theo **clock thật của từng chip** (HSI lệch tới ±2%): suite `timing` lấy `delayMs` làm thước đo.
- Thêm suite mới: tạo `src/<suite>/main.c`, 3 env `<suite>_<chip>` trong `test/ch32v003/platformio.ini`, và `test/runner/suites/<suite>.py` có `on_wait(step, bench, ctx)`.

## Git / phát hành

### Quy trình BẮT BUỘC khi xong một thư viện

"Xong" = code đúng template + README cập nhật (Quick Start qua `check_readme.py`) + test (host/HIL) ĐẠT trên cả 3 chip + `TASKS.md` đã ghi. Ngay khi xong, làm **lập tức, không chờ hỏi**, theo thứ tự:

**Chỉ phát hành khi có thay đổi ảnh hưởng người dùng** (API, hành vi, sửa lỗi, hiệu năng, ví dụ/README có nội dung mới). Thay đổi chỉ là định dạng, chú thích, `.gitignore`... thì **chỉ commit + push + trỏ submodule, không bump version, không tag, không Release**.

1. **Trong submodule `ch32v003_<lib>`:**
   - Bump `version` trong `library.json` theo SemVer (`embedded-library-release-core.md`): đổi/xóa API hoặc type → MAJOR; thêm API → MINOR; chỉ sửa lỗi/tối ưu → PATCH.
   - Commit (`feat(<lib>): ...` / `fix(<lib>): ...` / `refactor(<lib>): ...`, tiếng Việt), tag `v<version>`, push nhánh + **đúng tag đó** (`git push origin HEAD:main v<version>`, không dùng `--tags`).
   - Tạo GitHub Release: `gh release create v<version> --repo voc-vach-iot-ch32v003/ch32v003_<lib> --title "v<version> - <mô tả>" --notes-file <file>`, nội dung theo `embedded-library-release-core.md` (Breaking Changes có code cũ vs mới, New Features, Improvements, Bug Fixes, ngày `YYYY-MM-DD`).
2. **Ở repo gốc:** `git add ch32v003_<lib>` (trỏ submodule tới commit mới) cùng các file test/tài liệu liên quan (`test/...`, `TASKS.md`, `CLAUDE.md`), commit `chore(<lib>): cập nhật submodule lên v<version>`, push.
3. Báo lại người dùng: version, link release, kết quả test.

- **Tài khoản:** git push/tag đi qua SSH alias `git@vocvachiot:` (đã xác thực là `voc-vach-iot`). `gh` có 2 tài khoản: mặc định **`Thieu-Van-Hieu`**; **chỉ khi tạo Release / thao tác GitHub API của tổ chức** mới `gh auth switch -u voc-vach-iot`, làm xong **phải đổi về ngay** `gh auth switch -u Thieu-Van-Hieu` (kể cả khi lỗi giữa chừng). Không thấy tài khoản `voc-vach-iot` trong `gh auth status` thì dừng lại hỏi người dùng.
- Thư viện phụ thuộc thư viện khác: release thư viện tầng dưới trước, tầng trên sau.
- Commit message theo kiểu sẵn có: `feat(gpio): ...`, `fix(i2c): ...`, `refactor: ...` (tiếng Việt).
- `test/HIL_SPEC.md` là tài liệu cục bộ (đã gitignore), không đưa lên repo.

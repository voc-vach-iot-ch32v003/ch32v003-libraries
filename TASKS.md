# TASKS — Chuẩn hóa thư viện CH32V003 theo template

Quy ước trạng thái: ✅ xong (đã build 3 chip J4M6/A4M6/F4P6) · 🔧 đang làm · ⬜ chưa làm · 🧪 cần chạy HIL trên phần cứng.

> Lưu ý chung: chưa có thay đổi nào được commit. Mọi mục ✅ mới chỉ được **build thử**, chưa chạy trên phần cứng.

## Thứ tự xử lý (thư viện ít phụ thuộc trước)

| Tầng | Thư viện | Phụ thuộc |
| :--: | :------- | :-------- |
| 0 | `delay`, `debug`, `intmath`, `gpio`, `timer` | (không) |
| 1 | `i2c` | gpio, debug |
| 1 | `spi` | gpio |
| 1 | `power` | gpio, delay |
| 1 | `ir` | gpio, timer, debug |
| 2 | `max7219` | spi |
| 2 | `bmi160` | i2c, intmath, delay, debug |
| 3 | `led_matrix` | max7219 |

## Việc chung cho MỌI thư viện (checklist template)

- [ ] `library.json`: `dependencies` theo dạng object `{ "name", "version": "<git url>" }` (hiện i2c, spi, max7219, led_matrix, bmi160, ir, power đang dùng mảng chuỗi URL).
- [ ] `README.md` khớp API mới (8 mục theo `embedded-library-docs-core.md`).
- [ ] Examples: chỉ driver/ngoại vi mới cần (đề xuất danh sách → chờ duyệt → tạo).
- [ ] Bộ test HIL trong `test/ch32v003/src/<lib>/` + kịch bản ESP32.
- [ ] Release (bump version trong `library.json`, tag, release note theo `embedded-library-release-core.md`).

---

## Đã phát hành

| Thư viện | Phiên bản | Ngày | Ghi chú |
| :-- | :-- | :-- | :-- |
| `ch32v003_delay` | v1.0.1 | 2026-10-22 | chuẩn hóa tài liệu |
| `ch32v003_debug` | v1.0.1 | 2026-10-22 | chuẩn hóa tài liệu |
| `ch32v003_intmath` | v1.0.1 | 2026-10-22 | sửa atan2/magnitude, cấu trúc `src/` |
| `ch32v003_timer` | v1.1.0 | 2026-10-22 | sửa tràn 89s, `timerInit()`, `micros()` nhanh hơn |
| `ch32v003_gpio` | v2.0.0 | 2026-10-22 | ⚠️ đổi tên type `GPIO_*_t`, `MCU_PIN_NONE` |

## Tầng 0

### `ch32v003_delay`
- ✅ Sửa thụt lề khối Doxygen header. README đúng docs-core, Quick Start biên dịch được (`check_readme.py`).
- ✅ HIL `timing` (3 chip): `delayMs(20)` / `delayUs(200)` khớp clock chip (HSI lệch -1.0% … +0.25% tùy con).

### `ch32v003_debug`
- ✅ Sửa thụt lề khối Doxygen header. README đúng docs-core, Quick Start biên dịch được.
- ✅ Gián tiếp kiểm chứng: mọi test HIL báo kết quả qua `printf` SWIO.

### `ch32v003_intmath`
- ✅ **Bug:** `intMathAtan2Centi` sai 100–150° khi x < 0; mất chính xác (~10°) với đầu vào nhỏ. → Lật về nửa phải + bù ±180°, chuẩn hóa độ lớn, bảng góc milli-độ 14 bước. Sai số còn ≤ 0.02° (test host).
- ✅ **Bug:** `intMathMagnitude2D` doc ghi sai số 3.9% nhưng thực tế −12.5%. → `max(hi, 7/8·hi + 1/2·lo)`, sai số −3% .. +1.2%.
- ✅ `_Generic` bỏ nhánh `int:` (trùng `int32_t` trên một số nền tảng → lỗi biên dịch).
- ✅ Bỏ khối Doxygen header ở file `.c` (đúng template).
- ✅ Test host: `test/host/run.sh` → ĐẠT.
- ✅ HIL `intmath` (3 chip): bản RV32EC trùng khớp bản máy tính; chi phí atan2 817 / magnitude 81 / sqrt 308 chu kỳ.
- ✅ README viết lại theo docs-core (sửa bảng hỏng, sai số, hiệu năng đo thật).

### `ch32v003_gpio`
- ✅ ⚠️ **Breaking:** đổi tên type theo chuẩn `<MODULE>_<PascalCase>_t`: `PinMode_t → GPIO_PinMode_t`, `DigitalState_t → GPIO_DigitalState_t`, `SwioMode_t → GPIO_SwioMode_t`. (Giữ tên hàm kiểu Arduino `pinMode/digitalWrite...` — ngoại lệ có chủ đích.)
- ✅ Thêm `MCU_PIN_NONE` (chuyển từ spi sang gpio để dùng chung).
- ✅ Đặt tên macro cho bit AFIO (`GPIO_AFIO_PA12_REMAP`, `GPIO_AFIO_SWD_DISABLE`), `digitalReadPort` trả về `GPIO_DigitalState_t`, rút gọn `digitalRead`.
- ✅ Sửa doc `@ref MCUPin_t`, chú thích `CHÂNMCU`.
- ✅ Example 01, 02 thiếu `platformio.ini` + `.gitignore` → đã thêm. README đã đổi tên type.
- ✅ HIL `gpio` (2026-10-22): F4P6 142/142, A4M6 106/106, J4M6 44/44 — xuất/đọc/toggle, pull-up/down, cách ly giữa các net trên mọi chân của bàn test. (PC0 có trở kéo xuống của nút trên board TWEN → bỏ qua test pull-up PC0.)

### `ch32v003_timer`
- ✅ **Bug:** `millis()`/`micros()` chia thẳng `SysTick->CNT` 32-bit → quay về 0 sau ~89 giây (48MHz), `elapsedMs` sai, timeout > 89s không bao giờ chạy. → Ngắt so sánh SysTick 1ms tăng bộ đếm 32-bit (tràn sau 49.7 ngày), tự khởi động ở lần gọi đầu, thêm `timerInit()`. `delayMs/Us` không bị ảnh hưởng (CNT vẫn chạy tự do).
- ✅ Bỏ `@version/@date` khỏi header.
- ✅ **Tối ưu:** `micros()` bỏ nhân/chia phần mềm (`ms*1000` = dịch bit, `/48` = dịch bit + chia 3 bằng dịch-cộng, kiểm chứng vét cạn 4 triệu giá trị) → ~180 chu kỳ/lần (trước ~480), đoạn chờ 500us vượt +4..6us (trước +10us).
- ✅ README viết lại: `timerInit()`, chiếm `SysTick_Handler`, giới hạn tràn số.
- ✅ HIL `timing` (3 chip, 7/7): millis/micros khớp clock chip, đơn điệu, micros ≈ millis·1000.
- ✅ ESP32 thêm lệnh `INTERVAL <ch> [timeoutMs]`: đo khoảng giữa 2 sườn lên bằng ngắt (đo được tới vài phút).
- ✅ HIL dài `timer_wrap` (3 chip, 8/8): chạy 95s qua mốc tràn SysTick 89.48s — millis/micros không giảm lần nào, millis khớp số tick SysTick tới 1ms, ESP32 đo khoảng 95s khớp clock chip trong 0.1% (phần lệch là HSI trôi).
- ✅ Mẫu đối chứng: cùng bài test chạy với `ch32v003_timer` bản cũ → báo 5 LỖI đúng tại mốc tràn (đoạn chờ 95s kết thúc sau 18s, millis giảm 1 lần) ⇒ bài test bắt được lỗi gốc.

## Tầng 1

### `ch32v003_i2c`
- ✅ **Bug nghiêm trọng:** NACK không xóa cờ AF và không phát STOP → bus kẹt, mọi giao dịch sau đều lỗi (VD: `bmi160Init` khi không có cảm biến). → `i2cHardwareAbort()`.
- ✅ **Bug:** 400kHz không bật bit F/S (chạy timing Standard ở tốc độ Fast). → Fast Mode CCR = F/3f | FS.
- ✅ **Bug:** `I2C_DEBUG_LOG_ENABLE` nằm trong `sys_config.h` của app nhưng thư viện không include → log không bao giờ bật. → `_defs.h` tự include `sys_config.h` qua `__has_include`, mặc định 0.
- ✅ Reset khối I2C1 + tắt PE trước khi cấu hình; đọc 1 byte: tắt ACK trước khi xóa ADDR.
- ✅ I2C mềm: lấy mẫu ACK 1 lần (bỏ vòng chờ 30000 lần khi NACK).
- ✅ Thêm `i2cWriteBurst()`. Dọn rác sau `#endif` trong `.h`.
- ⬜ README + library.json.
- 🧪 HIL: ESP32 đóng vai I2C slave (ghi/đọc thanh ghi, NACK địa chỉ sai, 100k/400k, HW + SW).

### `ch32v003_spi`
- ✅ **Bug:** F4P6 không biên dịch được (`NSS_PC0` không tồn tại).
- ✅ ⚠️ **Breaking:** chuyển sang Handle `SPI_DriverHandle_t` (giống i2c): `spiInit(&handle)`, `spiTransfer(&handle, b)`, thêm `spiWrite()`, `spiWaitIdle(&handle)`. Bỏ enum remap thừa (NSS luôn quản lý mềm), thêm `SPI_ClockDivider_t`. Đổi `SPI_SoftwareConfig_t → SPI_SoftwareConfiguration_t`.
- ✅ **Tối ưu:** bit-bang cache Port + mặt nạ bit lúc init → mỗi bit chỉ ghi BSHR/BCR thay vì giải mã chân qua `digitalWrite`.
- ⬜ README + library.json.
- 🧪 HIL: ESP32 SPI slave nhận byte, so sánh; đo tần số SCK; HW (F4P6) + SW.

### `ch32v003_power`
- ⬜ **Bug (hiệu năng):** `calcAwuTiming` quét 882 tổ hợp nhân/chia mềm (RV32EC không có MUL/DIV) và bị gọi SAU khi hạ clock xuống HSI/256 (~94kHz) → có thể tốn vài giây. → Tính giải tích 14 vòng, tính sẵn ở `powerConfigureWakeup()`.
- ⬜ **Bug:** `LSI_FREQ_HZ 107092` là số đo của 1 con chip cụ thể → macro cấu hình `POWER_LSI_FREQUENCY_HZ` (mặc định 128000).
- ⬜ **Bug:** thức dậy luôn xóa bit SWD → ghi đè `setSwioMode(SWIO_MODE_GPIO)` của người dùng. → Lưu/khôi phục SWCFG.
- ⬜ `EXTI7_0_IRQHandler` chiếm cứng → `weak`.
- ⬜ ⚠️ **Breaking:** đổi tên theo template: `WakeupTrigger_t → POWER_WakeupTrigger_t`, `PowerWakeupConfig_t → POWER_WakeupConfiguration_t`, `PowerWakeupPin_t → POWER_WakeupPin_t` (dùng `mcuPin` thay `void* GPIOx`), `sleepUltraLowPower → powerSleep`, `enterStandbyMode → powerStandby`.
- ⬜ README + library.json + 2 example hiện có.
- 🧪 HIL: ESP32 đo thời gian ngủ/đánh thức qua chân, kích cạnh đánh thức.

### `ch32v003_ir`
- ⬜ **Bug:** `rawLen` kiểu `uint8_t` nhưng buffer 256 phần tử → tràn về 0.
- ⬜ **Bug:** đếm bit `(rawLen-2)/2` sai với giao thức không có stopMark → Sony không bao giờ giải mã được.
- ⬜ **Bug:** `command = rawData[2] | rawData[3] << 16` (phải `<< 8`).
- ⬜ **Bug:** `irSend` Sony đặt address vào bit thấp → chỉ phát address (12 bit = 7 command + 5 address).
- ⬜ **Bug:** `irSend` TCL (76 bit) không xóa `tmp.rawData[6..9]` → phát rác từ stack.
- ⬜ **Bug:** thiếu `IR_SUPPORT_*` trong `sys_config.h` → không giao thức nào được bật. → mặc định 1.
- ⬜ NEC: encode/decode đối xứng (address/command 8-bit kèm byte đảo, hoặc 16-bit extended).
- ⬜ **Tối ưu:** sóng mang tính theo mốc SysTick (không trôi), `IR_RAW_BUFFER_SIZE` mặc định 160 (đủ 76 bit TCL, tiết kiệm RAM — `irRead` cấp buffer trên stack 2KB RAM).
- ⬜ Công khai `irDecodeRaw()` / `irSendData()`, `printBin8` → static.
- 🧪 HIL: ESP32 phát khung NEC/Sony/Samsung đã giải điều chế → CH32 giải mã; CH32 phát → ESP32 bắt sườn, đo tần số sóng mang.

## Tầng 2

### `ch32v003_max7219`
- ✅ ⚠️ **Breaking:** `Max7219_t → MAX7219_DriverHandle_t { spiHandle, csPin, cascadeCount }`, `max7219Init(&dev)` trả `uint8_t`, `max7219WriteReg → max7219WriteRegister`.
- ✅ Thêm `max7219WriteChain()` (mỗi IC 1 dữ liệu riêng trong 1 khung CS).
- ✅ Bật hiển thị SAU khi xóa (không chớp rác lúc khởi động). Bỏ include `sys_config.h` thừa.
- ⬜ README + library.json.
- 🧪 HIL: ESP32 SPI slave bắt khung 16-bit × N, kiểm tra thứ tự Daisy Chain.

### `ch32v003_bmi160`
- ⬜ **Bug:** `BMI160_GYRO_RANGE_DEFAULT = 0` trùng `BMI160_GYRO_RANGE_2000_DPS = 0x00` → chọn 2000 dps thực tế chạy 250 dps (sand-glass đang dính).
- ⬜ **Bug:** chờ gyro khởi động 60ms < 80ms theo datasheet.
- ⬜ **Bug:** `bmi160GetNormalizedGravity2D` dùng `static` filter → 2 cảm biến dùng chung trạng thái. → Đưa vào struct.
- ⬜ `printf` vô điều kiện trong `bmi160CalibrateZeroOffset` → `BMI160_DEBUG_LOG_ENABLE` (mặc định 0, tiết kiệm Flash).
- ⬜ Gom code trùng: dùng `intMathMagnitude2D`, `i2cWriteBurst`/`i2cReadBurst` thay start/write/stop tay.
- ⬜ README + library.json + examples.
- 🧪 HIL: ESP32 giả lập BMI160 (chip ID 0xD1, thanh ghi dữ liệu) qua I2C slave.

## Tầng 3

### `ch32v003_led_matrix`
- ✅ **Tối ưu:** `ledMatrixUpdate` gửi N² khung → 8 khung (1 khung/hàng cho cả chuỗi).
- ✅ **Bug:** xoay 90/270 với > 1 module ghi tràn Frame Buffer. → Xoay từng module tại chỗ; 180 vẫn lật cả dải.
- ✅ **Bug:** trạng thái cuộn chữ là biến `static` → 2 màn hình dùng chung. → Đưa vào Handle; cài đặt cuộn UP/DOWN (trước đây khai báo nhưng không chạy).
- ✅ **Bug font:** `n`, `[`, `]`, `~` sai glyph.
- ✅ ⚠️ **Breaking:** `LedMatrix_t → LED_MATRIX_DriverHandle_t`, `MatrixRotation_t → LED_MATRIX_Rotation_t` (`LED_MATRIX_ROTATE_*`), `MatrixScrollDirection_t → LED_MATRIX_ScrollDirection_t` (`LED_MATRIX_SCROLL_*`), `ledMatrixInit(dev, spiHandle, csPin, buffer, count)`.
- ⬜ README + library.json.
- 🧪 HIL: bắt khung SPI, đối chiếu Frame Buffer.

---

## Hạ tầng test HIL

- ✅ Chuyển spec → `test/HIL_SPEC.md` (gitignore).
- ✅ Khai báo đủ 12 submodule trong `.gitmodules` (thiếu bmi160, i2c, intmath).
- ✅ `test/ch32v003/` project build mọi thư viện × 3 chip (env `smoke_*`).
- ✅ `test/host/` test logic thuần trên máy tính (intmath).
- ✅ `test/WIRING.md` + `test/wiring/gen.py` — bản vẽ ESP32 30 chân (2 mini) ↔ F4P6 (TWEN32F003) ↔ A4M6/J4M6. Cả 3 chip luôn có nguồn từ GPIO (GPIO2 → F4P6, GPIO22 → A4M6, GPIO23 → J4M6; ESP32 USB hướng lên, A4M6 xoay 180°), SWIO nối chung, chip không test chạy firmware đỗ.
- ✅ Firmware đỗ `test/ch32v003/src/park/` (env `park_J4M6/A4M6/F4P6`, 1148 byte Flash): mọi chân Hi-Z, `delayMs(1000)` (cửa sổ nạp), in `HIL:PARKED <chip>`, `setSwioMode(SWIO_MODE_GPIO)`, ngủ `WFI`. 🧪 Chưa nạp thử trên phần cứng.
- ⬜ `CLAUDE.md` — quy trình làm việc, quy chuẩn template, cách chạy test, quy tắc bắt buộc `delayMs(1000)` đầu `main()`.
- 🔧 Firmware ESP32 `test/esp32/`: ✅ phần nguồn (`PING`, `PWR <chip|ALL> <ON|OFF>`, `RST <chip> [ms]`, `STATUS`; khởi động thì bật cả 3 chip). ✅ Lệnh 16 kênh (`Z`, `MODE`, `W`/`ON`/`OFF`, `R`, `RA`, `ADC`, `DAC`, `CAP`, `FREQ`). ⬜ `I2CS`, `SPIS`, `UART` (làm cùng thư viện tương ứng).
- ✅ `test/runner/bench_check.py`: kiểm tra bàn test — ĐẠT trên phần cứng (2026-10-22): cả 3 chip đỗ thì không ai trả lời; khởi động lại từng chip → nạp đúng chip trong cửa sổ 1 giây (F4P6 `0x0030`, A4M6 `0x0032`, J4M6 `0x0033`).
- ✅ Runner `test/runner/hil.py <suite>`: Hi-Z → RST chip → nạp + mở terminal SWIO trong cửa sổ 1s (kiểm tra Part Type để chặn nạp nhầm chip) → xử lý `HIL:WAIT/CHECK/END` → nạp lại firmware đỗ → báo cáo JSON `test/runner/reports/`.
- ✅ `test/runner/check_readme.py <lib...>`: biên dịch Quick Start của README trên 3 chip.

## Ảnh hưởng tới dự án đang dùng `lib_extra_dirs`

Các thay đổi ⚠️ Breaking (gpio type, spi, max7219, led_matrix, sắp tới power) làm `iot/ch32v003/sand-glass`, `led-matrix-max7219`... không build được cho tới khi sửa theo API mới.

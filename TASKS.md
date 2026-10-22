# TASKS — Chuẩn hóa thư viện CH32V003 theo template

Quy ước trạng thái: ✅ xong (đã build 3 chip J4M6/A4M6/F4P6) · 🔧 đang làm · ⬜ chưa làm · 🧪 cần chạy HIL trên phần cứng.

> Thư viện đã phát hành thì đã chạy HIL trên phần cứng. Các mục ✅ của thư viện chưa phát hành mới chỉ được **build thử**.

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
| `ch32v003_intmath` | v1.0.1 | 2026-10-22 | sửa atan2/magnitude, cấu trúc `src/` |
| `ch32v003_timer` | v1.1.0 | 2026-10-22 | sửa tràn 89s, `timerInit()`, `micros()` nhanh hơn |
| `ch32v003_gpio` | v2.0.0 | 2026-10-22 | ⚠️ đổi tên type `GPIO_*_t`, `MCU_PIN_NONE` |
| `ch32v003_i2c` | v1.1.0 | 2026-10-22 | không kẹt bus khi NACK, Fast Mode, `i2cWriteBurst`, SW ~100kHz |
| `ch32v003_gpio` | v2.0.1 | 2026-10-22 | ví dụ 03 toggle sai chân trên A4M6/F4P6, bảng chân theo gói vỏ |
| `ch32v003_spi` | v2.0.0 | 2026-10-22 | ⚠️ Handle `SPI_DriverHandle_t`, `spiWrite` liền mạch, sửa build F4P6 |
| `ch32v003_power` | v2.0.0 | 2026-10-22 | ⚠️ `powerSleep`/`powerStandby`, `POWER_*_t` (mcuPin), sửa hẹn giờ AWU + SWIO, chạy cả J4M6 |
| `ch32v003_power` | v2.0.1 | 2026-10-22 | tài liệu: dòng đo thực tế (Standby 6.7µA, Sleep 371µA), chân không dùng về `ANALOG`, nạp lại chip đang ngủ |
| `ch32v003_power` | v2.0.2 | 2026-10-22 | ví dụ 02 mất printf (PD1 ANALOG), chi phí hẹn giờ +1.2µA |
| `ch32v003_power` | v2.0.3 | 2026-10-22 | kéo lên SWIO trong lúc ngủ: chân bỏ trống không còn đánh thức chip liên tục |

`delay`, `debug` giữ v1.0.0: thay đổi chỉ là định dạng Doxygen + `.gitignore` → đã commit, không phát hành.
`timer`: sửa chú thích `MCU_PIN3` trong README (PA2 chỉ đúng với J4M6; A4M6 là PC3, F4P6 là PD6) → commit, không phát hành.

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
- ✅ (v2.0.1) Đối chiếu `ch32v003_gpio_pins.h` với pinout ch32fun (`.tmp/*.png`) + bảng remap RM: số chân, pad gộp J4M6, chức năng đều khớp. Thêm ghi chú chức năng nằm sẵn trên chân gộp J4M6 (chân 1 = PA1+PD6: URX/A6; chân 8 = SWIO+PD5+PD4: UTX/UCK/A5/A7/OPO) — không cần remap.
- ✅ (v2.0.1) **Bug ví dụ 03:** `pinMode(MCU_PIN6)` nhưng toggle PC2 → chỉ đúng trên J4M6. Ví dụ 01/02 ghi "PA2/PC1 trên mọi gói vỏ" → bảng chân theo gói vỏ.
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
- ✅ **Bug:** `i2cStop()` gọi thừa sau `i2cRead(0)` (STOP đã tự phát) đặt lại bit STOP khi không còn là Master. → Chỉ phát STOP khi còn MSL.
- ✅ **Tối ưu:** I2C phần mềm 37kHz → ~100kHz, macro `I2C_SOFTWARE_DELAY_LOOPS` (mặc định 5).
- ✅ README viết lại theo docs-core (Quick Start qua `check_readme.py`), library.json dependencies dạng object, mã nguồn vào `src/`.
- ✅ HIL `i2c` (2026-10-22, 48/48 × 3 chip): ESP32 làm slave 0x42; HW 100k / HW 400k / SW: ghi burst, đọc burst 1/2/4 byte, API rời + `i2cStop` thừa, NACK rồi giao dịch tiếp, SCL đo được 97.7k / 381k / 95–98kHz.
- ✅ Mẫu đối chứng: tắt bản sửa NACK → bus kẹt, hàng loạt LỖI ⇒ bài test bắt được lỗi gốc.
- ⬜ Chưa test Remap 1/2 (F4P6): PD0/PD1 trùng SWIO, PC5/PC6 không có trở kéo lên trên bàn test.

### `ch32v003_spi`
- ✅ **Bug:** F4P6 không biên dịch được (`NSS_PC0` không tồn tại).
- ✅ ⚠️ **Breaking:** chuyển sang Handle `SPI_DriverHandle_t` (giống i2c): `spiInit(&handle)`, `spiTransfer(&handle, b)`, thêm `spiWrite()`, `spiWaitIdle(&handle)`. Bỏ enum remap thừa (NSS luôn quản lý mềm), thêm `SPI_ClockDivider_t`. Đổi `SPI_SoftwareConfig_t → SPI_SoftwareConfiguration_t`.
- ✅ **Tối ưu:** bit-bang cache Port + mặt nạ bit lúc init → mỗi bit chỉ ghi BSHR/BCR thay vì giải mã chân qua `digitalWrite` (~1MHz).
- ✅ **Tối ưu:** `spiWrite` phần cứng nạp byte khi TXE trống (không chờ RXNE từng byte) → SCK liền mạch, cuối cùng chờ BSY + xóa RXNE/OVR.
- ✅ README viết lại theo docs-core (Quick Start qua `check_readme.py`), library.json dependencies dạng object, mã nguồn vào `src/`.
- ✅ HIL `spi` (2026-10-22): ESP32 SPI slave Mode 0. F4P6 24/24 (HW 3MHz, HW 187.5kHz, SW), A4M6 9/9, J4M6 9/9 (SW + từ chối HW): song công 8 byte, `spiWrite` 12 byte, `spiTransfer` sau `spiWrite`, tốc độ qua độ dài CS (32 byte @3MHz = 84µs, đúng lý thuyết).
- ✅ Mẫu đối chứng: bỏ bước xóa RXNE/OVR sau `spiWrite` → 3 LỖI (đọc nhầm byte cũ) ⇒ bài test bắt được lỗi.

### `ch32v003_power`
- ✅ **Bug (hiệu năng):** `calcAwuTiming` quét 882 tổ hợp nhân/chia mềm ở clock ~94kHz → tính giải tích 14 vòng, tính sẵn ở `powerConfigureWakeup()`.
- ✅ **Bug:** `LSI_FREQ_HZ 107092` (số đo 1 chip) → macro `POWER_LSI_FREQUENCY_HZ` (mặc định 128000). Bàn test đo được LSI 109 / 112 / 123kHz.
- ✅ **Bug:** thức dậy luôn bật lại SWD → lưu/khôi phục bit SWD (`getSwioMode` đọc lại được PCFR1, đã kiểm chứng).
- ✅ **Bug:** Standby để lại SLEEPDEEP/PDDS → tắt sau khi thức. Khôi phục clock theo trạng thái cũ (PLL nếu có).
- ✅ Bỏ chiếm `EXTI7_0_IRQHandler`: đánh thức bằng sự kiện EXTI (WFE), phân biệt chân/hẹn giờ qua cờ INTFR (không bật NVIC).
- ✅ ⚠️ **Breaking:** `powerSleep()` / `powerStandby()` (trả `uint8_t`), `POWER_WakeupTrigger_t`, `POWER_WakeupPin_t { mcuPin, trigger }`, `POWER_WakeupConfiguration_t`. Bỏ phụ thuộc `ch32v003_delay`.
- ✅ **Sai tài liệu:** Standby của CH32V003 chạy tiếp + giữ RAM (không reset), chân đánh thức bất kỳ, chạy cả J4M6 (đối chiếu ví dụ `standby_btn` của ch32fun + đo HIL).
- ✅ Ví dụ viết lại: `01_sleep_button_wakeup`, `02_standby_button_timer` (build 3 gói vỏ với thư viện local). README viết lại (Quick Start qua `check_readme.py`).
- ✅ HIL `power` (2026-10-22, 53/53 × 3 chip): Sleep + Standby × (từ chối khi không có nguồn, 3 kiểu sườn + không thức sớm, 2 chân, hẹn giờ 2000/5200ms + công thức `(W+1)·P` khớp 0.1%, chân cắt ngang hẹn giờ 20s, RAM giữ), hẹn giờ 32s nhiều lần AWU, SWIO GPIO giữ sau khi thức.
- ✅ Mẫu đối chứng: khôi phục hành vi cũ (luôn bật lại SWD) → 2 LỖI ⇒ bài test bắt được lỗi.
- ✅ Bằng chứng Standby thật (không cần đo dòng): nhánh Standby không tắt PLL, nhưng sau khi thức cờ `PLLRDYF` = 1 → phần cứng đã tắt PLL. 70/70 × 3 chip (J4M6 cũng vào Standby thật). Đối chứng âm: bỏ PDDS/SLEEPDEEP → `PLLRDYF` = 0, bài test báo LỖI. (`HSIRDYF` không bật khi phần cứng tự khởi động lại HSI → không dùng được.)
- ✅ Đo tay dòng J4M6 (shunt ở chân GND + ANENG M118A thang DCV, `test/runner/standby_hold.py --mode standby|sleep`): Standby chân thả nổi ≥ 50µA (0.5V / 9.9kΩ), Standby chân `ANALOG` **6.7µA** (0.066V / 9.9kΩ), Sleep **371µA** (0.119V / 321Ω). → v2.0.1: README + doc header ghi số đo, khuyến cáo chân không dùng về `ANALOG`, cách nạp lại chip đang ngủ.
- ✅ Thí nghiệm `test/runner/swio_probe.py` (J4M6): PD1, PD4+PD5, mọi chân về `ANALOG` → SWIO vẫn nạp được. Lần "khóa nạp" trước là do chip ngủ vĩnh viễn (SWD tắt) mà VDD không đi qua GPIO ESP32 nên không tắt/bật được nguồn; cứu bằng `rvprog -e` + cấp nguồn tay.
- ✅ Máy đo dòng tự động bằng Arduino Uno (`test/uno_meter/`, `test/runner/meter.py`, `WIRING.md` mục 6): R2 22Ω (thang mA) + R1 10kΩ nối tắt bằng 8050 (thang µA), tham chiếu nội 1.1V, tự đổi thang. Suite `current` (J4M6): chạy 5.4mA, Standby thức bằng chân 8.1µA, thức bằng hẹn giờ 9.35µA (LSI + AWU +1.2µA), Sleep 390µA. Lặp lại ổn định giữa các lần chạy.
- ✅ PD1 ở `ANALOG` làm mất `printf` qua SWIO đang chạy (vẫn nạp được) → v2.0.2: ví dụ 02 + README giữ PD1.
- ✅ **Bug (v2.0.3):** chip đặt riêng (không cắm mạch nạp) với chân SWIO thả nổi → Standby bị đánh thức liên tục (xung mỗi ~15ms, 11.5µA). → Trong lúc ngủ thư viện bật Pull-up nội cho PD1 nếu PD1 đang là SWIO (bỏ qua nếu đã dùng làm GPIO, hoặc J4M6 đang lái PD4/PD5 output trên chân 8). Đo lại: 10.12µA ổn định không cần trở ngoài. HIL `power` 70/70 × 3 chip.
- ⬜ Đo lại ngoài bàn ở 3.3V cho `run` / `sleep` / `standby` (lần đo giai đoạn 1–2 là ở 5V: run 4.90mA, sleep 416µA).
- ⬜ Máy đo dòng bằng CH32V003 (A4M6) thay Uno: tụ 104 + OPA làm bộ so sánh + TIM2 capture (dải µA), trở 22Ω + ADC (dải mA), gửi kết quả UART → ESP32 GPIO35. Cần thư viện OPA + UART trước.

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
- ⬜ **Sai chân trong README:** `IR_TX_PIN MCU_PIN4` = **VDD** trên J4M6, NRST (PD7) trên F4P6.
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
- ⬜ **Sai chân trong README:** `CS_PIN MCU_PIN7` = **VSS** trên F4P6, PD1/SWIO trên A4M6.

---

## Hạ tầng test HIL

- ✅ Chuyển spec → `test/HIL_SPEC.md` (gitignore).
- ✅ Khai báo đủ 12 submodule trong `.gitmodules` (thiếu bmi160, i2c, intmath).
- ✅ `test/ch32v003/` project build mọi thư viện × 3 chip (env `smoke_*`).
- ✅ `test/host/` test logic thuần trên máy tính (intmath).
- ✅ `test/WIRING.md` + `test/wiring/gen.py` — bản vẽ ESP32 30 chân (2 mini) ↔ F4P6 (TWEN32F003) ↔ A4M6/J4M6. Cả 3 chip luôn có nguồn từ GPIO (GPIO2 → F4P6, GPIO22 → A4M6, GPIO23 → J4M6; ESP32 USB hướng lên, A4M6 xoay 180°), SWIO nối chung, chip không test chạy firmware đỗ.
- ✅ Firmware đỗ `test/ch32v003/src/park/` (env `park_J4M6/A4M6/F4P6`, 1148 byte Flash): mọi chân Hi-Z, `delayMs(1000)` (cửa sổ nạp), in `HIL:PARKED <chip>`, `setSwioMode(SWIO_MODE_GPIO)`, ngủ `WFI`. ✅ Chạy trên phần cứng.
- ✅ `CLAUDE.md` — quy trình làm việc, quy chuẩn template, cách chạy test, quy tắc phát hành.
- 🔧 Firmware ESP32 `test/esp32/`: ✅ phần nguồn (`PING`, `PWR <chip|ALL> <ON|OFF>`, `RST <chip> [ms]`, `STATUS`; khởi động thì bật cả 3 chip). ✅ Lệnh 16 kênh (`Z`, `MODE`, `W`/`ON`/`OFF`, `R`, `RA`, `ADC`, `DAC`, `CAP [NS]`, `FREQ`, `INTERVAL`). ✅ `I2CS ON/OFF/LOAD/RX` (I2C slave, nạp sẵn dữ liệu vì ESP32 gốc không kéo giãn SCL). ✅ `SPIS ON/OFF/LOAD/RX` (SPI slave Mode 0 trên 4 kênh bất kỳ, ESP-IDF `spi_slave`, không DMA, ≤ 64 byte/giao dịch).
- ✅ Runner: đỗ cả 3 chip lúc bắt đầu + đỗ lại trong `finally` / SIGTERM (chip còn chạy firmware test lái net dùng chung làm sai phép đo); nhận `HIL:` giữa dòng (rác SWIO sau khi thức); đối chiếu số CHECK chip báo ở `HIL:END` với số dòng nhận được. ⬜ `UART` (làm cùng thư viện tương ứng).
- ✅ `test/runner/bench_check.py`: kiểm tra bàn test — ĐẠT trên phần cứng (2026-10-22): cả 3 chip đỗ thì không ai trả lời; khởi động lại từng chip → nạp đúng chip trong cửa sổ 1 giây (F4P6 `0x0030`, A4M6 `0x0032`, J4M6 `0x0033`).
- ✅ Runner `test/runner/hil.py <suite>`: Hi-Z → RST chip → nạp + mở terminal SWIO trong cửa sổ 1s (kiểm tra Part Type để chặn nạp nhầm chip) → xử lý `HIL:WAIT/CHECK/END` → nạp lại firmware đỗ → báo cáo JSON `test/runner/reports/`.
- ✅ `test/runner/check_readme.py <lib...>`: biên dịch Quick Start của README trên 3 chip.

## Ảnh hưởng tới dự án đang dùng `lib_extra_dirs`

Các thay đổi ⚠️ Breaking (gpio type, spi, max7219, led_matrix, sắp tới power) làm `iot/ch32v003/sand-glass`, `led-matrix-max7219`... không build được cho tới khi sửa theo API mới.

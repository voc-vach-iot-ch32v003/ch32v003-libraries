# Sơ đồ nối dây bàn test HIL

> Bản vẽ chi tiết (toàn cảnh, từng lỗ cắm, đi dây theo bước) được sinh từ `test/wiring/gen.py`.
> Muốn đổi cách nối thì sửa bảng `CH` trong `gen.py` rồi sinh lại, để bản vẽ và tài liệu này luôn khớp nhau:
> `python3 test/wiring/gen.py html test/wiring/template.html > hil-wiring.html`

## 1. Bố trí

| Mini | Nội dung |
| :-- | :-- |
| `EL` | Hàng chân **xa** F4P6 của ESP32 DevKit 30 chân (cột j). ESP32 cắm **USB hướng lên** |
| `ER` | Hàng chân **gần** F4P6 (cột a) |
| `F` | TWEN32F003 (F4P6), hàng 1–11, USB-C hướng lên → hàng 2 của board (G V TX RX A1 A2 C0–C4) nằm bên trái. Mini `F` đặt **thấp hơn mini ESP32 3 hàng** (hàng 1 của F ngang hàng 4 của ER) để 6 dây CH8–CH13 nằm ngang thẳng |
| `C` | Ngay dưới `F`: A4M6 (DIP16) hàng 3–10 **xoay 180°** (chân 1 ở `C:10g`), J4M6 (DIP8) hàng 12–15 chân 1 trên-trái (`C:12d`). Nửa phải hàng 17 là đường 3V3 cho pull-up |

Thứ tự chân ESP32 DevKit 30 chân khi **USB hướng lên**, từ trên xuống:

- Cột xa (EL): 3V3, GND, 15, 2, 4, 16, 17, 5, 18, 19, 21, RX, TX, 22, 23
- Cột gần (ER): VIN, GND, 13, 12, 14, 27, 26, 25, 33, 32, 35, 34, VN, VP, EN

Vì sao xoay A4M6: khi đặt bình thường, 9/13 dây từ F4P6 phải vắt sang phía đối diện; xoay 180° chỉ còn 4 dây (PC1–PC4), và toàn bộ dây A4M6 → J4M6 nằm cùng phía, cùng thứ tự.

Chân header TWEN32F003, đánh số từ phía Type-C:

| # | Hàng 1 | Hàng 2 |
| :-: | :-- | :-- |
| 1 | G | G |
| 2 | V | V |
| 3 | D7 (NRST) | TX (= PD5) |
| 4 | D4 | RX (= PD6) |
| 5 | D3 | A1 |
| 6 | D2 | A2 |
| 7 | D1 (SWIO, LED trên board) | C0 (nút nhấn trên board) |
| 8 | D0 | C1 |
| 9 | C7 | C2 |
| 10 | C6 | C3 |
| 11 | C5 | C4 |

> ⚠️ Đo trước: chân V nào là 3V3. Bản vẽ giả định là V của hàng 2 (`F:2a`). Khi chạy bàn test thì rút USB-C của board.

## 2. 16 kênh tín hiệu

- Chỉ **phía ESP32** có trở 1kΩ (vắt qua rãnh giữa mini, ngay tại hàng của chân).
- F4P6 → A4M6 → J4M6 nối **thẳng**, không có trở.
- "–" nghĩa là gói vỏ đó không có chân này.

| Kênh | ESP32 GPIO | Net | F4P6 (nhãn) | A4M6 (chân DIP16) | J4M6 (chân DIP8) | Vai trò |
| :-: | :-: | :-: | :-: | :-: | :-: | :-- |
| 1 | 13 | PD4 | D4 | 8 | – | GPIO / ADC A7 |
| 2 | 12 | PD3 | D3 | – | – | GPIO / ADC A4 |
| 3 | 15 | PD2 | D2 | – | – | GPIO / ADC A3 |
| 4 | 4 | PD0 | D0 | – | – | GPIO |
| 5 | 16 | PC7 | C7 | 6 | – | SPI MISO |
| 6 | 17 | PC6 | C6 | 5 | – | SPI MOSI |
| 7 | 5 | PC5 | C5 | – | – | SPI SCK |
| 8 | 27 | PD6 | RX | 10 | – | UART – CH32 RX |
| 9 | 14 | PD5 | TX | 9 | – | UART – CH32 TX |
| 10 | 26 | PA1 | A1 | 12 | 1 | DAC2 → ADC A1 |
| 11 | 25 | PA2 | A2 | 13 | 3 | DAC1 → ADC A0 |
| 12 | 33 | PC0 | C0 | 16 | – | SPI CS / GPIO |
| 13 | 32 | PC1 | C1 | 1 | 5 | I2C SDA |
| 14 | 18 | PC2 | C2 | 2 | 6 | I2C SCL |
| 15 | 19 | PC3 | C3 | 3 | – | GPIO / PWM |
| 16 | 21 | PC4 | C4 | 4 | 7 | GPIO / ADC A2 |

## 3. Nguồn, GND, SWIO

| Tín hiệu | Nối |
| :-- | :-- |
| GPIO2 (D2) | VDD F4P6 (chân V 3V3) |
| GPIO22 | VDD A4M6 (chân 15) + tụ 100nF xuống chân 14 |
| GPIO23 | VDD J4M6 (chân 4) + tụ 100nF xuống chân 2 |
| ESP32 3V3 | Nửa phải hàng 17 mini `C` → 2 trở 4.7kΩ pull-up lên PC1, PC2 (tại chân J4M6) |
| ESP32 GND | GND F4P6, A4M6 chân 14, GND WCH-LinkE, GND Arduino Uno. **J4M6 chân 2 đi qua máy đo dòng** (mục 6) |
| WCH-LinkE SWDIO | Nối chung: F4P6 D1, A4M6 chân 7, J4M6 chân 8 |
| WCH-LinkE 3V3/5V | **Không nối** |

**Không dùng GPIO12 để cấp nguồn.** Chân này phải ở LOW lúc ESP32 khởi động; nếu nối vào VDD của chip, pull-up I2C sẽ nuôi ngược qua chip làm chân lên HIGH, ESP32 chọn flash 1.8V và không boot. GPIO12 được dùng làm tín hiệu ở đường không có pull-up (CH2 → D3).

## 4. Nguyên tắc vận hành (vì sao không cần công tắc nguồn/SWIO)

1. Cả 3 chip **luôn có nguồn** (GPIO ở mức HIGH).
2. Chip không test chạy **firmware đỗ**: mọi chân để Hi-Z; sau `delayMs(1000)` thì tắt SWD (`setSwioMode(SWIO_MODE_GPIO)`). Chip đỗ không tải bus và không trả lời trên SWIO.
3. Muốn nạp chip X: đưa 16 kênh ESP32 về Hi-Z, kéo nguồn X xuống LOW khoảng 50ms rồi bật lại. X có 1 giây với SWD bật, và là chip duy nhất trả lời trên SWIO, nên runner nạp X bằng `minichlink`.
4. Firmware test giữ SWD bật để in kết quả qua SWIO. Xong thì nạp lại firmware đỗ.
5. **Mọi firmware** (đỗ, test, ví dụ) **bắt buộc** có `delayMs(1000)` ở đầu `main()`.
6. Lần đầu tiên, nạp firmware đỗ cho từng chip riêng lẻ (mỗi lần chỉ cắm SWIO vào một chip).

Vì sao không ngắt nguồn chip không test: chip bị ngắt VDD (kể cả ngắt luôn GND) vẫn được cấp điện ngược qua diode bảo vệ ở các chân IO. Nếu VDD bị kéo về 0V thì chip lại ghim các đường chỉ có pull-up (I2C) và cả đường SWIO xuống khoảng 0.6V.

## 5. Kiểm tra sau khi lắp

1. Chưa cắm chip: ESP32 boot và nạp firmware bình thường.
2. Đo thông mạch GND giữa ESP32, WCH-LinkE và 3 chip.
3. Nạp firmware đỗ cho từng chip riêng lẻ.
4. Cắm SWIO chung. Tắt/bật từng chip rồi chạy `minichlink -i` ngay sau đó: phải nhận đúng chip vừa khởi động lại.
5. Không nhấn nút C0 trên TWEN32F003 khi đang test.

## 6. Máy đo dòng tiêu thụ (Arduino Uno, chỉ J4M6)

Dòng của J4M6 đi từ chân 2 (VSS) qua 2 trở đo xuống GND chung. Uno (firmware `test/uno_meter/`, cổng `/dev/ttyACM*`, runner tự dò bằng `PING`) đo điện áp bằng tham chiếu nội ~1.1V và tự đổi thang.

```
 J4M6 chân 2 (VSS) ── nút A ────────────────────────────────────────── Uno A0
                         │
                      R2 22Ω
                         │
                      nút X ──┬──────────────┬──────────────┬───────── Uno A1
                              │              │              │
                          R1 10kΩ     Q1 8050: C      Q2 8050 làm diode
                              │       B ─ 1kΩ ─ Uno D2  (B+C ở nút X, E ở GND)
                              │       (10kΩ Uno D2 → Uno 5V)│
                              │              E              │
 GND chung ───────────────────┴──────────────┴──────────────┴───────── Uno GND
```

| Thang | Q1 (D2) | Dòng | Dải |
| :-- | :-- | :-- | :-- |
| µA | tắt (LOW) | V(A1) / R1 | ~0.5 – 40µA |
| mA | dẫn (HIGH, mặc định) | (V(A0) − V(A1)) / R2 | ~40µA – 45mA |

- **E của Q1 phải nối thẳng GND** (không qua trở đo): dòng base ~4mA của Q1 sẽ không bị tính vào dòng chip. Bản đầu đặt R2 dưới Q1 nên đo dư ~3.9mA.
- **R2 nhỏ (22Ω)** để GND của chip chỉ nâng ~0.1V khi chạy / nạp code. R2 = 100Ω làm GND nâng ~0.4–0.8V, nạp code thất bại.
- Mặc định Q1 dẫn; trở 10kΩ kéo D2 lên 5V giữ Q1 dẫn cả lúc Uno đang reset (mở cổng serial). Q2 giữ chip không sập nguồn trong vài µs nếu chip thức lúc đang ở thang µA.
- Hiệu chuẩn 1 lần (lưu EEPROM): `CAL AREF <mV đo ở chân AREF>`, `CAL R1 <Ω>`, `CAL R2 <Ω>`. Bàn hiện tại: AREF 1065mV, R1 9.9kΩ, R2 22.2Ω.
- Kiểm tra nhanh: tắt nguồn cả 3 chip (`PWR ALL OFF`) → máy đo phải chỉ ~0µA. Điện áp A1 ở thang mA phải < 50mV (Q1 bão hòa); nếu ~0.65V là Q1 không dẫn (cắm ngược C/E hoặc hở base).
- Suite `current` (`hil.py current`): đo J4M6 chạy / Standby (thức bằng chân và bằng hẹn giờ) / Sleep.

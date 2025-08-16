# Motion Simon Says (ESP32 + MPU6050 + OLED)

A tilt‑controlled **Simon Says** memory game for **ESP32**, using an **MPU6050** IMU and a **128×64 SSD1306 OLED**. Players watch a sequence of arrows on the OLED, then reproduce the sequence by tilting the board **Up / Down / Left / Right** within a timeout. Levels get longer until you win or miss a move.

> **Author (code):** Ariel Lai
> **This README:** generated to document setup, wiring, usage, and customization.

---

## Features

* I²C peripherals with **auto‑retry** init and **periodic I²C recovery**.
* Clear OLED prompts: start, level intro, arrows, win/lose screens.
* Motion input via **pitch/roll** thresholds (no buttons required for gameplay).
* Configurable **max level**, **move display delay**, and **input timeout**.
* Debounced **Start/Reset** button.

---

## Hardware

* **MCU:** ESP32 (DevKitC or similar)
* **IMU:** MPU6050 (address `0x68`)
* **Display:** 0.96" SSD1306 OLED 128×64 I²C (address `0x3C`)
* **Pushbutton:** momentary, to GND (uses internal pull‑up)
* **Wires** and breadboard or PCB

### Pin Wiring (ESP32 default I²C)

| Peripheral | ESP32 Pin   | Notes                                  |
| ---------- | ----------- | -------------------------------------- |
| SDA (I²C)  | **GPIO 21** | Shared by OLED + MPU6050               |
| SCL (I²C)  | **GPIO 22** | Shared by OLED + MPU6050               |
| Button     | **GPIO 15** | Button to **GND**; uses `INPUT_PULLUP` |
| 3V3        | **3.3V**    | Power for OLED + MPU6050               |
| GND        | **GND**     | Common ground                          |

> If your modules expose **VCC** pins rated 3.3–5V, prefer **3.3V** to match ESP32 logic. Keep I²C wires short and twisted/paired for stability.

---

## Software & Libraries

* **Arduino IDE** (or PlatformIO) with **ESP32 Board Package**
* Libraries (from Library Manager):

  * `Adafruit_MPU6050`
  * `Adafruit_Sensor` (dependency)
  * `Adafruit_SSD1306`
  * `Adafruit_GFX` (dependency)

---

## Project Configuration (in code)

```cpp
// I2C
#define I2C_SDA 21
#define I2C_SCL 22
#define I2C_SPEED 100000

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C

// MPU6050
#define MPU_ADDRESS 0x68

// Button
#define BUTTON_PIN 15

// Game tuning
#define MAX_LEVEL 10
#define MOVE_DELAY 800      // ms
#define INPUT_TIMEOUT 3000  // ms

// Tilt thresholds (degrees)
const int PITCH_THRESHOLD = 25;  // up/down
const int ROLL_THRESHOLD  = 15;  // left/right
```

### IMU Ranges & Filter (set in `setup()`)

```cpp
mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
mpu.setGyroRange(MPU6050_RANGE_500_DEG);
mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
```

Adjust ranges/bandwidth if your unit is very noisy or your motions are too sensitive.

---

## Build & Upload (Arduino IDE)

1. **Install ESP32 boards**: *Boards Manager* → search "ESP32" → install Espressif package.
2. **Select board**: *Tools → Board* → your ESP32 dev board; set correct **COM/tty** port.
3. **Install libraries** (Library Manager) listed above.
4. **Open the sketch** containing this code and **Upload**.

> If serial monitor prints initialization failures, see **Troubleshooting** below.

---

## How to Play

1. Power the board. The OLED shows **“Simon Says Game – Press the button to start!”**
2. **Press the button** to start. The game generates a random sequence of arrows.
3. Watch the **level screen**, then each arrow is displayed for `MOVE_DELAY` ms.
4. **Reproduce the sequence** by tilting the board:

   * **Up**: pitch < `-PITCH_THRESHOLD`
   * **Down**: pitch > `PITCH_THRESHOLD`
   * **Left**: roll < `-ROLL_THRESHOLD`
   * **Right**: roll > `ROLL_THRESHOLD`
5. You have `INPUT_TIMEOUT` ms after your **last correct tilt** to enter the next one.
6. Finish the level to advance. Reach **Level **\`\`** + 1** to **win**. A wrong tilt or timeout shows **Game Over**. Press button to **play again**.

---

## Calibration & Sensitivity

* Start with the board flat on a table. The code computes **pitch/roll** from accelerometer data only (no fusion), so positioning matters.
* If your gestures **don’t register**, **decrease** thresholds (`PITCH_THRESHOLD`, `ROLL_THRESHOLD`).
* If false positives occur (moves trigger too easily), **increase** thresholds or reduce **filter bandwidth** noise (`MPU6050_BAND_21_HZ` → lower).
* For very jittery sensors, consider adding a small **dead‑zone** or **median filter** in the tilt functions.

---

## File/Function Map

* \`\` – Initializes Serial, button, I²C (with retries), MPU, OLED; shows start screen.
* \`\` – Periodic I²C recovery, start/reset button handling, gameplay state machine.
* \`\` – Retry helpers for robust startup.
* `** / **` – UI screens.
* `** / **`\*\* / \*\*\`\` – Game lifecycle.
* \`\` with `UpArrow/DownArrow/LeftArrow/RightArrow()` – Draws arrow glyphs.
* `** / **` – Reads tilts, checks them against the sequence, advances level, handles win/lose.
* \`\` – Computes pitch/roll and compares to thresholds.

---

## Troubleshooting

**No display / OLED init failed:**

* Confirm address `0x3C` and wiring (SDA→21, SCL→22, 3V3, GND). Some OLEDs are `0x3D` → update `OLED_ADDRESS`.
* Ensure libraries `Adafruit_SSD1306` and `Adafruit_GFX` are installed and up‑to‑date.

**MPU6050 init failed:**

* Check address `0x68`. If AD0 is pulled **HIGH**, address becomes `0x69` → update `MPU_ADDRESS`.
* Power from **3.3V**, common **GND**, stable I²C wiring. Try reducing `I2C_SPEED`.

**Random freezes / I²C lockups:**

* Keep I²C leads short; avoid hot‑plugging. The sketch already attempts **minute‑interval bus recovery**; you can increase frequency if needed.

**Gestures feel laggy or too fast:**

* Tune `MOVE_DELAY` (display pacing) and `INPUT_TIMEOUT` (how long players can think).
* Adjust thresholds or `setFilterBandwidth` to balance responsiveness vs. noise.

**Button not detected:**

* Verify **GPIO 15** isn’t used by your board for boot strapping in your setup. Any free GPIO with internal pull‑up works—update `BUTTON_PIN` accordingly.

---

## Customization Ideas

* **Sound/Buzzer** feedback per move.
* **Lives/strikes** before Game Over.
* **Accelerating pace** (reduce `MOVE_DELAY` by level).
* **Difficulty profiles** (different thresholds/timeouts).
* **High‑score** stored to NVS/EEPROM.
* **Arrow icons** redesigned for larger/smaller displays.

---

## Known Limitations

* Tilt detection uses **accelerometer‑only** angles; quick motions or vibration can cause noise. For best performance, keep the board mostly static except for intentional tilts.
* No on‑screen pause; gameplay resumes continuously within `INPUT_TIMEOUT` windows.

---

## License & Credits

* Game logic and code in the provided sketch by **Ariel Lai**.
* Uses Adafruit libraries under their respective licenses.

If you reuse or publish this project, please credit the original author. Enjoy building and have fun!

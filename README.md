# RumiCar Arduino Library

Arduino library for controlling the [RumiCar](https://www.rumicar.com/) autonomous driving platform.

RumiCar is a small autonomous-driving development platform equipped with three front VL53L0X
laser time-of-flight (ToF) distance sensors and two DC motors (steering and drive). An optional
fourth (rear) VL53L0X is supported and auto-detected at startup, so the same sketch and the same
compiled library run unchanged on vehicles with or without the rear sensor.
This library provides a thin abstraction over board-specific PWM and I2C wiring so the
same sketch can run on multiple supported architectures.

## Installation

### Arduino Library Manager (recommended)

This library is registered to the official Arduino Library Manager.

1. In Arduino IDE, open **Sketch > Include Library > Manage Libraries...**.
2. Search for `RumiCar` and click **Install**.
3. When prompted, choose **Install all** to also install the dependency (VL53L0X).

### Manual installation (ZIP)

1. Download this repository as a ZIP.
2. In Arduino IDE, go to **Sketch > Include Library > Add .ZIP Library...** and select the ZIP.
3. Manually install the [VL53L0X](https://github.com/pololu/vl53l0x-arduino) library from Library Manager.

## Dependencies

- [VL53L0X](https://github.com/pololu/vl53l0x-arduino) by Pololu — automatically installed by Library Manager when prompted.

## Supported architectures

`architectures=*` — the library compiles for all architectures via preprocessor branches:

| Architecture | Notes |
|---|---|
| AVR / ATmega328 (Arduino Uno, Nano, etc.) | Default branch. Uses analog/digital pins as labeled in the source. |
| ESP32 | PWM is driven via the LEDC peripheral (channel-based). |
| Arduino Spresense (`ARDUINO_ARCH_SPRESENSE`) | Uses `analogWrite` with a 10 kHz default frequency. |
| Raspberry Pi Pico W (`ARDUINO_RASPBERRY_PI_PICO_W`) | Custom I2C SDA/SCL pin assignment. |

Pin assignments for each board are defined in `src/RumiCar.h`.

## API

```cpp
void RC_setup();                       // Initialize sensors and motors. Call once in setup().
int  RC_steer(int direc);              // Set steering direction. direc in {LEFT, CENTER, RIGHT}.
int  RC_drive(int direc, int ipwm);    // Set drive motor. direc in {FREE, REVERSE, FORWARD, BRAKE}, ipwm 0..255.
int  RC_read (int direc);              // Read distance from a sensor. direc in {LEFT, CENTER, RIGHT, REAR}.
```

### `RC_read()` return value

| Return value | Meaning |
|---|---|
| `0` .. `2000` | Measured distance in mm (normal range). |
| `-1` | Timeout (sensor did not respond). For `REAR`, also returned on vehicles that have no rear sensor fitted. |
| `-2` | Invalid argument (`direc` was not LEFT/CENTER/RIGHT/REAR). |
| `-3` | Out of range or low signal quality (raw value above 2000 mm). |

Negative error codes (`-1`, `-2`, `-3`) keep the Y-axis of the Arduino IDE Serial Plotter
clean. The 0 .. 2000 mm normal range is preserved while errors are visible as small
downward dips.

### Direct sensor access (legacy / educational)

The distance sensors are also exposed as global `VL53L0X` objects: `sensor0` (LEFT),
`sensor1` (CENTER), `sensor2` (RIGHT), and `sensor3` (REAR, optional). Educational sketches in
`examples/` use this longer form (`sensor1.readRangeSingleMillimeters()`) on purpose, so learners
experience the underlying API before discovering the simpler `RC_read(CENTER)` alternative.

Prefer `RC_read(REAR)` over `sensor3` directly: `RC_read` returns `-1` when no rear sensor is
fitted, whereas reading `sensor3` directly on a rear-less vehicle yields an undefined value.

### Rear sensor (optional, auto-detected)

The rear VL53L0X has no XSHUT line and shares the I2C bus with the three front sensors. Because
every VL53L0X powers up at the same default address (`0x29`), `RC_setup()` resolves this entirely
internally and transparently:

1. While the three front sensors are held in reset (XSHUT low), `0x29` can only belong to the rear
   sensor. `RC_setup()` probes the bus there (and at the rear's working address) to detect whether a
   rear sensor is fitted.
2. If present, the rear is moved off `0x29` to its own address (one above the front maximum) so the
   front address assignment cannot disturb it, then it is initialized and started.
3. If absent, all rear handling is skipped and the library behaves exactly like a 3-sensor build.

Users do not need to configure anything: the same sketch and the same compiled `.hex`/binary run on
vehicles with or without the rear sensor. Just call `RC_read(REAR)` — it returns a distance when a
rear sensor is present and `-1` when it is not.


## Reserved names and library-managed state

To avoid name collisions, **do not redefine or reuse the following names** in your sketch.

### Macros (defined in `RumiCar.h`)

| Category | Names |
|---|---|
| Steering directions | `LEFT`, `CENTER`, `RIGHT` |
| Read-only direction | `REAR` (valid for `RC_read` only, not steering) |
| Drive directions | `FREE`, `REVERSE`, `FORWARD`, `BRAKE` |
| Sensor XSHUT pins | `SHUT0`, `SHUT1`, `SHUT2` |
| Motor pin macros | `AIN1_PIN`, `AIN2_PIN`, `BIN1_PIN`, `BIN2_PIN` |
| Servo pin macros (unused) | `SERVO1`, `SERVO2` |
| I2C pin macros (Pico W only) | `SCL0`, `SDA0` |
| PWM helper | `RC_analogWrite` |
| Sensor mode flags | `LONG_RANGE`, `HIGH_SPEED`, `HIGH_ACCURACY` |
| Header guard | `RumiCar_h` |

### Global variables (declared in `RumiCar.h`, defined in `RumiCar.cpp`)

| Type | Names | Purpose |
|---|---|---|
| `uint8_t` | `AIN1`, `AIN2`, `BIN1`, `BIN2` | Runtime motor pin/channel numbers. **Do not modify directly** — set by `RC_setup()`. On ESP32 these are overwritten with PWM channel numbers (0..3). |
| `VL53L0X` | `sensor0`, `sensor1`, `sensor2`, `sensor3` | Distance sensors (LEFT, CENTER, RIGHT, and optional REAR). Initialized and started in continuous mode by `RC_setup()`. `sensor3` is only initialized when a rear sensor is detected. |

### Functions

`RC_setup`, `RC_steer`, `RC_drive`, `RC_read`

### Library-managed state (side effects of `RC_setup()`)

`RC_setup()` performs the following initialization automatically. You do not need to call these yourself:

- **`Serial.begin()`** — 115200 bps on Arduino Spresense, 9600 bps on other architectures.
- **`Wire.begin()`** — I2C bus initialization (custom SDA/SCL on Pico W).
- **`sensor0.startContinuous()`, `sensor1.startContinuous()`, `sensor2.startContinuous()`** — the three front sensors are started in continuous ranging mode. `sensor3` (rear) is additionally started only if a rear sensor is detected at startup.
- **PWM setup** — board-specific (e.g., LEDC channels on ESP32, 10 kHz default frequency on Spresense).

Calling `Serial.begin()` or `Wire.begin()` again in your own `setup()` is harmless but unnecessary.

## Usage

After installing, open **File > Examples > RumiCar** to see a series of sketches:

| Example | Description |
|---|---|
| `Basic` | Minimal sketch: forward, brake, coast in a loop. |
| `01_DistanceSingle` | Print the center distance to Serial. |
| `02_DistanceTriple` | Print all three sensor distances on one line (great with Serial Plotter). |
| `03_Steering` | Alternate steering right and left. |
| `04_DriveSpeed` | Drive forward at three different PWM speeds. |
| `05_DriveForwardReverse` | Alternate forward and reverse. |
| `06_ZigZag` | Zigzag motion combining drive and steering. |
| `07_Autonomous_Center` | Distance-based autonomous control using the center sensor. |
| `08_Autonomous_AllSensors` | Full autonomous control using all three sensors. |

## License

[MIT License](LICENSE) — Copyright (c) 2026 RumiCar-group.

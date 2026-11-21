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
int  RC_steer(int direc, int ipwm = 255); // Set steering. direc in {LEFT, CENTER, RIGHT}, ipwm = strength 0..255 (optional).
int  RC_drive(int direc, int ipwm);    // Set drive motor. direc in {FREE, REVERSE, FORWARD, BRAKE}, ipwm 0..255.
int  RC_read (int direc);              // Read distance from a sensor. direc in {LEFT, CENTER, RIGHT, REAR}. 0..limit mm, or -1/-2/-3.
int  RC_setReadLimit(int limitMm);     // Set the distance limit of RC_read for all sensors. 1..2000 mm, 0 = no limit.
int  RC_setReadLimit(int direc, int limitMm); // Same, for one sensor only. direc in {LEFT, CENTER, RIGHT, REAR}.
```

### `RC_steer()` strength

`RC_steer(direc)` works as before. Since v0.6.0 an optional second argument sets the steering
strength: `RC_steer(RIGHT, 128)`.

| `ipwm` | Output |
|---|---|
| `255` (or omitted) | Full strength, exactly the same output as `RC_steer(direc)` in earlier versions. |
| `1` .. `254` | Slow decay: for `RIGHT`, AIN1 is held HIGH and AIN2 gets `255 - ipwm`; for `LEFT`, the opposite. |
| `0` | Same as `CENTER` (both inputs 0, neutral). |

- The strength is the proportion of the signal (0..255) and is not compensated; values outside
  0..255 are clamped. It is ignored for `CENTER`.
- The return value is unchanged: `1` for `LEFT`/`CENTER`/`RIGHT`, `0` for any other `direc`.
- The steering is a DC motor, not a servo, so how far the wheels turn for a given strength depends
  on the vehicle (and on the battery). Treat the value as a relative guide — "about 30 % more",
  "half" — and adjust it while watching how the car responds (see `Exercise-2.5` and `Exercise-3.3`).

### `RC_read()` return value

| Return value | Meaning |
|---|---|
| `0` .. limit | Measured distance in mm. The limit is 2000 mm by default (the maximum range of the VL53L0X), or the value set with `RC_setReadLimit()`. |
| `-1` | Timeout (sensor did not respond). For `REAR`, also returned on vehicles that have no rear sensor fitted. |
| `-2` | Invalid argument (`direc` was not LEFT/CENTER/RIGHT/REAR). |
| `-3` | Out of range: above the limit, no target (the sensor reports about 8190), or low signal quality. |

`-3` means "farther than the limit" and driving logic should treat it as open road.
Note that `-3` is the *smallest* value numerically: a plain `if (d < 100)` would take it for
"very close", so map it to a large value before comparing (`if (d == -3) d = 999;`). `-1` (no
reading) is different from `-3` and should not be treated as open road.

Negative error codes (`-1`, `-2`, `-3`) keep the Y-axis of the Arduino IDE Serial Plotter
clean. The normal range (0 .. limit) is preserved while errors are visible as small
downward dips.

### `RC_setReadLimit()`

The VL53L0X can measure up to 2 m, but its beam spreads out in a cone, so depending on where it
is mounted it can pick up the floor at a distance and report it as an obstacle. How far a reading
can be trusted is decided by your program, not by the library: call `RC_setReadLimit()` once in
`setup()`, right after `RC_setup()` (see "Choosing the limit" below for an example and how to
choose the value). Readings above the limit are returned by `RC_read()` as `-3` (far).

| Item | Description |
|---|---|
| `RC_setReadLimit(limitMm)` | Sets the same limit for all sensors (LEFT, CENTER, RIGHT, REAR). |
| `RC_setReadLimit(direc, limitMm)` | Sets the limit for one sensor only (`LEFT`, `CENTER`, `RIGHT` or `REAR`). |
| `limitMm` | `1` .. `2000` mm. `0` means no limit (back to the default, up to 2000 mm). Values outside `0` .. `2000` are clamped, as with the strength of `RC_steer()`. |
| Effect | `RC_read()` returns `-3` when the reading is above the limit of that sensor. Readings at or below the limit, `-1` and `-2` are unchanged. |
| Default | No limit: right after `RC_setup()`, `RC_read()` returns up to 2000 mm. |
| Return value | `1` when the limit was set, `0` when `direc` is invalid (nothing is changed). |
| Raw readings | `sensorN.readRangeSingleMillimeters()` is not affected. |

Write the value in one place only: the `RC_setReadLimit()` line in `setup()`. Keep the distance
thresholds of your driving logic at or below that limit, and say so in a comment, so that changing
the limit later means changing that one line. `REAR` can be set on a vehicle without a rear sensor;
`RC_read(REAR)` then still returns `-1`.

### Choosing the limit

This is the only place in this README that gives a value for the limit; the rest of the
documentation refers to "the limit set with `RC_setReadLimit()`".

- **Recommended example: 200 mm.** The hands-on text states that the RumiCar model 32 picks up
  the floor at about 30 cm; 200 mm leaves a margin below that. `Exercise-3.3` and
  `Exercise-4.2-Answer` use this value, which gives exactly the same readings as v0.5.1 .. v0.6.1.

  ```cpp
  void setup()
  {
    RC_setup();
    RC_setReadLimit(200);   // the distance RC_read trusts [mm]; beyond it RC_read returns -3 (far)
  }
  ```

- **Check it on your own car.** The distance at which the floor appears depends on the vehicle,
  the mounting height and the floor. Place the car on the floor with nothing in front of it and
  watch the readings in the Serial Plotter with `Exercise-1.2`, which prints the raw readings of the
  three sensors (not affected by the limit): a distance that shows up instead of about 8190 (no
  target) is where the floor is seen. Choose a limit with some margin below it.
- **Per sensor if needed.** The left and right sensors point forward at an angle, so the distance
  at which they see the floor may differ from the center sensor (not yet measured). If it does, set
  them separately with `RC_setReadLimit(LEFT, …)` and so on.

### Other RumiCar environments

The same API, values and return codes, including the strength argument of `RC_steer()`
(v0.6.0), are used by the Python library for the Raspberry Pi CM (`RasPi/RumiCar.py` in
[RumiCar-group/RumiCar](https://github.com/RumiCar-group/RumiCar)), by
[RumiCar-Zero2Cam](https://github.com/RumiCar-group/RumiCar-Zero2Cam), which runs your
`drive.py` with that same `RumiCar.py`, and by the
[RumiCar Simulator](https://github.com/RumiCar-group/RumiCar-Simulator). A strength of `128`
means a signal of 128/255 on each of them; the PWM frequency differs by computer module, and the
steering angle a value gives differs by vehicle. `RC_setReadLimit()` and the 2000 mm default are
being added to them with the same name and meaning (`RumiCar.py` 0.7.0, Zero2Cam 0.7.0,
Simulator v12.0.0); until those versions are released, they still limit `RC_read()` to 200 mm.

### Direct sensor access (legacy / educational)

The distance sensors are also exposed as global `VL53L0X` objects: `sensor0` (LEFT),
`sensor1` (CENTER), `sensor2` (RIGHT), and `sensor3` (REAR, optional). Educational sketches in
`examples/` use this longer form (`sensor1.readRangeSingleMillimeters()`) on purpose, so learners
experience the underlying API before discovering the simpler `RC_read(CENTER)` alternative.
These raw readings are not affected by `RC_setReadLimit()`: they return values above the limit
as they are, and about 8190 when there is no target.

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

`RC_setup`, `RC_steer`, `RC_drive`, `RC_read`, `RC_setReadLimit`

### Library-managed state (side effects of `RC_setup()`)

`RC_setup()` performs the following initialization automatically. You do not need to call these yourself:

- **`Serial.begin()`** — 115200 bps on Arduino Spresense, 9600 bps on other architectures.
- **`Wire.begin()`** — I2C bus initialization (custom SDA/SCL on Pico W).
- **`sensor0.startContinuous()`, `sensor1.startContinuous()`, `sensor2.startContinuous()`** — the three front sensors are started in continuous ranging mode. `sensor3` (rear) is additionally started only if a rear sensor is detected at startup.
- **PWM setup** — board-specific (e.g., LEDC channels on ESP32, 10 kHz default frequency on Spresense).

Calling `Serial.begin()` or `Wire.begin()` again in your own `setup()` is harmless but unnecessary.

## Usage

After installing, open **File > Examples > RumiCar** to see a series of sketches.

The `Exercise-*` sketches are the programs used in the RumiCar hands-on textbook
(see the `Hands-on text` folder of [RumiCar-group/RumiCar](https://github.com/RumiCar-group/RumiCar)),
and they keep the same names as the textbook. The numbering follows the course:
**Exercise-1** reads the sensors, **Exercise-2** controls the motors, **Exercise-3**
combines sensor readings and motor control to drive autonomously, and **Exercise-4**
improves a sketch together with an AI coding agent (see below).

| Example | Description |
|---|---|
| `Exercise` | Starting template with an empty `loop()`. In Exercise-1.1 you add one line to print the center distance. |
| `Exercise-1.1` | Print the center distance to Serial. |
| `Exercise-1.2` | Print all three sensor distances on one line (great with Serial Plotter, which is Exercise-1.3). |
| `Exercise-2.1` | Alternate steering right and left. |
| `Exercise-2.2` | Drive forward at three different PWM speeds. |
| `Exercise-2.3` | Alternate forward and reverse. |
| `Exercise-2.4` | Zigzag motion combining drive and steering. |
| `Exercise-2.5` | Change the steering strength: alternate right and left at full strength (255) and at half strength (128). |
| `Exercise-3.1` | Stop safely: distance-based control using the center sensor. |
| `Exercise-3.2` | Drive down a street: autonomous control using all three sensors. |
| `Exercise-3.3` | Drive smoothly: instead of always steering left or right as in Exercise-3.2, go straight when the left and right distances are close, and adjust the steering strength (1.3 times when it is not enough, half when the car swings past the middle). |
| `Exercise-4.1` | Starting point for the AI exercise "forward and reverse without slipping": the same code as Exercise-2.3, plus `AGENTS.md`. |
| `Exercise-4.1-Answer` | One answer: ramp the PWM up and down and brake before changing direction. |
| `Exercise-4.2` | Starting point for the AI exercise "a car that backs out when it gets stuck": the same code as Exercise-3.2, plus `AGENTS.md`. |
| `Exercise-4.2-Answer` | One answer: three ways to detect that the car is stuck (direction flips, no progress, three sides close) and a bounded back-and-forth escape maneuver. |
| `Basic` | Minimal sketch: forward, brake, coast in a loop. |

### Exercise-4: improving a sketch with an AI coding agent

In Exercise-4 the learner saves a copy of the sketch, opens the sketch folder in a terminal and asks an
AI coding agent (the textbook uses Google's Antigravity CLI, `agy`) in plain language, e.g. "make the
acceleration smoother". The agent edits the `.ino` in place and Arduino IDE 2 picks up the change
automatically; the learner reviews the diff, approves it, and uploads from the IDE as usual.

The `Exercise-4.1` and `Exercise-4.2` folders each ship an `AGENTS.md`: the rules the agent must follow
(only touch the `.ino`, only use the RumiCar API, bounded reversing, compile with the IDE's bundled
`arduino-cli` before handing back, never upload). Arduino treats `.md` as a non-compiled sketch file,
so `AGENTS.md` shows up as a tab next to the sketch and is copied along by **File > Save As**. Both
copies of `AGENTS.md` are identical (checked in CI). The `*-Answer` sketches are reference answers for
learners who want to compare after trying themselves.

Board settings differ by the computer module's USB connector: Type-C modules use processor
"ATmega328P" (`arduino:avr:nano:cpu=atmega328`); the older modules with a Mini-USB or Micro-USB connector use
"ATmega328P (Old Bootloader)" (`arduino:avr:nano:cpu=atmega328old`). Both types use the CH340 USB-serial chip, so the driver is the same.

> v0.7.0 removes the fixed 200 mm limit of `RC_read()`: by default it returns 0 .. 2000 mm again, as up to v0.5.0, and the program decides how far to trust the readings with the new `RC_setReadLimit()` (see "Choosing the limit"). `Exercise-3.3` and `Exercise-4.2-Answer` call `RC_setReadLimit(200)` in `setup()` and behave exactly as in v0.6.1; the other sketches are unchanged. A sketch of your own that uses `RC_read()` and relied on the 200 mm limit needs the same line added to `setup()`.
> v0.6.1 changes only wording in `AGENTS.md` and the comments of `Exercise-4.1`/`Exercise-4.2` (Japanese text in a plain adult style, matching the hands-on text); the code is unchanged.
> v0.6.0 adds an optional second argument (strength) to `RC_steer()` and the sketches `Exercise-2.5` and `Exercise-3.3`; the other sketches are unchanged.
> v0.5.1 limits `RC_read()` to 200 mm (readings above it return `-3`); the sketches that use `RC_read()` (`Exercise-4.2-Answer`) are adjusted, the others are unchanged.
> v0.5.0 adds `Exercise-4.1`, `Exercise-4.2` and their `*-Answer` sketches; the earlier sketches are unchanged.
> Threshold tuning and other improvements after v0.5.0 are released as v0.5.1, v0.5.2, … — Library Manager shows an **Update** button when a new version is available.
> Please report problems and tuning results as [GitHub Issues](https://github.com/RumiCar-group/RumiCar-lib/issues).
> Up to v0.3.0 these sketches were named `01_DistanceSingle` … `08_Autonomous_AllSensors`.
> From v0.4.0 they use the textbook names: `01`→`Exercise-1.1`, `02`→`Exercise-1.2`,
> `03`→`Exercise-2.1`, `04`→`Exercise-2.2`, `05`→`Exercise-2.3`, `06`→`Exercise-2.4`,
> `07`→`Exercise-3.1`, `08`→`Exercise-3.2`. The code itself is unchanged.

## License

[MIT License](LICENSE) — Copyright (c) 2026 RumiCar-group.

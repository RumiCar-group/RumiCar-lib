# RumiCar Arduino Library

Arduino library for controlling the [RumiCar](https://www.rumicar.com/) autonomous driving platform.

RumiCar is a small autonomous-driving development platform equipped with three VL53L0X
laser time-of-flight (ToF) distance sensors and two DC motors (steering and drive).
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
int  RC_read (int direc);              // Read distance from a sensor. direc in {LEFT, CENTER, RIGHT}.
```

### `RC_read()` return value

| Return value | Meaning |
|---|---|
| `0` .. `2000` | Measured distance in mm (normal range). |
| `-1` | Timeout (sensor did not respond). |
| `-2` | Invalid argument (`direc` was not LEFT/CENTER/RIGHT). |
| `-3` | Out of range or low signal quality (raw value above 2000 mm). |

Negative error codes (`-1`, `-2`, `-3`) keep the Y-axis of the Arduino IDE Serial Plotter
clean. The 0 .. 2000 mm normal range is preserved while errors are visible as small
downward dips.

### Direct sensor access (legacy / educational)

The three distance sensors are also exposed as global `VL53L0X` objects: `sensor0` (LEFT),
`sensor1` (CENTER), `sensor2` (RIGHT). Educational sketches in `examples/` use this longer
form (`sensor1.readRangeSingleMillimeters()`) on purpose, so learners experience the
underlying API before discovering the simpler `RC_read(CENTER)` alternative.

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

# RumiCar Arduino Library

Arduino library for controlling the [RumiCar](https://www.rumicar.com/) autonomous driving platform.

RumiCar is a small autonomous-driving development platform equipped with three VL53L0X
laser time-of-flight (ToF) distance sensors and two DC motors (steering and drive).
This library provides a thin abstraction over board-specific PWM and I2C wiring so the
same sketch can run on multiple supported architectures.

## Installation

### Arduino IDE (manual)

1. Download this repository as a ZIP.
2. In Arduino IDE, go to **Sketch > Include Library > Add .ZIP Library...** and select the ZIP.

### Arduino Library Manager

Once registered, install via **Sketch > Include Library > Manage Libraries...** and search for
"RumiCar".

## Dependencies

- [VL53L0X](https://github.com/pololu/vl53l0x-arduino) by Pololu (install via Library Manager).

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
```

The three distance sensors are exposed as global `VL53L0X` objects: `sensor0`, `sensor1`, `sensor2`.
Read distances with `sensor0.readRangeContinuousMillimeters()` and similar methods provided by the
underlying VL53L0X library.

## Usage

After installing, open **File > Examples > RumiCar > Basic** to see a minimal sketch that drives
the car forward, brakes, and coasts in a loop.

## License

[MIT License](LICENSE) — Copyright (c) 2026 RumiCar-group.

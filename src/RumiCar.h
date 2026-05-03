#ifndef RumiCar_h
#define RumiCar_h

// RumiCar include
//操舵用の設定
#define LEFT 0
#define CENTER 1
#define RIGHT 2

//走行用の設定
#define FREE 0
#define REVERSE 1
#define FORWARD 2
#define BRAKE 3

#include <stdint.h>

#if defined ESP32
#define RC_analogWrite ledcWrite
#define SHUT0 19
#define SHUT1 18
#define SHUT2 5

//Aが操舵、Bが走行 (ESP32ではRC_setup()内でPWMチャンネル番号に上書きされる)
#define AIN1_PIN 4
#define AIN2_PIN 26
#define BIN1_PIN 27
#define BIN2_PIN 25

#elif defined (ARDUINO_ARCH_SPRESENSE)
#define RC_analogWrite analogWrite
#define SHUT0 2
#define SHUT1 7
#define SHUT2 41

//Aが操舵、Bが走行
#define AIN1_PIN 6
#define AIN2_PIN 5
#define BIN1_PIN 9
#define BIN2_PIN 3

#elif defined ARDUINO_RASPBERRY_PI_PICO_W
#define RC_analogWrite analogWrite
// LRF
#define SCL0 13
#define SDA0 12
#define SHUT0 8
#define SHUT1 7
#define SHUT2 6

// unused
#define SERVO1 5
#define SERVO2 4

// steering & drive
#define AIN1_PIN 21
#define AIN2_PIN 22
#define BIN1_PIN 26
#define BIN2_PIN 27

#else
#define RC_analogWrite analogWrite
//SHUTはディジタルピン,A0はD14,以降同じ、注意すること
#define SHUT0 14
#define SHUT1 15
#define SHUT2 16

//Aが操舵、Bが走行
#define AIN1_PIN 3
#define AIN2_PIN 11
#define BIN1_PIN 5
#define BIN2_PIN 6
#endif

// AIN1/AIN2/BIN1/BIN2 are runtime variables (initialized from *_PIN macros in
// RC_setup()). On ESP32 they are overwritten with PWM channel numbers; on
// other architectures they hold the actual GPIO pin numbers. Library users
// should not modify these directly.
extern uint8_t AIN1;
extern uint8_t AIN2;
extern uint8_t BIN1;
extern uint8_t BIN2;

// Uncomment this line to use long range mode. This
// increases the sensitivity of the sensor and extends its
// potential range, but increases the likelihood of getting
// an inaccurate reading because of reflections from objects
// other than the intended target. It works best in dark
// conditions.

//#define LONG_RANGE


// Uncomment ONE of these two lines to get
// - higher speed at the cost of lower accuracy OR
// - higher accuracy at the cost of lower speed

#define HIGH_SPEED
//#define HIGH_ACCURACY

#include <VL53L0X.h>
extern VL53L0X sensor0;
extern VL53L0X sensor1;
extern VL53L0X sensor2;

void RC_setup(); //RumiCarのセンサとモータの初期化
int RC_steer (int direc ); //操舵の関数
int RC_drive(int direc, int ipwm); //走行の関数
int RC_read (int direc); //測距の関数 (戻り値: 0-2000=mm距離, -1=タイムアウト, -2=引数エラー, -3=範囲外/信号品質低下)

#endif /* RumiCar_h */

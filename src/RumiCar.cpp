// RumiCar Library

#include "RumiCar.h"
#include <Wire.h>

VL53L0X sensor0;
VL53L0X sensor1;
VL53L0X sensor2;

// Runtime motor pin/channel variables. Initialized from board-specific
// *_PIN macros at the top of RC_setup(). On ESP32 they are subsequently
// overwritten with LEDC PWM channel numbers (0..3) so that RC_analogWrite
// (= ledcWrite) operates on channels rather than GPIO numbers.
uint8_t AIN1 = AIN1_PIN;
uint8_t AIN2 = AIN2_PIN;
uint8_t BIN1 = BIN1_PIN;
uint8_t BIN2 = BIN2_PIN;

namespace
{
VL53L0X& getSensor(uint8_t pin)
{
  return pin == SHUT0 ? sensor0 : pin == SHUT1 ? sensor1 : sensor2;
}

void setSensorAddress(uint8_t pin, uint8_t address)
{
  // turn on the device via INPUT, which enables XSHUT pull-up resistor
  // , so it is equivalent to digitalWrite(pin, HIGH);
  pinMode(pin, INPUT);
  delay(150);

  auto& sensor = getSensor(pin);
  sensor.init(true);
  delay(100);

  // set address which stays until next reboot
  sensor.setAddress(address);
  sensor.setTimeout(500);

  // next device won't overwrite it
}

void initI2C()
{
  // turn off VL53L0X, so later turn them on one by one and set address
  digitalWrite(SHUT0, LOW);
  digitalWrite(SHUT1, LOW);
  digitalWrite(SHUT2, LOW);
  delay(150);

#if defined SDA0 && defined SCL0
  Wire.setSDA(SDA0);
  Wire.setSCL(SCL0);
#endif
  Wire.begin();

  // address needs to be set on each boot
  setSensorAddress(SHUT0, 20);
  setSensorAddress(SHUT1, 21);
  setSensorAddress(SHUT2, 22);
}
}

void RC_setup()
{
  // (Re)initialize motor pin/channel variables on every boot. These may have
  // been overwritten with PWM channel numbers on ESP32 in a previous run.
  AIN1 = AIN1_PIN;
  AIN2 = AIN2_PIN;
  BIN1 = BIN1_PIN;
  BIN2 = BIN2_PIN;

#if defined (ARDUINO_ARCH_SPRESENSE)
  Serial.begin(115200);
#else
  Serial.begin(9600);
#endif

  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(SHUT0, OUTPUT);
  pinMode(SHUT1, OUTPUT);
  pinMode(SHUT2, OUTPUT);

  initI2C();

#if defined LONG_RANGE
  // lower the return signal rate limit (default is 0.25 MCPS)
  sensor0.setSignalRateLimit(0.1);
  sensor1.setSignalRateLimit(0.1);
  sensor2.setSignalRateLimit(0.1);
  // increase laser pulse periods (defaults are 14 and 10 PCLKs)
  sensor0.setVcselPulsePeriod(VL53L0X::VcselPeriodPreRange, 18);
  sensor1.setVcselPulsePeriod(VL53L0X::VcselPeriodPreRange, 18);
  sensor2.setVcselPulsePeriod(VL53L0X::VcselPeriodPreRange, 18);
  sensor0.setVcselPulsePeriod(VL53L0X::VcselPeriodFinalRange, 14);
  sensor1.setVcselPulsePeriod(VL53L0X::VcselPeriodFinalRange, 14);
  sensor2.setVcselPulsePeriod(VL53L0X::VcselPeriodFinalRange, 14);
#endif

#if defined HIGH_SPEED
  // reduce timing budget to 20 ms (default is about 33 ms)
  sensor0.setMeasurementTimingBudget(20000);
  sensor1.setMeasurementTimingBudget(20000);
  sensor2.setMeasurementTimingBudget(20000);
#elif defined HIGH_ACCURACY
  // increase timing budget to 200 ms
  sensor0.setMeasurementTimingBudget(200000);
  sensor1.setMeasurementTimingBudget(200000);
  sensor2.setMeasurementTimingBudget(200000);
#endif

  sensor0.startContinuous();
  sensor1.startContinuous();
  sensor2.startContinuous();

#if defined ESP32
  //ESP32の場合はピン番号ではなくチャンネルでPWMを行うのでチャンネルとして再設定
  const uint8_t PWM_resolution = 8;
  // 8の場合8bitの解像度でArduinoと同じ
  //モータのPWMのチャンネル、周波数の設定
  ledcSetup(0, 490, PWM_resolution);
  ledcSetup(1, 490, PWM_resolution);
  ledcSetup(2, 960, PWM_resolution);
  ledcSetup(3, 960, PWM_resolution);

  //モータのピンとチャンネルの設定
  ledcAttachPin(AIN1, 0);
  ledcAttachPin(AIN2, 1);
  ledcAttachPin(BIN1, 2);
  ledcAttachPin(BIN2, 3);
  //pin番号をチャンネル番号に上書き
  AIN1 = 0;
  AIN2 = 1;
  BIN1 = 2;
  BIN2 = 3;

#elif defined (ARDUINO_ARCH_SPRESENSE)

  analogWriteSetDefaultFreq(10000);

#endif

  RC_analogWrite(AIN1, 0);
  RC_analogWrite(AIN2, 0);
  RC_analogWrite(BIN1, 0);
  RC_analogWrite(BIN2, 0);

}
//操舵の関数
int RC_steer (int direc ){
  if ( direc == RIGHT ){
    RC_analogWrite(AIN1,255);
    RC_analogWrite(AIN2,0);
  }else if ( direc == LEFT ){
    RC_analogWrite(AIN1,0);
    RC_analogWrite(AIN2,255);
  }else if ( direc == CENTER ){
    RC_analogWrite(AIN1,0);
    RC_analogWrite(AIN2,0);
  }else{
    return 0;
  }
  return 1;
}

// 走行モータの駆動方式・始動キックの調整パラメータ
// (実機・低電池寄りの条件でキャリブレーションして確定する)
#define KICK_DUTY 200   // 走り出し時に静止摩擦を破る一発の大きさ (0-255)
#define KICK_MS   20    // キックを与える時間 [ms]

// スローデケイ(drive/brake)で走行モータを駆動する内部関数。
// DRV8835(IN/IN)では片方の入力をHIGH固定し、もう片方を反転PWMすることで
// 駆動とブレーキを交互に切り替える(=スローデケイ)。fast decay(drive/coast)より
// 低dutyでの始動性と duty-速度の線形性が向上する。
// s=0 のとき両入力HIGH=ブレーキ相当、s=255 で連続駆動。
static void driveSlowDecay(int direc, int s)
{
  if ( direc == FORWARD ){
    RC_analogWrite(BIN1, 255);
    RC_analogWrite(BIN2, 255 - s);
  }else{ // REVERSE
    RC_analogWrite(BIN2, 255);
    RC_analogWrite(BIN1, 255 - s);
  }
}

//走行の関数
int RC_drive(int direc, int ipwm){
  static int prevDirec = FREE;   // 直前の駆動状態(走り出しエッジ検出用)

  if ( ipwm < 0 )   ipwm = 0;    // 入力ガード
  if ( ipwm > 255 ) ipwm = 255;

  if ( direc == FREE ){
    RC_analogWrite(BIN1,0);
    RC_analogWrite(BIN2,0);
  }else if ( direc == REVERSE || direc == FORWARD ){
    // 停止/逆方向からの走り出しエッジでのみ始動キックを打つ
    if ( prevDirec != direc && ipwm > 0 ){
      driveSlowDecay(direc, KICK_DUTY);
      delay(KICK_MS);            // Timer0は変更しないのでdelayは正確
    }
    driveSlowDecay(direc, ipwm); // 目標速度(スローデケイ)
  }else if ( direc == BRAKE ){
    // 確実停止: 両入力HIGH (ipwmは無視)
    RC_analogWrite(BIN1,255);
    RC_analogWrite(BIN2,255);
  }else{
    return 0;                    // 未知の方向: prevDirec更新せず
  }

  prevDirec = direc;
  return 1;
}

// 距離測定関数: 指定方向のセンサで距離を取得
// 戻り値: 0-2000=mm距離(正常), -1=タイムアウト, -2=引数エラー, -3=範囲外/信号品質低下
int RC_read(int direc)
{
  VL53L0X *sensor;

  switch (direc) {
    case LEFT:   sensor = &sensor0; break;
    case CENTER: sensor = &sensor1; break;
    case RIGHT:  sensor = &sensor2; break;
    default: return -2;  // 引数エラー
  }

  uint16_t raw = sensor->readRangeSingleMillimeters();

  if (sensor->timeoutOccurred()) {
    return -1;  // タイムアウト
  }

  if (raw > 2000) {
    return -3;  // 範囲外/信号品質低下
  }

  return (int)raw;  // 正常値 (0-2000mm)
}

#include <RumiCar.h>

// Exercise-4.1-Answer スリップしない前進と後進（解答例）
//   いきなり全速にせず、少しずつ速くして、止まる前に少しずつ遅くします。
//   向きを変える前に一度ブレーキで止まるのがコツです。
//   数値（1 段の量・時間・最高速・最低速）は車体と床に合わせて変えてみましょう。

const int STEP_PWM = 15;    // 1 段で変える速さ（PWM）
const int STEP_MS  = 40;    // 1 段あたりの時間 [ms]
const int MAX_PWM  = 255;   // 最高速
const int MIN_PWM  = 70;    // 走り出し・止まる直前の速さ（これより遅いとモーターが回らない）
const int HOLD_MS  = 300;   // 最高速で走る時間 [ms]
const int STOP_MS  = 300;   // 止まってから向きを変えるまでの時間 [ms]

void setup()
{
  RC_setup();
}

// direc の向きに、ゆっくり加速 → 少し走る → ゆっくり減速 → 停止
void rampDrive(int direc)
{
  for (int pwm = MIN_PWM; pwm <= MAX_PWM; pwm += STEP_PWM) {   // 加速
    RC_drive(direc, pwm);
    delay(STEP_MS);
  }
  RC_drive(direc, MAX_PWM);
  delay(HOLD_MS);
  for (int pwm = MAX_PWM; pwm >= MIN_PWM; pwm -= STEP_PWM) {   // 減速
    RC_drive(direc, pwm);
    delay(STEP_MS);
  }
  RC_drive(BRAKE, 0);                                           // 止まってから向きを変える
  delay(STOP_MS);
}

void loop()
{
  rampDrive(FORWARD);
  rampDrive(REVERSE);
}

#include <RumiCar.h>

// Exercise-4.2-Answer はまったら切り返す車（解答例）
//   Exercise-3.2 の走り方に、「はまったことに気づく」3 つの方法と、
//   「ハンドルを切り返して脱出する」手順を足したものです。
//   使えるのは前・左・右の距離センサーと、自分がモーターに何を命じたかだけです。
//
//   はまりの検知（どれか 1 つでも成立したら脱出に入る）
//     (1) 前後の切り替えが短い時間に何回も起きた
//     (2) 前進や後進を命じているのに、前の距離がほとんど変わらない
//     (3) 前・左・右がすべて近い（角に鼻先が入っている）
//   脱出（切り返し）
//     近い壁の側にハンドルを切って少し後退 → 遠い側に切って少し前進 → 前が開いたか確かめる。
//     後退するときは「近い壁の側」にハンドルを切るのがコツです（鼻先が壁から離れる向きに振れる）。
//     決めた回数で抜けられなければ止まって待ち、前が開いたら走行を再開します。
//   数値はすべて目安です。自分の車体とコースに合わせて変えてみましょう。

// ---- 3.2 と同じ走り方のパラメータ ----
const int PWM_SLOW   = 150;
const int PWM_MID    = 200;
const int PWM_FAST   = 255;

// ---- はまりの検知 ----
const unsigned long FLIP_WINDOW_MS = 3000;  // (1) この時間の中で
const int           FLIP_LIMIT     = 4;     // (1) 前後が何回入れ替わったら「はまり」か
const unsigned long STALL_CHECK_MS = 1000;  // (2) 前の距離を比べる間隔 [ms]
const int           STALL_DELTA_MM = 20;    // (2) これより変化が小さければ「進んでいない」（測距誤差 1〜2cm を考慮）
const int           STALL_NEAR_MM  = 200;   // (2) 前がこれより近いときだけ判定する（遠い壁は見ない）
const int           STALL_LIMIT    = 2;     // (2) 何回続いたら「はまり」か（最初の 1 回は比べる相手が無いので検知まで約 3 秒）
const int           CORNER_MM      = 120;   // (3) 前・左・右がすべてこれ未満なら角に入っている

// ---- 脱出（切り返し） ----
const int           ESCAPE_PWM     = 200;   // 切り返しの速さ
const unsigned long BACK_MS        = 600;   // 後退する時間 [ms]
const unsigned long FWD_MS         = 400;   // 前進する時間 [ms]
const int           ESCAPE_TRIES   = 3;     // 切り返しの最大回数
const int           CLEAR_MM       = 250;   // 前がこれ以上開いたら脱出成功
const int           REAR_STOP_MM   = 80;    // 後方センサーがある車体：後ろがこれより近ければ後退をやめる
const unsigned long COOLDOWN_MS    = 2000;  // 脱出のあと、再びはまりを判定しない時間 [ms]

// ---- 状態を覚えておく変数 ----
int           lastDirec   = FREE;   // 直前に命じた走行方向（FORWARD / REVERSE）
unsigned long flipStart   = 0;      // (1) 数え始めた時刻
int           flipCount   = 0;      // (1) 前後の切り替え回数
unsigned long stallTime   = 0;      // (2) 前回比べた時刻
int           stallPrev   = -1;     // (2) 前回の前の距離（-1 = まだ無い）
int           stallCount  = 0;      // (2) 「進んでいない」が続いた回数
unsigned long cooldownEnd = 0;      // 脱出のあと、この時刻までは判定しない

void setup()
{
  RC_setup();
}

// 距離が測れなかったとき（-1 や -3）は「遠い」として扱う（3.2 の 8190 と同じ考え方）
int farIfInvalid(int d)
{
  return (d < 0) ? 9999 : d;
}

// 前が開いているか（遠い、または測れないほど遠い）
bool isClear(int s1)
{
  return (s1 == -3) || (s1 >= CLEAR_MM);
}

// RC_drive の代わりに使う。前後の切り替え回数を数える（検知 (1) のため）
void driveAndCount(int direc, int pwm)
{
  if ((direc == FORWARD && lastDirec == REVERSE) ||
      (direc == REVERSE && lastDirec == FORWARD)) {
    unsigned long now = millis();
    if (flipCount == 0 || now - flipStart > FLIP_WINDOW_MS) {   // 数え直し
      flipStart = now;
      flipCount = 0;
    }
    flipCount++;
  }
  if (direc == FORWARD || direc == REVERSE) {
    lastDirec = direc;
  }
  RC_drive(direc, pwm);
}

// (1) 前後の切り替えが短い時間に何回も起きたか
bool stuckByFlip()
{
  return (flipCount >= FLIP_LIMIT) && (millis() - flipStart <= FLIP_WINDOW_MS);
}

// (2) 前進や後進を命じているのに、前の距離がほとんど変わらないか
bool stuckByNoProgress(int s1)
{
  unsigned long now = millis();
  if (now - stallTime < STALL_CHECK_MS) {
    return false;                                    // まだ比べる時刻ではない
  }
  stallTime = now;
  bool moving = (lastDirec == FORWARD || lastDirec == REVERSE);
  bool near   = (s1 >= 0 && s1 < STALL_NEAR_MM);     // 測れていて、しかも近い
  if (moving && near && stallPrev >= 0 && abs(s1 - stallPrev) < STALL_DELTA_MM) {
    stallCount++;
  } else {
    stallCount = 0;
  }
  stallPrev = near ? s1 : -1;
  return stallCount >= STALL_LIMIT;
}

// (3) 前・左・右がすべて近いか（角に鼻先が入っている）
bool stuckByThreeSides(int s0, int s1, int s2)
{
  return (lastDirec == FORWARD) &&
         (s0 >= 0 && s0 < CORNER_MM) &&
         (s1 >= 0 && s1 < CORNER_MM) &&
         (s2 >= 0 && s2 < CORNER_MM);
}

void resetDetectors()
{
  flipCount  = 0;
  stallCount = 0;
  stallPrev  = -1;
  stallTime  = millis();
  cooldownEnd = millis() + COOLDOWN_MS;
}

// 切り返して脱出する
void escape(int s0, int s2)
{
  // 近い壁の側を決める（測れない側は「遠い」とみなす）
  int nearSide = (farIfInvalid(s0) < farIfInvalid(s2)) ? LEFT : RIGHT;
  int farSide  = (nearSide == LEFT) ? RIGHT : LEFT;

  for (int i = 0; i < ESCAPE_TRIES; i++) {
    // 後退：近い壁の側にハンドルを切る（鼻先が壁から離れる）
    RC_steer(nearSide);
    RC_drive(REVERSE, ESCAPE_PWM);
    unsigned long t0 = millis();
    while (millis() - t0 < BACK_MS) {
      int rear = RC_read(REAR);                       // 後方センサーが無い車体は -1 が返る
      if (rear >= 0 && rear < REAR_STOP_MM) {
        break;                                        // 後ろが近い：後退をやめる
      }
    }
    RC_drive(BRAKE, 0);
    delay(100);

    // 前進：遠い側にハンドルを切る
    RC_steer(farSide);
    RC_drive(FORWARD, ESCAPE_PWM);
    delay(FWD_MS);
    RC_drive(BRAKE, 0);
    delay(100);

    // 前が開いたか確かめる
    if (isClear(RC_read(CENTER))) {
      Serial.println("ESCAPED");
      lastDirec = FORWARD;
      resetDetectors();
      return;
    }
  }

  // 抜けられなかった：止まって待ち、前が開いたら再開する（手で助けてもらう）
  RC_drive(FREE, 0);
  Serial.println("GIVE UP: waiting");
  while (!isClear(RC_read(CENTER))) {
    delay(100);
  }
  lastDirec = FORWARD;
  resetDetectors();
}

// Exercise-3.2 と同じ走り方
void driveLikeExercise32(int s0, int s1, int s2)
{
  int d0 = farIfInvalid(s0);
  int d1 = farIfInvalid(s1);
  int d2 = farIfInvalid(s2);

  if (d1 < 100) {
    driveAndCount(REVERSE, PWM_SLOW);
  } else if (d1 < 150) {
    driveAndCount(FORWARD, PWM_SLOW);
  } else if (d1 < 250) {
    driveAndCount(FORWARD, PWM_MID);
  } else {
    driveAndCount(FORWARD, PWM_FAST);
  }
  if (d0 > d2) {
    RC_steer(LEFT);
  } else {
    RC_steer(RIGHT);
  }
}

void loop()
{
  int s0 = RC_read(LEFT);
  int s1 = RC_read(CENTER);
  int s2 = RC_read(RIGHT);

  Serial.print("L:");  Serial.print(s0);
  Serial.print(" C:"); Serial.print(s1);
  Serial.print(" R:"); Serial.print(s2);
  Serial.print(" flips:"); Serial.print(flipCount);
  Serial.print(" stall:"); Serial.println(stallCount);

  bool canJudge = (millis() >= cooldownEnd);   // 脱出の直後は判定しない
  if (canJudge && (stuckByFlip()
                   || stuckByNoProgress(s1)
                   || stuckByThreeSides(s0, s1, s2))) {
    Serial.println("STUCK");
    escape(s0, s2);                           // 切り返し（最大 ESCAPE_TRIES 回）
    return;
  }

  driveLikeExercise32(s0, s1, s2);            // 3.2 と同じ走り方
}

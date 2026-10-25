#include <RumiCar.h>

// Exercise-4.2 はまったら切り返す車（AI と一緒に改良する）
//   カーブで壁に斜めに当たると、前後に小刻みに動くだけで抜け出せなくなります。
//   (1) はまったことを自分で知る方法  (2) ハンドルを切り返して脱出する手順
//   の 2 つを、まず自分で考えてから AI に頼みましょう（方針を伝えると良い結果になります）。
//   1. 「ファイル」→「名前を付けて保存」で、まずスケッチブックに保存する
//   2. 「スケッチ」→「スケッチのフォルダを表示」→ そのフォルダをターミナルで開き agy を起動する
//   AI へのお約束は隣のタブ AGENTS.md にあります。解答例は Exercise-4.2-Answer。
// 下のコードは Exercise-3.2 と同じです。

void setup()
{
  RC_setup();
}

void loop()
{

  int ibound =250;
  int s0, s1, s2;
  s0=sensor0.readRangeSingleMillimeters();
  s1=sensor1.readRangeSingleMillimeters();
  s2=sensor2.readRangeSingleMillimeters();


  Serial.print("Sensor0:");
  Serial.print(s0);
  Serial.print("  Sensor1:");
  Serial.print(s1);
  Serial.print("  Sensor2:");
  Serial.println(s2);

  if(s1<100){
    RC_drive(REVERSE,150);
  }else if (s1<150){
    RC_drive(FORWARD,150);
  }else if (s1<250){
    RC_drive(FORWARD,200);
  }else{
    RC_drive(FORWARD,255);
  }
  if(s0>s2){
    RC_steer(LEFT);
  }else{
    RC_steer(RIGHT);
  }
}

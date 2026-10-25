#include <RumiCar.h>

// Exercise-4.1 スリップしない前進と後進（AI と一緒に改良する）
//   1. 「ファイル」→「名前を付けて保存」で、まずスケッチブックに保存する
//   2. 「スケッチ」→「スケッチのフォルダを表示」→ そのフォルダをターミナルで開き agy を起動する
//   3. 例：「加減速をもう少しスムースにして」と頼み、差分を読んでから承認する
//   AI へのお約束は隣のタブ AGENTS.md にあります。解答例は Exercise-4.1-Answer。
// 下のコードは Exercise-2.3 と同じです。

void setup()
{
  RC_setup();
}

void loop()
{
  RC_drive(FORWARD,255);
  delay(500);
  RC_drive(REVERSE,255);
  delay(500);
}

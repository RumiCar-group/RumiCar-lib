# AGENTS.md — RumiCar の Exercise を AI と一緒に改良するときのお約束

> **English summary (the rules below are in Japanese):** This folder is an Arduino sketch for RumiCar, a small self-driving teaching car. The users are programming-class students (children to adults) practicing how to improve a sketch together with an AI. Rules: edit only the `.ino` in this folder — never create files or edit `AGENTS.md`; never upload to the board (a person uploads from Arduino IDE); use only the RumiCar API (`RC_setup`, `RC_steer`, `RC_drive`, `RC_read` — see the table in section 2); every reversing routine needs a time or count limit, no unbounded loops, `pwm` within 0–255, `delay()` of a few seconds at most, keep `setup()`/`loop()`; keep changes minimal; reply in the language the user writes in, briefly (about 3 lines: what you changed and why), ask one question if the request is ambiguous; after editing, compile-check (no upload) with the Arduino IDE's bundled arduino-cli using the FQBN for the user's CM port — Type-C (default): `arduino:avr:nano:cpu=atmega328`, older Mini-USB or Micro-USB: `arduino:avr:nano:cpu=atmega328old` — commands in section 5.

このフォルダは RumiCar（小さな自動運転の教材用の車）の Arduino スケッチです。
使っている人はプログラミング教室の参加者（小学生〜大人）で、AI と一緒にスケッチを改良する練習をしています。
あなた（AI）は次の約束を守って手伝ってください。

## 1. 触ってよいもの・いけないもの

- 編集してよいのは、このフォルダの `.ino` ファイルだけです。
- 新しいファイルを作らないでください。`AGENTS.md`（このファイル）も変更しないでください。
- 書き込み（アップロード）はしないでください。車への書き込みは、人が Arduino IDE の「書き込み」ボタンで行います。
- 変更は頼まれた範囲の最小限にとどめ、頼まれていない部分の書き換えや整形はしないでください。

## 2. 使ってよい関数（RumiCar ライブラリ）

スケッチは `#include <RumiCar.h>` で RumiCar ライブラリを使います。モーターやセンサーは次の関数だけで扱ってください（ピン番号を直接使ったり、`analogWrite` や `Wire` を直接呼んだりしないでください）。

| 関数 | 意味 |
|---|---|
| `RC_setup()` | センサーとモーターの初期化。`setup()` で 1 回呼ぶ（すでに書いてあります） |
| `RC_steer(direc)` | ハンドル。`direc` は `LEFT` / `CENTER` / `RIGHT` |
| `RC_drive(direc, pwm)` | 走行モーター。`direc` は `FORWARD` / `REVERSE` / `BRAKE` / `FREE`、`pwm` は 0〜255（速さ） |
| `RC_read(direc)` | 距離 [mm]。`direc` は `LEFT` / `CENTER` / `RIGHT` / `REAR`。戻り値 0〜2000 が距離、`-1` は応答なし（後方センサーが無い車体の `REAR` も `-1`）、`-2` は引数エラー、`-3` は範囲外（遠すぎる） |

- 古い書き方 `sensor0.readRangeSingleMillimeters()`（左）、`sensor1…`（中央）、`sensor2…`（右）も使えます。この書き方では遠すぎるとき 8190 前後の大きな値が返ります。どちらの書き方でも「測れなかったとき」の扱いを忘れないでください。
- 距離センサーは誤差が 1〜2 cm あり、30 cm より先は床を拾うことがあります。判定のしきい値は 30 cm 以内で、余裕を持たせてください。
- `RC_drive` は向きが変わるとき自動で短い始動キックを入れます。`delay()` と `millis()` は普通に使えます。
- シリアルは `RC_setup()` が 9600 bps で開いています。`Serial.print` で値を表示してかまいません。

## 3. 安全のための約束

- 後退（`REVERSE`）を続ける処理には、必ず時間か回数の上限を付けてください。
- 止まらない繰り返し（抜け出せない `while`）を作らないでください。
- `pwm` は 0〜255 の範囲、`delay()` は数秒以内にしてください。
- 元のプログラムの構造（`setup()` と `loop()`）は残してください。

## 4. 書き方とやり取り

- 相手は初心者です。変数名は意味の分かる英語にし、変更した場所には短いコメントを付けてください（コメントは相手が使っている言語で）。
- 返事は相手が使っている言語で（日本語で頼まれたら日本語、英語なら英語）。「何を・なぜ変えたか」を 3 行程度で説明してください。長い解説は不要です。
- 頼まれた内容があいまいなときは、勝手に決めずに 1 つだけ質問してください。

## 5. 変更したらコンパイルを確認する

編集のあと、Arduino IDE に同梱の arduino-cli でコンパイルが通ることを確認してください（書き込みはしません）。
ボードは Arduino Nano です。端子の種類で設定が違います。**既定は Type-C 端子の CM** です。

| CM の端子 | FQBN |
|---|---|
| Type-C（既定） | `arduino:avr:nano:cpu=atmega328` |
| Mini-USB または Micro-USB（旧型） | `arduino:avr:nano:cpu=atmega328old` |

Windows（PowerShell）：

```
& "$env:LOCALAPPDATA\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe" --config-file "$env:USERPROFILE\.arduinoIDE\arduino-cli.yaml" compile --fqbn arduino:avr:nano:cpu=atmega328 .
```

macOS：

```
"/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli" --config-file "$HOME/.arduinoIDE/arduino-cli.yaml" compile --fqbn arduino:avr:nano:cpu=atmega328 .
```

コンパイルが通らなかったら、エラーの行を直してからもう一度確認してください。arduino-cli が見つからないときは、その旨を伝えて、人に IDE の「検証」ボタンで確認してもらってください。

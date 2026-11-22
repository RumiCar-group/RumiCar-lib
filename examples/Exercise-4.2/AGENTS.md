# AGENTS.md — RumiCar の Exercise を AI と一緒に改良するときの決まり

> **English summary (the rules below are in Japanese):** This folder is an Arduino sketch for RumiCar, a small self-driving teaching car. The users are programming-class students (children to adults) practicing how to improve a sketch together with an AI. Rules: edit only the `.ino` in this folder — never create files or edit `AGENTS.md`; never upload to the board (a person uploads from Arduino IDE); use only the RumiCar API (`RC_setup`, `RC_setReadLimit`, `RC_steer`, `RC_drive`, `RC_read` — see the table in section 2); when using `RC_read`, first split its value into a distance (0 or more) and a status (a negative number, handled in one block: `-1` = no answer from a front sensor → stop, `-3` = out of range, nothing before the limit; to compare `-3` as "far", replace it with a constant at the top such as `FAR_MM = 9999`, used only for comparisons, never in a left/right difference); the distance limit is the constant `LIMIT_MM` at the top of the sketch, set with `RC_setReadLimit(LIMIT_MM)` in `setup()` (if it is missing, add the constant and the call right after `RC_setup()` and ask the user for the value), and new distance thresholds must be at or below `LIMIT_MM`; every reversing routine needs a time or count limit, no unbounded loops, `pwm` and the steering strength within 0–255, `delay()` of a few seconds at most, keep `setup()`/`loop()`; keep changes minimal; reply in the language the user writes in, briefly (about 3 lines: what you changed and why), ask one question if the request is ambiguous; after editing, compile-check (no upload) with the Arduino IDE's bundled arduino-cli using the FQBN for the user's CM port — Type-C (default): `arduino:avr:nano:cpu=atmega328`, older Mini-USB or Micro-USB: `arduino:avr:nano:cpu=atmega328old` — commands in section 5.

このフォルダは RumiCar（小さな自動運転の教材用の車）の Arduino スケッチです。
使っている人はプログラミング教室の参加者（小学生〜大人）で、AI と一緒にスケッチを改良する練習をしています。
あなた（AI）は次の決まりを守って手伝ってください。

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
| `RC_setReadLimit(limit)`<br>`RC_setReadLimit(direc, limit)` | `RC_read` が信じる距離の上限 [mm]。値はスケッチの先頭の定数 `LIMIT_MM` に書き、`setup()` で `RC_setup()` の次に `RC_setReadLimit(LIMIT_MM)` と呼ぶ。上限を超える値は `RC_read` が `-3` にする。`limit` は 1〜2000、`0` は上限なし。`direc`（`LEFT` / `CENTER` / `RIGHT` / `REAR`）を付けるとその方向だけ |
| `RC_steer(direc)`<br>`RC_steer(direc, pwm)` | ハンドル。`direc` は `LEFT` / `CENTER` / `RIGHT`。`pwm` はハンドルの強さ 0〜255（省略すると 255 でいちばん強い。`0` は `CENTER` と同じ。`CENTER` のときは使わない） |
| `RC_drive(direc, pwm)` | 走行モーター。`direc` は `FORWARD` / `REVERSE` / `BRAKE` / `FREE`、`pwm` は 0〜255（速さ） |
| `RC_read(direc)` | 距離 [mm]。`direc` は `LEFT` / `CENTER` / `RIGHT` / `REAR`。値はまず 0 以上かで分けて扱う：0〜上限 は距離（上限は `LIMIT_MM`）。負の数はステータスで距離ではない：`-1` は応答なし（前・左・右なら止まる。後方センサーが無い車体の `REAR` も `-1`）、`-2` は引数エラー、`-3` は範囲外（上限より先。対象物が無いときや信号が弱いときも `-3`）。`-3` を「遠い」として比べるときは、先頭の定数（例 `FAR_MM = 9999`）に置き換える。ステータスの処理は 1 つのブロックにまとめる |

- 古い書き方 `sensor0.readRangeSingleMillimeters()`（左）、`sensor1…`（中央）、`sensor2…`（右）も使えます。この書き方は `RC_setReadLimit` の影響を受けず、上限を超える値もそのまま返り、遠すぎるときは 8190 前後の大きな値が返ります。どちらの書き方でも「測れなかったとき」の扱いを忘れないでください。
- 距離センサーは誤差が 1〜2 cm あり、遠くでは床を拾うことがあります。床を拾わない範囲を、先頭の定数 `LIMIT_MM`（`setup()` の `RC_setReadLimit(LIMIT_MM)`）で上限として決めています。新しく決める判定のしきい値は `LIMIT_MM` 以下にし、余裕を持たせてください（上限の値を変えても、この決まりはそのままです）。`RC_setReadLimit` に数を直接書いているスケッチでは、その数が上限です。
- `RC_read` を使うのに `setup()` に `RC_setReadLimit` が無いときは、スケッチの先頭に `const int LIMIT_MM = 上限;` を、`RC_setup()` の次の行に `RC_setReadLimit(LIMIT_MM);` を追加してください。上限の値は自分で決めずに、人に 1 つ質問して決めてください。
- `FAR_MM` は大小の比較（しきい値、どちらが広いか）だけに使ってください。左右の差のように値の大きさを使う計算に `-3` の置き換えの値を入れず、片側が `-3` のときは場合を分けて扱ってください。
- 古い書き方を `RC_read` に書き換えるときは、上限より大きいしきい値は成り立たなくなります（上限より先は `-3` になります）。そのようなしきい値は上限以下に直してください。
- ハンドルはサーボではなく DC モーターなので、強さに対してどれだけ曲がるかは車体や電池で変わります。強さは「3 割増し」「半分」のような目安として使い、効かなければ増やし、行き過ぎたら減らすように決めてください。
- `RC_drive` は向きが変わるとき自動で短い始動キックを入れます。`delay()` と `millis()` は普通に使えます。
- シリアルは `RC_setup()` が 9600 bps で開いています。`Serial.print` で値を表示してかまいません。

## 3. 安全のための決まり

- 後退（`REVERSE`）を続ける処理には、必ず時間か回数の上限を付けてください。
- 止まらない繰り返し（抜け出せない `while`）を作らないでください。
- `pwm`（速さとハンドルの強さ）は 0〜255 の範囲、`delay()` は数秒以内にしてください。
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

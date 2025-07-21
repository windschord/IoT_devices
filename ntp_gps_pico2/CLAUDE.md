# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 開発コマンド

### ビルドとアップロード
```bash
# プロジェクトをビルド
pio build

# Raspberry Pi Pico 2にビルドしてアップロード
pio run -t upload

# ビルド成果物をクリーンアップ
pio run -t clean
```

### テストとデバッグ
```bash
# コードチェック（アップロードなしでコンパイル）
pio check

# シリアル出力をモニター
pio device monitor

# 特定のボーレートでモニター
pio device monitor -b 115200
```

### ライブラリ管理
```bash
# ライブラリのインストール/更新
pio lib install

# インストール済みライブラリの一覧
pio lib list

# 全ライブラリの更新
pio lib update
```

## プロジェクトアーキテクチャ

### ハードウェアプラットフォーム
- **ターゲット**: Raspberry Pi Pico 2 (RP2350)
- **GPSモジュール**: SparkFun GNSS Timing Breakout ZED-F9T
- **イーサネット**: W5500モジュール
- **ディスプレイ**: SH1106 OLED (128x64)
- **RTC**: DS3231（時刻バックアップ用）

### 主要コンポーネント

#### GPS時刻同期 (src/Gps_Client.cpp/h)
- SparkFun u-blox GNSSライブラリとの統合
- マルチコンステレーション対応（GPS、GLONASS、Galileo、BeiDou、QZSS）
- QZSS L1S災害警報信号処理
- 高精度PPS（Pulse Per Second）信号処理
- UBXプロトコルメッセージ処理（NAV-PVT、RXM-SFRBX、NAV-SAT）

#### ネットワークサービス (src/webserver.cpp/h)
- W5500モジュール経由のイーサネット接続
- ステータス監視用Webインターフェース
- Prometheusメトリクスエンドポイント（/metrics）
- NTPサーバー機能（計画中）

#### メイン制御ループ (src/main.cpp)
- GPSデータ処理と表示
- イーサネットステータス監視
- OLEDディスプレイ管理
- PPS割り込み処理
- ステータスLED制御

### ピン設定
```cpp
#define GPS_PPS_PIN 8        // PPS信号入力
#define GPS_SDA_PIN 6        // GPS I2Cデータ（Wire1）
#define GPS_SCL_PIN 7        // GPS I2Cクロック（Wire1）
#define BTN_DISPLAY_PIN 11   // ディスプレイ切替ボタン
#define LED_ERROR_PIN 14     // エラーステータスLED
#define LED_PPS_PIN 15       // PPSパルスLED
#define LED_ONBOARD_PIN 25   // オンボードLED

// I2Cアドレス
#define SCREEN_ADDRESS 0x3C  // SH1106 OLED
// GPSモジュール: 0x42（u-bloxデフォルト）
// RTC: 0x68（DS3231）
```

### ライブラリ依存関係
- **SparkFun_u-blox_GNSS_Arduino_Library**: GPS/GNSS通信
- **WIZnet-ArduinoEthernet**: W5500イーサネットサポート
- **Adafruit-GFX-Library**: OLEDグラフィックプリミティブ
- **QZQSM/QZSSDCX**: QZSS L1S災害警報処理
- **uRTCLib**: RTC時刻管理

### 開発機能
- platformio.iniのデバッグフラグ:
  - `DEBUG_CONSOLE_GPS`: GPSステータスデバッグ
  - `DEBUG_CONSOLE_PPS`: PPS信号デバッグ
  - `DEBUG_CONSOLE_DCX_ALL`: QZSS災害警報デバッグ

### 特殊機能
- **QZSS L1S対応**: 日本のQZSS衛星からの災害・危機管理警報処理
- **マルチコンステレーションGNSS**: GPS、GLONASS、Galileo、BeiDou、QZSSサポート
- **高精度タイミング**: NTPサーバー精度のためのPPS信号処理
- **フェイルオーバータイミング**: GPS信号喪失時のRTCバックアップ

## コード構成

### コアクラス
- `GpsClient`: GPSデータ処理とQZSS処理
- `WebServer`: HTTPサーバーとメトリクス
- `GpsSummaryData`（Gps_model.h）: GPSデータ構造

### ハードウェア抽象化
- I2Cバス: Wire（OLED/RTC）とWire1（GPS）
- SPI: W5500イーサネットコントローラー
- GPIO: PPS入力、ステータスLED、ディスプレイボタン

### 時刻同期フロー
1. GPS/GNSSが衛星時刻を取得
2. PPS信号が精密タイミング割り込みをトリガー
3. GPS利用不可時はRTCが時刻を維持
4. NTPサーバーがネットワーク時刻サービスを提供
5. QZSS L1Sが災害警報を処理

## Claude向けメッセージ
### 必ず守るべきルール
- 常に資料にしたがう
- 仕様を変える場合はユーザーに確認してから、@requirements.md -> @design.md ->  @tasks.mdの順に修正する
- @tasks.md を進捗に沿って更新する
### 資料
- 要求書: @requirements.md
- 設計書: @design.md
- タスク: @tasks.md
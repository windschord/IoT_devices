# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 開発コマンド

### 統合Makefile（推奨）

プロジェクトには包括的なMakefileが含まれており、ビルドとデプロイメントを簡素化します：

```bash
# 完全デプロイ（ビルド + ファームウェア + ファイルシステム）
make full

# 基本コマンド
make build       # プロジェクトをビルド
make upload      # ファームウェアをアップロード
make uploadfs    # HTML/JSファイルをLittleFSにアップロード
make clean       # ビルド成果物をクリーンアップ
make monitor     # シリアルモニター開始 (9600 baud)

# 開発用コマンド
make test        # テスト実行
make check       # コードチェック（コンパイルのみ）
make rebuild     # クリーン + ビルド
make fs-check    # data/ディレクトリ内容確認

# オプション指定
make monitor BAUD=115200         # カスタムボーレート
make upload PORT=/dev/ttyACM0    # 特定ポート指定
```

### 従来のPlatformIOコマンド

必要に応じて直接PlatformIOコマンドも使用可能：

```bash
# プロジェクトをビルド
pio run -e pico

# Raspberry Pi Pico 2にビルドしてアップロード
pio run -e pico -t upload

# LittleFSファイルシステムアップロード
pio run -e pico -t uploadfs

# ビルド成果物をクリーンアップ
pio run -e pico -t clean
```

### テストとデバッグ
```bash
# Makefileベース（推奨）
make test           # テスト実行
make monitor        # シリアルモニター
make check          # コードチェック

# PlatformIO直接実行
pio test -e pico
pio device monitor -b 9600
pio check -e pico
```

### ライブラリ管理
```bash
# Makefileベース
make lib-update     # 全ライブラリ更新

# PlatformIO直接実行
pio lib install     # ライブラリのインストール/更新
pio lib list        # インストール済みライブラリの一覧
pio lib update      # 全ライブラリの更新
```

### デプロイメント手順

1. **完全デプロイ（推奨）**:
   ```bash
   make full
   ```

2. **段階的デプロイ（トラブルシューティング用）**:
   ```bash
   make build      # ビルド
   make upload     # ファームウェアアップロード
   make uploadfs   # Webファイルアップロード
   make monitor    # 動作確認
   ```

3. **開発時の高速デプロイ**:
   ```bash
   # コード変更後
   make upload     # ファームウェアのみ

   # Webファイル変更後
   make uploadfs   # ファイルシステムのみ
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
#define SCREEN_ADDRESS 0x3C  // SH1106 OLED (Wire0: GPIO 0/1)
// GPSモジュール: 0x42（u-bloxデフォルト）(Wire1: GPIO 6/7)
// RTC: 0x68（DS3231）(Wire1: GPIO 6/7)
// 注意: OLEDはWire0、GPS/RTCはWire1で分離
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

### ハードウェア拽象化
- I2Cバス: Wire0（OLED専用）、Wire1（GPS/RTC共有）
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
- 組み込みでリソースが限られるため、可能な限り処理を単純にして可読性の高いコードにする
- 個人で使うためセキュリティや高機能よりは、シンプルで保守性が高い設計や実装にする

### 資料
- 要求書: @requirements.md
- 設計書: @design.md
- タスク: @tasks.md
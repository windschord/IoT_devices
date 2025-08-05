# GPS NTP Server - 開発ガイド

## 概要

本ドキュメントは、GPS NTP Serverの開発・拡張・保守に関する包括的なガイドです。開発環境のセットアップから、コーディング規約、テスト戦略、デバッグ手法、機能拡張まで、開発者が必要とする全ての情報を提供します。

## 目次

1. [開発環境セットアップ](#開発環境セットアップ)
2. [プロジェクト構造](#プロジェクト構造)
3. [ビルドシステム](#ビルドシステム)
4. [コーディング規約](#コーディング規約)
5. [アーキテクチャガイド](#アーキテクチャガイド)
6. [デバッグとテスト](#デバッグとテスト)
7. [機能拡張ガイド](#機能拡張ガイド)
8. [パフォーマンス最適化](#パフォーマンス最適化)
9. [トラブルシューティング](#トラブルシューティング)
10. [リリース管理](#リリース管理)

## 開発環境セットアップ

### 必要なソフトウェア

#### 基本開発ツール

```bash
# PlatformIO CLI インストール (推奨)
pip install platformio

# または VS Code拡張を使用
# https://platformio.org/install/ide?install=vscode

# Git バージョン管理
git --version  # 2.30+ 推奨

# Python (ベンチマークツール用)
python3 --version  # 3.8+ 推奨
pip install requests json matplotlib
```

#### 開発支援ツール

```bash
# シリアル通信ツール
pip install pyserial

# JSON処理ツール
sudo apt install jq  # Linux
brew install jq      # macOS

# ネットワークテストツール
sudo apt install nmap curl netcat

# ハードウェアデバッグツール (オプション)
# OpenOCD, GDB, Logic Analyzer Software
```

### プロジェクトクローンとセットアップ

```bash
# リポジトリクローン
git clone <repository_url>
cd ntp_gps_pico2

# PlatformIO環境初期化
pio lib install

# 依存関係確認
pio lib list

# ビルドテスト
make build

# 設定ファイル確認
ls -la .vscode/
ls -la .pio/
```

### ハードウェアセットアップ

#### 必要なハードウェア

```
Development Hardware:
├─ Raspberry Pi Pico 2 (RP2350)
├─ SparkFun GNSS ZED-F9T Breakout
├─ W5500 Ethernet Module
├─ SH1106 OLED Display (128x64)
├─ DS3231 RTC Module (optional)
├─ Active GPS Antenna
├─ Ethernet Cable
├─ USB-C Cable
├─ Breadboard/Jumper Wires
└─ Power Supply (5V 2A)

Debug Tools (Optional):
├─ Logic Analyzer (8+ channels)
├─ Oscilloscope (>100MHz)
├─ Multimeter
├─ Function Generator
└─ Debugger Probe (Picoprobe)
```

#### 配線接続

```
Connection Matrix:
Raspberry Pi Pico 2    ←→    Module/Component
─────────────────────────────────────────────
GPIO 0 (I2C0 SDA)     ←→    SH1106 SDA
GPIO 1 (I2C0 SCL)     ←→    SH1106 SCL
GPIO 6 (I2C1 SDA)     ←→    ZED-F9T SDA, DS3231 SDA
GPIO 7 (I2C1 SCL)     ←→    ZED-F9T SCL, DS3231 SCL
GPIO 8                ←→    ZED-F9T PPS
GPIO 11               ←→    Reset Button (Pull-up)
GPIO 16 (SPI0 RX)     ←→    W5500 MISO
GPIO 17 (SPI0 CSn)    ←→    W5500 CS
GPIO 18 (SPI0 SCK)    ←→    W5500 SCLK
GPIO 19 (SPI0 TX)     ←→    W5500 MOSI
GPIO 20               ←→    W5500 RST
GPIO 21               ←→    W5500 INT
GPIO 4                ←→    Status LED 1 (GPS Fix)
GPIO 5                ←→    Status LED 2 (Network)
GPIO 14               ←→    Status LED 3 (Error)
GPIO 15               ←→    Status LED 4 (PPS)
3V3                   ←→    All modules VCC
GND                   ←→    All modules GND

Pull-up Resistors (4.7kΩ):
├─ I2C0: SDA, SCL
├─ I2C1: SDA, SCL
└─ Reset Button: GPIO 11 to 3V3
```

## プロジェクト構造

### ディレクトリ構成

```
ntp_gps_pico2/
├── docs/                      # ドキュメント
│   ├── user_manual.md         # ユーザーマニュアル
│   ├── api_specification.md   # API仕様書
│   ├── technical_specification.md  # 技術仕様書
│   └── hardware_interface_detailed.md  # HW詳細仕様
├── src/                       # ソースコード
│   ├── main.cpp              # メインプログラム
│   ├── gps/                  # GPS関連
│   │   ├── Gps_Client.cpp/h  # GPS制御クラス
│   │   └── Gps_model.h       # GPSデータ構造
│   ├── network/              # ネットワーク関連
│   │   ├── webserver.cpp/h   # Webサーバー
│   │   ├── NtpServer.cpp/h   # NTPサーバー
│   │   └── NetworkManager.cpp/h  # ネットワーク管理
│   ├── display/              # 表示関連
│   │   └── DisplayManager.cpp/h  # ディスプレイ制御
│   ├── config/               # 設定管理
│   │   └── ConfigManager.cpp/h   # 設定管理クラス
│   ├── logging/              # ログ管理
│   │   └── LoggingService.cpp/h  # ログサービス
│   ├── metrics/              # メトリクス
│   │   └── PrometheusMetrics.cpp/h  # Prometheus対応
│   ├── system/               # システム制御
│   │   ├── SystemController.cpp/h   # システム制御
│   │   ├── ErrorHandler.cpp/h       # エラー処理
│   │   └── SystemMonitor.cpp/h      # システム監視
│   └── utils/                # ユーティリティ
│       ├── TimeUtils.cpp/h   # 時刻処理ユーティリティ
│       ├── I2CUtils.cpp/h    # I2C通信ユーティリティ
│       └── LogUtils.cpp/h    # ログ処理ユーティリティ
├── test/                     # テストコード
│   ├── test_*.cpp           # ユニットテスト
│   ├── test_integration_*.cpp    # 統合テスト
│   ├── benchmark_*.py       # パフォーマンステスト
│   └── README_*.md          # テスト説明書
├── lib/                     # ローカルライブラリ
├── data/                    # Web ファイル (HTML/CSS/JS)
│   ├── index.html          # メイン Web ページ
│   ├── config.html         # 設定ページ
│   ├── style.css           # スタイルシート
│   └── script.js           # JavaScript
├── include/                 # ヘッダーファイル
├── .vscode/                # VS Code設定
├── platformio.ini          # PlatformIO設定
├── Makefile               # ビルド自動化
├── CLAUDE.md              # AI支援開発指示
├── design.md              # 設計文書
├── requirements.md        # 要求仕様
├── tasks.md               # 実装タスク
└── README.md              # プロジェクト概要
```

### ファイル命名規約

```
Naming Conventions:
├─ C++ Source Files: PascalCase.cpp (GpsClient.cpp)
├─ C++ Header Files: PascalCase.h (GpsClient.h)
├─ Test Files: test_snake_case.cpp (test_gps_client.cpp)
├─ Python Scripts: snake_case.py (benchmark_performance.py)
├─ Documentation: snake_case.md (user_manual.md)
├─ Web Files: lowercase.html (config.html)
├─ Constants Files: UPPERCASE.h (CONFIG_CONSTANTS.h)
└─ Utility Files: PascalCase.cpp/h (TimeUtils.cpp/h)
```

## ビルドシステム

### PlatformIO設定 (platformio.ini)

```ini
[env:pico]
platform = https://github.com/maxgerhardt/platform-raspberrypi.git
board = pico2
framework = arduino

; ライブラリ依存関係
lib_deps = 
    sparkfun/SparkFun u-blox GNSS Arduino Library @ ^2.2.27
    adafruit/Adafruit GFX Library @ ^1.11.5
    bblanchon/ArduinoJson @ ^6.21.3

; ビルドフラグ
build_flags = 
    -DPIO_FRAMEWORK_ARDUINO_ENABLE_CDC
    -DBOARD_NAME="Raspberry Pi Pico 2"
    -DARDUINO_ARCH_RP2040
    -DUSE_ARDUINO_NATIVE_I2C
    
; デバッグ設定
debug_tool = cmsis-dap
debug_init_break = tbreak setup

; アップロード設定
upload_protocol = picotool
upload_port = auto

; モニター設定
monitor_speed = 115200
monitor_filters = send_on_enter
```

### Makefile自動化

```makefile
# GPS NTP Server Makefile

# デフォルトターゲット
.DEFAULT_GOAL := help

# プロジェクト設定
PROJECT_NAME = gps_ntp_server
VERSION = 1.0.0
BUILD_ENV = pico

# PlatformIO コマンド
PIO = pio

# 基本コマンド ------------------------------------------

.PHONY: help
help:  ## ヘルプ表示
	@echo "GPS NTP Server Build System"
	@echo "Usage: make [target]"
	@echo ""
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | \
		awk 'BEGIN {FS = ":.*?## "}; {printf "  %-15s %s\n", $$1, $$2}'

.PHONY: build
build:  ## プロジェクトビルド
	@echo "Building $(PROJECT_NAME) v$(VERSION)..."
	$(PIO) run -e $(BUILD_ENV)
	@echo "Build completed successfully!"

.PHONY: clean
clean:  ## ビルドクリーンアップ
	@echo "Cleaning build artifacts..."
	$(PIO) run -e $(BUILD_ENV) -t clean
	rm -rf .pio/build/
	@echo "Clean completed!"

.PHONY: rebuild
rebuild: clean build  ## クリーン後リビルド

# アップロード ------------------------------------------

.PHONY: upload
upload:  ## ファームウェアアップロード
	@echo "Uploading firmware..."
	$(PIO) run -e $(BUILD_ENV) -t upload
	@echo "Upload completed!"

.PHONY: uploadfs
uploadfs:  ## ファイルシステムアップロード
	@echo "Uploading filesystem..."
	$(PIO) run -e $(BUILD_ENV) -t uploadfs
	@echo "Filesystem upload completed!"

.PHONY: full
full: build upload uploadfs  ## 完全デプロイ (ビルド+アップロード+FS)

# 開発支援 ------------------------------------------

.PHONY: monitor
monitor:  ## シリアルモニター開始
	$(PIO) device monitor -b 115200

.PHONY: check
check:  ## コードチェック (コンパイルのみ)
	@echo "Performing code check..."
	$(PIO) check -e $(BUILD_ENV)

.PHONY: test
test:  ## テスト実行
	@echo "Running tests..."
	$(PIO) test -e $(BUILD_ENV)

# ライブラリ管理 ------------------------------------

.PHONY: lib-list
lib-list:  ## インストール済みライブラリ一覧
	$(PIO) lib list

.PHONY: lib-update
lib-update:  ## 全ライブラリ更新
	$(PIO) lib update

.PHONY: lib-install
lib-install:  ## ライブラリインストール/更新
	$(PIO) lib install

# 情報表示 ------------------------------------------

.PHONY: size
size:  ## バイナリサイズ表示
	$(PIO) run -e $(BUILD_ENV) -t size

.PHONY: env
env:  ## 環境情報表示
	$(PIO) system info
	$(PIO) platform show raspberrypi

.PHONY: fs-check
fs-check:  ## data/ディレクトリ内容確認
	@echo "Filesystem contents:"
	@ls -la data/ 2>/dev/null || echo "data/ directory not found"

# 高度なターゲット ----------------------------------

.PHONY: benchmark
benchmark:  ## パフォーマンスベンチマーク実行
	@echo "Running performance benchmark..."
	python3 test/benchmark_system_performance.py

.PHONY: format
format:  ## コードフォーマット
	@echo "Formatting code..."
	find src/ -name "*.cpp" -o -name "*.h" | xargs clang-format -i
	@echo "Code formatting completed!"

.PHONY: docs
docs:  ## ドキュメント生成
	@echo "Generating documentation..."
	doxygen Doxyfile 2>/dev/null || echo "Doxygen not found, skipping..."

# カスタムポート/ボーレート指定
MONITOR_PORT ?= auto
MONITOR_BAUD ?= 115200

.PHONY: monitor-custom
monitor-custom:  ## カスタムポート/ボーレートでモニター
	$(PIO) device monitor -p $(MONITOR_PORT) -b $(MONITOR_BAUD)

# バージョン管理
.PHONY: version
version:  ## バージョン情報表示
	@echo "$(PROJECT_NAME) v$(VERSION)"
	@echo "Build environment: $(BUILD_ENV)"
	@echo "PlatformIO version: $(shell $(PIO) --version)"
```

### ビルド自動化スクリプト

```bash
#!/bin/bash
# build_script.sh - 自動ビルドスクリプト

set -e  # エラー時に終了

PROJECT_NAME="GPS NTP Server"
VERSION="1.0.0"
BUILD_ENV="pico"

# 色付き出力
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_header() {
    echo -e "${BLUE}================================${NC}"
    echo -e "${BLUE}  $PROJECT_NAME Build Script${NC}"
    echo -e "${BLUE}  Version: $VERSION${NC}"
    echo -e "${BLUE}================================${NC}"
}

print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 前提条件チェック
check_prerequisites() {
    print_status "Checking prerequisites..."
    
    if ! command -v pio &> /dev/null; then
        print_error "PlatformIO CLI not found!"
        exit 1
    fi
    
    if ! [ -f "platformio.ini" ]; then
        print_error "platformio.ini not found!"
        exit 1
    fi
    
    print_status "Prerequisites OK"
}

# ライブラリ依存関係チェック
check_libraries() {
    print_status "Checking library dependencies..."
    pio lib install
    print_status "Libraries OK"
}

# ビルド実行
build_project() {
    print_status "Building project..."
    pio run -e $BUILD_ENV
    
    if [ $? -eq 0 ]; then
        print_status "Build successful!"
    else
        print_error "Build failed!"
        exit 1
    fi
}

# サイズ情報表示
show_size_info() {
    print_status "Binary size information:"
    pio run -e $BUILD_ENV -t size
}

# テスト実行
run_tests() {
    print_status "Running tests..."
    pio test -e $BUILD_ENV --verbose
    
    if [ $? -eq 0 ]; then
        print_status "All tests passed!"
    else
        print_error "Some tests failed!"
        exit 1
    fi
}

# メイン処理
main() {
    print_header
    check_prerequisites
    check_libraries
    build_project
    show_size_info
    
    # テスト実行 (オプション)
    if [ "$1" = "--with-tests" ]; then
        run_tests
    fi
    
    print_status "Build process completed successfully!"
}

# スクリプト実行
main "$@"
```

## コーディング規約

### 命名規則

#### C++ 命名規則

```cpp
// クラス名: PascalCase
class SystemController {};
class GpsClient {};
class NetworkManager {};

// メソッド名: camelCase  
void initializeSystem();
bool isSystemHealthy();
uint32_t getCurrentTime();

// 変数名: snake_case
uint32_t system_uptime;
bool gps_fix_valid;
char hostname[64];

// 定数名: UPPER_SNAKE_CASE
const uint16_t DEFAULT_NTP_PORT = 123;
constexpr size_t MAX_BUFFER_SIZE = 4096;
static const char* DEVICE_NAME = "GPS-NTP-Server";

// 列挙型: PascalCase (値はUPPER_SNAKE_CASE)
enum class SystemState {
    INITIALIZING,
    STARTUP,
    RUNNING,
    DEGRADED,
    ERROR,
    RECOVERY,
    SHUTDOWN
};

// 構造体: PascalCase (メンバーはsnake_case)
struct GpsSummaryData {
    uint32_t utc_time;
    bool fix_valid;
    uint8_t satellites_used;
};

// マクロ: UPPER_SNAKE_CASE
#define GPS_TIMEOUT_MS 30000
#define LOG_BUFFER_SIZE 4096

// 名前空間: PascalCase
namespace HAL {
    class I2CManager {};
}
```

#### ファイル命名規則

```
C++ Files:
├─ Header Files: PascalCase.h
│  └─ Example: GpsClient.h, SystemController.h
├─ Source Files: PascalCase.cpp  
│  └─ Example: GpsClient.cpp, SystemController.cpp
├─ Test Files: test_snake_case.cpp
│  └─ Example: test_gps_client.cpp, test_ntp_server.cpp
└─ Utility Files: PascalCaseUtils.h/cpp
   └─ Example: TimeUtils.h, I2CUtils.cpp

Documentation Files:
├─ Markdown: snake_case.md
│  └─ Example: user_manual.md, api_specification.md
├─ Configuration: lowercase or snake_case
│  └─ Example: platformio.ini, Makefile
└─ Web Files: lowercase.html
   └─ Example: index.html, config.html
```

### コードフォーマット

#### インデント・空白

```cpp
class ExampleClass {
public:  // アクセス指定子: インデントなし
    ExampleClass();  // public メンバー: 4スペースインデント
    ~ExampleClass();
    
    bool initialize();
    void update();
    
private:
    void privateMethod();  // private メンバー: 4スペースインデント
    
    uint32_t member_variable_;  // メンバー変数: 末尾アンダースコア
    bool is_initialized_;
};

void ExampleClass::publicMethod() {
    if (condition) {  // 制御構文: 4スペースインデント
        doSomething();
        
        for (int i = 0; i < count; ++i) {  // ネスト: さらに4スペース
            processItem(i);
        }
    }
}
```

#### 括弧・改行スタイル

```cpp
// Allman スタイル (推奨)
class ClassName 
{
public:
    void methodName()
    {
        if (condition)
        {
            // 処理
        }
        else
        {
            // 別の処理
        }
    }
};

// 1つの文の場合は括弧省略可能
if (simple_condition)
    simple_action();

// 複雑な条件では括弧必須
if (complex_condition || 
    another_condition)
{
    complex_action();
}
```

#### 行長・空行

```cpp
// 行長: 100文字以内を推奨
const char* long_string = 
    "This is a very long string that needs to be broken "
    "across multiple lines for better readability";

// 関数間: 2行空ける
void function1()
{
    // 実装
}


void function2()  // 2行空行
{
    // 実装
}

// ブロック内: 論理グループ間に1行
void complexFunction()
{
    // 初期化処理
    initialize();
    setupResources();
    
    // メイン処理 (1行空行)
    processData();
    calculateResults();
    
    // クリーンアップ処理 (1行空行)
    cleanup();
}
```

### コメント規約

#### ドキュメントコメント (Doxygen形式)

```cpp
/**
 * @brief GPS時刻同期システムを管理するクラス
 * 
 * このクラスは、GPS受信機からの時刻情報とPPS信号を使用して
 * システム時刻を高精度に同期します。GPS信号が利用できない場合は
 * 内部RTCにフォールバックします。
 * 
 * @author GPS NTP Server Development Team
 * @version 1.0.0
 * @date 2025-07-30
 * 
 * @example
 * ```cpp
 * GpsClient gps;
 * if (gps.initialize()) {
 *     gps.startSynchronization();
 * }
 * ```
 */
class GpsClient
{
public:
    /**
     * @brief GPS クライアントを初期化
     * 
     * I2C通信を設定し、GPS受信機との接続を確立します。
     * 初期化に失敗した場合、詳細なエラー情報がログに記録されます。
     * 
     * @return true: 初期化成功, false: 初期化失敗
     * 
     * @pre I2C1バスが利用可能であること
     * @post GPS受信機との通信が確立される
     * 
     * @warning この関数は setup() 内で一度だけ呼び出してください
     * @note 初期化には最大5秒かかる場合があります
     * 
     * @see startSynchronization(), getGpsStatus()
     */
    bool initialize();
    
    /**
     * @brief GPS データを更新
     * 
     * GPS受信機から最新のナビゲーションデータを取得し、
     * 内部状態を更新します。
     * 
     * @param force_update 強制更新フラグ (default: false)
     * @return GpsSummaryData 最新のGPS情報
     * 
     * @retval valid データが有効な場合
     * @retval invalid GPS信号が利用できない場合
     * 
     * @throw std::runtime_error I2C通信エラー時
     * 
     * @par Performance
     * この関数の実行時間は通常100ms以下です。
     * 
     * @par Thread Safety
     * この関数はスレッドセーフではありません。
     * 同時呼び出しを避けてください。
     */
    GpsSummaryData updateGpsData(bool force_update = false);

private:
    /**
     * @brief PPS信号割り込みハンドラ
     * 
     * @param timestamp PPS信号検出時刻 (マイクロ秒)
     */
    void handlePpsInterrupt(uint64_t timestamp);
    
    uint32_t last_update_time_;  ///< 最終更新時刻 (Unix timestamp)
    bool is_synchronized_;       ///< 同期状態フラグ
};
```

#### インラインコメント

```cpp
void processGpsData()
{
    // GPS データの妥当性チェック
    if (!gps_data.fix_valid) {
        return;  // 無効なデータは処理しない
    }
    
    // 時刻精度チェック (50ns以内)
    if (gps_data.time_accuracy > 50) {
        LOG_WARNING("GPS", "Time accuracy degraded: %uns", 
                   gps_data.time_accuracy);
    }
    
    /* 
     * マルチコンステレーション処理
     * 各衛星システムの信号を個別に評価し、
     * 最適な組み合わせを選択する
     */
    uint8_t total_satellites = 
        gps_data.satellites_gps +      // GPS (アメリカ)
        gps_data.satellites_glonass +  // GLONASS (ロシア) 
        gps_data.satellites_galileo +  // Galileo (EU)
        gps_data.satellites_beidou +   // BeiDou (中国)
        gps_data.satellites_qzss;      // QZSS (日本)
    
    // TODO: L1S災害警報処理を実装
    // FIXME: BeiDou衛星でタイムアウト発生中
    // HACK: 一時的な回避策 - 要リファクタリング
    // NOTE: この処理は毎秒1回実行される
}
```

#### ヘッダーコメント

```cpp
/**
 * @file GpsClient.h
 * @brief GPS/GNSS受信機制御クラス
 * 
 * SparkFun u-blox ZED-F9T GPS受信機を制御し、高精度な時刻同期を
 * 提供するクラスの定義。マルチコンステレーション対応、PPS信号処理、
 * QZSS L1S災害警報機能を含む。
 * 
 * @details
 * 本ファイルで定義されるGpsClientクラスは、以下の機能を提供：
 * - GPS/GLONASS/Galileo/BeiDou/QZSS衛星からの時刻取得
 * - PPS (Pulse Per Second) 信号による高精度同期
 * - QZSS L1S信号からの災害・危機管理通報受信
 * - 衛星信号品質監視と自動フォールバック
 * 
 * @author GPS NTP Server Development Team
 * @date 2025-07-30
 * @version 1.0.0
 * 
 * @copyright Copyright (c) 2025 GPS NTP Server Project
 * @license MIT License
 * 
 * @see https://github.com/sparkfun/SparkFun_u-blox_GNSS_Arduino_Library
 * @see design.md システム設計書
 * @see requirements.md 要求仕様書
 */

#ifndef GPS_CLIENT_H
#define GPS_CLIENT_H

// Standard includes
#include <stdint.h>
#include <stdbool.h>

// Project includes  
#include "Gps_model.h"
#include "../utils/TimeUtils.h"

// External library includes
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
```

### エラーハンドリング規約

#### 例外処理

```cpp
// エラーコード定義
enum class GpsError : uint8_t {
    SUCCESS = 0,
    HARDWARE_NOT_FOUND = 1,
    COMMUNICATION_ERROR = 2,
    TIMEOUT = 3,
    INVALID_DATA = 4,
    INSUFFICIENT_SATELLITES = 5
};

// 戻り値によるエラー処理 (推奨)
GpsError GpsClient::initialize()
{
    // ハードウェア検出
    if (!detectHardware()) {
        LOG_ERROR("GPS", "Hardware detection failed");
        return GpsError::HARDWARE_NOT_FOUND;
    }
    
    // 通信テスト
    if (!testCommunication()) {
        LOG_ERROR("GPS", "Communication test failed");
        return GpsError::COMMUNICATION_ERROR;
    }
    
    return GpsError::SUCCESS;
}

// 使用例
void setup()
{
    GpsError result = gps_client.initialize();
    if (result != GpsError::SUCCESS) {
        handleGpsError(result);
        // フォールバック処理
        initializeRtcFallback();
    }
}
```

#### ログ出力規約

```cpp
// ログレベル別使用ガイドライン
void demonstrateLogging()
{
    // DEBUG: 開発時の詳細情報
    LOG_DEBUG("GPS", "Processing satellite data: PRN=%d, C/N0=%d", 
              sat_prn, signal_strength);
    
    // INFO: 一般的な動作情報
    LOG_INFO("SYSTEM", "GPS NTP Server started successfully");
    LOG_INFO("NETWORK", "IP address assigned: %s", ip_address);
    
    // NOTICE: 重要な状態変化
    LOG_NOTICE("GPS", "GPS fix acquired: 3D fix with %d satellites", 
               satellite_count);
    
    // WARNING: 潜在的な問題
    LOG_WARNING("MEMORY", "Memory usage high: %d%% used", 
                memory_usage_percent);
    
    // ERROR: 処理続行可能なエラー
    LOG_ERROR("NETWORK", "Failed to send syslog message: %s", 
              error_message);
    
    // CRITICAL: 処理続行困難なエラー
    LOG_CRITICAL("SYSTEM", "Hardware failure detected: %s", 
                 hardware_component);
}

// ログメッセージフォーマット
// [YYYY-MM-DD HH:MM:SS.mmm] [LEVEL] [COMPONENT] Message
// 例: [2025-07-30 12:34:56.789] [INFO] [GPS] 3D fix acquired
```

## アーキテクチャガイド

### システムアーキテクチャ概要

#### 階層構造

```
Application Layer (アプリケーション層)
├─ NtpServer: NTPプロトコル処理
├─ WebServer: HTTP API処理  
├─ PrometheusMetrics: メトリクス収集
└─ DisplayManager: UI表示制御

Service Layer (サービス層)
├─ TimeService: 時刻管理サービス
├─ LoggingService: ログ管理サービス
├─ ConfigManager: 設定管理サービス
└─ NetworkManager: ネットワーク管理

HAL Layer (ハードウェア抽象化層)
├─ GpsHAL: GPS通信抽象化
├─ NetworkHAL: ネットワーク抽象化
├─ DisplayHAL: 表示制御抽象化
├─ StorageHAL: ストレージ抽象化
└─ ButtonHAL: ボタン制御抽象化

Hardware Layer (ハードウェア層)
├─ ZED-F9T: GPS/GNSS受信機
├─ W5500: イーサネットコントローラ
├─ SH1106: OLEDディスプレイ
├─ RP2350: マイクロコントローラ
└─ Peripherals: LED、ボタン、RTC
```

#### 依存関係の原則

```cpp
/**
 * Dependency Inversion Principle (依存関係逆転の原則)
 * 
 * 上位レイヤーは下位レイヤーに直接依存せず、
 * 抽象インターフェースを通じて依存する
 */

// 悪い例: 直接依存
class NtpServer {
private:
    GpsClient gps_client;  // 具体クラスに直接依存
public:
    void updateTime() {
        auto gps_data = gps_client.getData();  // 密結合
        // 処理...
    }
};

// 良い例: 依存関係注入
class TimeProvider {  // 抽象インターフェース
public:
    virtual ~TimeProvider() = default;
    virtual TimeData getCurrentTime() = 0;
    virtual bool isTimeValid() = 0;
};

class GpsTimeProvider : public TimeProvider {  // 具象実装
public:
    TimeData getCurrentTime() override {
        // GPS実装
    }
    bool isTimeValid() override {
        // GPS固有の検証
    }
};

class NtpServer {
private:
    TimeProvider* time_provider_;  // 抽象インターフェースに依存
public:
    NtpServer(TimeProvider* provider) : time_provider_(provider) {}
    
    void updateTime() {
        if (time_provider_->isTimeValid()) {
            auto time_data = time_provider_->getCurrentTime();
            // 処理...
        }
    }
};
```

### 設計パターン

#### Singleton パターン (システム設定)

```cpp
/**
 * @brief システム設定管理のSingletonクラス
 * 
 * システム全体で唯一の設定インスタンスを提供。
 * スレッドセーフな実装。
 */
class ConfigManager {
public:
    // スレッドセーフなSingleton取得
    static ConfigManager& getInstance() {
        static ConfigManager instance;  // C++11以降はスレッドセーフ
        return instance;
    }
    
    // コピー・移動を禁止
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    ConfigManager(ConfigManager&&) = delete;
    ConfigManager& operator=(ConfigManager&&) = delete;
    
    // 設定アクセスメソッド
    bool loadConfig();
    bool saveConfig();
    const SystemConfig& getConfig() const { return config_; }
    
private:
    ConfigManager() = default;  // プライベートコンストラクタ
    SystemConfig config_;
    mutable std::mutex config_mutex_;  // 組み込みではmutexは軽量実装
};

// 使用例
void someFunction() {
    auto& config = ConfigManager::getInstance();
    if (config.getConfig().network.dhcp_enabled) {
        // DHCP処理
    }
}
```

#### Observer パターン (システム状態通知)

```cpp
/**
 * @brief システム状態変化の通知インターフェース
 */
class SystemObserver {
public:
    virtual ~SystemObserver() = default;
    virtual void onSystemStateChanged(SystemState new_state) = 0;
    virtual void onErrorOccurred(ErrorCode error) = 0;
};

/**
 * @brief Observable システム制御クラス
 */
class SystemController {
private:
    std::vector<SystemObserver*> observers_;
    SystemState current_state_;
    
public:
    void addObserver(SystemObserver* observer) {
        observers_.push_back(observer);
    }
    
    void removeObserver(SystemObserver* observer) {
        observers_.erase(
            std::remove(observers_.begin(), observers_.end(), observer),
            observers_.end()
        );
    }
    
    void setState(SystemState new_state) {
        if (current_state_ != new_state) {
            current_state_ = new_state;
            notifyStateChanged(new_state);
        }
    }
    
private:
    void notifyStateChanged(SystemState state) {
        for (auto* observer : observers_) {
            observer->onSystemStateChanged(state);
        }
    }
    
    void notifyError(ErrorCode error) {
        for (auto* observer : observers_) {
            observer->onErrorOccurred(error);
        }
    }
};

// 具象Observer実装例
class DisplayManager : public SystemObserver {
public:
    void onSystemStateChanged(SystemState new_state) override {
        switch (new_state) {
            case SystemState::RUNNING:
                showNormalDisplay();
                break;
            case SystemState::ERROR:
                showErrorDisplay();
                break;
            // 他の状態...
        }
    }
    
    void onErrorOccurred(ErrorCode error) override {
        showErrorMessage(error);
    }
};
```

#### Factory パターン (サービス生成)

```cpp
/**
 * @brief サービス基底クラス
 */
class ServiceBase {
public:
    virtual ~ServiceBase() = default;
    virtual bool initialize() = 0;
    virtual void update() = 0;
    virtual const char* getName() const = 0;
};

/**
 * @brief サービスファクトリ
 */
class ServiceFactory {
public:
    enum class ServiceType {
        GPS_CLIENT,
        NTP_SERVER,
        WEB_SERVER,
        DISPLAY_MANAGER,
        CONFIG_MANAGER,
        LOGGING_SERVICE,
        PROMETHEUS_METRICS,
        NETWORK_MANAGER
    };
    
    static std::unique_ptr<ServiceBase> createService(ServiceType type) {
        switch (type) {
            case ServiceType::GPS_CLIENT:
                return std::make_unique<GpsClient>();
            case ServiceType::NTP_SERVER:
                return std::make_unique<NtpServer>();
            case ServiceType::WEB_SERVER:
                return std::make_unique<WebServer>();
            case ServiceType::DISPLAY_MANAGER:
                return std::make_unique<DisplayManager>();
            case ServiceType::CONFIG_MANAGER:
                return std::make_unique<ConfigManager>();
            case ServiceType::LOGGING_SERVICE:
                return std::make_unique<LoggingService>();
            case ServiceType::PROMETHEUS_METRICS:
                return std::make_unique<PrometheusMetrics>();
            case ServiceType::NETWORK_MANAGER:
                return std::make_unique<NetworkManager>();
            default:
                return nullptr;
        }
    }
};

// 使用例
void initializeServices() {
    std::vector<std::unique_ptr<ServiceBase>> services;
    
    // 必要なサービスを動的生成
    services.push_back(ServiceFactory::createService(ServiceFactory::ServiceType::GPS_CLIENT));
    services.push_back(ServiceFactory::createService(ServiceFactory::ServiceType::NTP_SERVER));
    services.push_back(ServiceFactory::createService(ServiceFactory::ServiceType::WEB_SERVER));
    
    // 全サービス初期化
    for (auto& service : services) {
        if (!service->initialize()) {
            LOG_ERROR("SYSTEM", "Failed to initialize service: %s", 
                     service->getName());
        }
    }
}
```

### メモリ管理

#### 静的メモリ割り当て (推奨)

```cpp
/**
 * @brief 組み込みシステム向け静的メモリ管理
 * 
 * 動的メモリ割り当て (new/delete, malloc/free) は避け、
 * コンパイル時に決定される静的配列を使用
 */

// 悪い例: 動的メモリ割り当て
class BadExample {
private:
    char* buffer_;
    size_t buffer_size_;
    
public:
    BadExample(size_t size) : buffer_size_(size) {
        buffer_ = new char[size];  // メモリリークリスク
    }
    
    ~BadExample() {
        delete[] buffer_;  // 削除し忘れリスク
    }
};

// 良い例: 静的メモリ割り当て
template<size_t BufferSize>
class GoodExample {
private:
    char buffer_[BufferSize];  // コンパイル時に決定
    size_t used_size_;
    
public:
    GoodExample() : used_size_(0) {
        // 動的割り当て不要
    }
    
    constexpr size_t capacity() const { return BufferSize; }
    size_t size() const { return used_size_; }
    char* data() { return buffer_; }
};

// 使用例: テンプレート特殊化
using LogBuffer = GoodExample<4096>;
using JsonBuffer = GoodExample<8192>;
using ConfigBuffer = GoodExample<2048>;
```

#### スマートポインタ (制限的使用)

```cpp
/**
 * @brief リソース管理にはRAII原則を適用
 * 
 * unique_ptr は使用可能だが、shared_ptr は避ける
 * (参照カウンタのオーバーヘッドのため)
 */

// RAII を活用したリソース管理
class I2CTransaction {
private:
    uint8_t device_address_;
    bool is_active_;
    
public:
    I2CTransaction(uint8_t address) 
        : device_address_(address), is_active_(false) {
        if (acquireI2CBus(address)) {
            is_active_ = true;
        }
    }
    
    ~I2CTransaction() {
        if (is_active_) {
            releaseI2CBus(device_address_);
        }
    }
    
    // コピー禁止、移動許可
    I2CTransaction(const I2CTransaction&) = delete;
    I2CTransaction& operator=(const I2CTransaction&) = delete;
    I2CTransaction(I2CTransaction&&) = default;
    I2CTransaction& operator=(I2CTransaction&&) = default;
    
    bool isActive() const { return is_active_; }
};

// 使用例: 自動リソース管理
bool readGpsData() {
    I2CTransaction transaction(GPS_I2C_ADDRESS);
    if (!transaction.isActive()) {
        return false;
    }
    
    // I2C操作実行
    // スコープ終了時に自動的にバス解放
    return performI2CRead();
}
```

## デバッグとテスト

### デバッグ戦略

#### ログベースデバッグ

```cpp
/**
 * @brief デバッグレベル設定
 */
void setDebugLevel() {
    #ifdef DEBUG_BUILD
        LoggingService::getInstance().setLogLevel(LogLevel::DEBUG);
    #else
        LoggingService::getInstance().setLogLevel(LogLevel::INFO);
    #endif
}

/**
 * @brief 条件付きデバッグマクロ
 */
#ifdef DEBUG_GPS
    #define GPS_DEBUG(fmt, ...) LOG_DEBUG("GPS", fmt, ##__VA_ARGS__)
#else
    #define GPS_DEBUG(fmt, ...) do {} while(0)
#endif

#ifdef DEBUG_NTP
    #define NTP_DEBUG(fmt, ...) LOG_DEBUG("NTP", fmt, ##__VA_ARGS__)
#else
    #define NTP_DEBUG(fmt, ...) do {} while(0)
#endif

// 使用例
void processNtpRequest() {
    NTP_DEBUG("Processing NTP request from client");
    
    // パケット内容のダンプ
    NTP_DEBUG("Packet dump:");
    for (int i = 0; i < packet_size; ++i) {
        NTP_DEBUG("  [%02d]: 0x%02X", i, packet_data[i]);
    }
}
```

#### アサーション

```cpp
/**
 * @brief カスタムアサーションマクロ
 */
#ifdef DEBUG_BUILD
    #define ASSERT(condition, message) \
        do { \
            if (!(condition)) { \
                LOG_CRITICAL("ASSERT", "Assertion failed: %s at %s:%d", \
                           message, __FILE__, __LINE__); \
                while(1) { /* システム停止 */ } \
            } \
        } while(0)
#else
    #define ASSERT(condition, message) do {} while(0)
#endif

// 使用例
void processGpsData(const GpsSummaryData& data) {
    ASSERT(data.satellites_used <= data.satellites_total, 
           "Used satellites cannot exceed total satellites");
    
    ASSERT(data.fix_type >= 0 && data.fix_type <= 5,
           "Invalid GPS fix type");
    
    // 処理続行
}
```

#### パフォーマンス測定

```cpp
/**
 * @brief パフォーマンス測定ユーティリティ
 */
class PerformanceTimer {
private:
    uint32_t start_time_;
    const char* operation_name_;
    
public:
    PerformanceTimer(const char* name) 
        : operation_name_(name) {
        start_time_ = micros();
    }
    
    ~PerformanceTimer() {
        uint32_t elapsed = micros() - start_time_;
        LOG_DEBUG("PERF", "%s took %u microseconds", 
                 operation_name_, elapsed);
    }
};

// マクロ定義
#ifdef ENABLE_PERFORMANCE_MONITORING
    #define PERF_TIMER(name) PerformanceTimer _timer(name)
#else
    #define PERF_TIMER(name) do {} while(0)
#endif

// 使用例
void complexGpsProcessing() {
    PERF_TIMER("GPS Processing");
    
    // 処理実行
    parseGpsMessages();
    calculatePosition();
    updateTimeReference();
    
    // デストラクタで自動的に実行時間ログ出力
}
```

### テスト戦略

#### ユニットテスト構造

```cpp
/**
 * @file test_gps_client.cpp
 * @brief GPS Client ユニットテスト
 */

#include <unity.h>
#include "../src/gps/Gps_Client.h"

// テストフィクスチャ
class GpsClientTest {
public:
    static void SetUp() {
        // テスト前の初期化
    }
    
    static void TearDown() {
        // テスト後のクリーンアップ
    }
};

// テストケース実装
void test_gps_initialization_success() {
    // Arrange
    GpsClient gps_client;
    
    // Act
    bool result = gps_client.initialize();
    
    // Assert
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(gps_client.isInitialized());
}

void test_gps_data_parsing() {
    // Arrange
    GpsClient gps_client;
    gps_client.initialize();
    
    // Mock GPS データ
    const char* mock_ubx_data = "sample_ubx_packet";
    
    // Act
    GpsSummaryData result = gps_client.parseGpsData(mock_ubx_data);
    
    // Assert
    TEST_ASSERT_EQUAL(3, result.fix_type);  // 3D Fix
    TEST_ASSERT_GREATER_THAN(0, result.satellites_used);
}

void test_pps_signal_processing() {
    // Arrange
    GpsClient gps_client;
    uint64_t mock_timestamp = 1000000;  // 1秒
    
    // Act
    gps_client.processPpsSignal(mock_timestamp);
    
    // Assert
    TEST_ASSERT_TRUE(gps_client.isPpsActive());
    TEST_ASSERT_EQUAL(mock_timestamp, gps_client.getLastPpsTime());
}

// テストメイン関数
void setUp(void) {
    GpsClientTest::SetUp();
}

void tearDown(void) {
    GpsClientTest::TearDown();
}

int main() {
    UNITY_BEGIN();
    
    // GPS基本機能テスト
    RUN_TEST(test_gps_initialization_success);
    RUN_TEST(test_gps_data_parsing);
    RUN_TEST(test_pps_signal_processing);
    
    return UNITY_END();
}
```

#### モックオブジェクト

```cpp
/**
 * @brief I2C通信のモック実装
 */
class MockI2CManager : public I2CManager {
private:
    std::map<uint8_t, std::vector<uint8_t>> mock_registers_;
    bool communication_should_fail_;
    
public:
    MockI2CManager() : communication_should_fail_(false) {}
    
    // モック動作設定
    void setRegisterValue(uint8_t address, uint8_t reg, uint8_t value) {
        mock_registers_[reg].push_back(value);
    }
    
    void setCommunicationFailure(bool should_fail) {
        communication_should_fail_ = should_fail;
    }
    
    // モック実装
    bool writeRegister(uint8_t address, uint8_t reg, uint8_t data) override {
        if (communication_should_fail_) return false;
        
        mock_registers_[reg] = {data};
        return true;
    }
    
    bool readRegister(uint8_t address, uint8_t reg, uint8_t& data) override {
        if (communication_should_fail_) return false;
        
        if (mock_registers_.find(reg) != mock_registers_.end()) {
            data = mock_registers_[reg][0];
            return true;
        }
        return false;
    }
};

// モックを使用したテスト
void test_gps_communication_failure() {
    // Arrange
    MockI2CManager mock_i2c;
    mock_i2c.setCommunicationFailure(true);
    
    GpsClient gps_client(&mock_i2c);  // 依存関係注入
    
    // Act
    bool result = gps_client.initialize();
    
    // Assert
    TEST_ASSERT_FALSE(result);
}
```

#### 統合テスト

```cpp
/**
 * @file test_integration_system.cpp
 * @brief システム統合テスト
 */

void test_gps_to_ntp_integration() {
    // システム全体の初期化
    SystemController system;
    TEST_ASSERT_TRUE(system.initialize());
    
    // GPS信号取得をシミュレート
    simulateGpsSignal();
    delay(5000);  // GPS Fix待機
    
    // NTPリクエストを送信
    NtpPacket request = createNtpRequest();
    NtpPacket response = system.processNtpRequest(request);
    
    // NTP応答検証
    TEST_ASSERT_EQUAL(1, response.stratum);  // GPS同期時はStratum 1
    TEST_ASSERT_NOT_EQUAL(0, response.transmit_timestamp_s);
}

void test_web_interface_integration() {
    // Webサーバー初期化
    WebServer web_server;
    TEST_ASSERT_TRUE(web_server.initialize());
    
    // 設定変更APIテスト
    const char* json_config = 
        "{\"network\":{\"hostname\":\"test-server\"}}";
    
    HttpResponse response = web_server.processConfigUpdate(json_config);
    
    // 応答検証
    TEST_ASSERT_EQUAL(200, response.status_code);
    TEST_ASSERT_TRUE(strstr(response.body, "success") != nullptr);
}
```

### テスト自動化

#### Continuous Integration Setup

```yaml
# .github/workflows/ci.yml
name: GPS NTP Server CI

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]

jobs:
  build-and-test:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Set up Python
      uses: actions/setup-python@v3
      with:
        python-version: '3.9'
    
    - name: Install PlatformIO
      run: |
        python -m pip install --upgrade pip
        pip install platformio
    
    - name: Install dependencies
      run: pio lib install
    
    - name: Build firmware
      run: pio run -e pico
    
    - name: Run unit tests
      run: pio test -e pico --verbose
    
    - name: Run integration tests
      run: |
        python test/benchmark_system_performance.py
        python test/integration_test_runner.py
    
    - name: Upload test results
      uses: actions/upload-artifact@v3
      with:
        name: test-results
        path: test/results/
```

#### テスト実行スクリプト

```bash
#!/bin/bash
# run_tests.sh - テスト実行スクリプト

set -e

PROJECT_NAME="GPS NTP Server"
TEST_ENV="pico"

echo "========================================"
echo "  $PROJECT_NAME Test Suite"
echo "========================================"

# テスト前の準備
echo "Preparing test environment..."
pio lib install

# ユニットテスト実行
echo "Running unit tests..."
pio test -e $TEST_ENV --verbose

if [ $? -eq 0 ]; then
    echo "✅ Unit tests passed!"
else
    echo "❌ Unit tests failed!"
    exit 1
fi

# 統合テスト実行
echo "Running integration tests..."
pio test -e $TEST_ENV --filter "test_integration*"

if [ $? -eq 0 ]; then
    echo "✅ Integration tests passed!"
else
    echo "❌ Integration tests failed!"
    exit 1
fi

# パフォーマンステスト実行
echo "Running performance tests..."
python3 test/benchmark_system_performance.py

# テスト結果サマリー
echo "========================================"
echo "  Test Summary"
echo "========================================"
echo "✅ All tests completed successfully!"
echo "📊 Check benchmark_results.json for performance metrics"
```

## 機能拡張ガイド

### 新機能追加プロセス

#### 1. 要求分析・設計

```markdown
# 新機能要求テンプレート

## 機能概要
- **機能名**: HTTPS サポート
- **優先度**: Medium
- **想定工数**: 40 時間

## 要求仕様
### 機能要求
1. Webサーバーで HTTPS 接続をサポート
2. 自己署名証明書による暗号化
3. HTTP からの自動リダイレクト

### 非機能要求
1. パフォーマンス: HTTP と同等の応答速度
2. セキュリティ: TLS 1.2 以上
3. 互換性: 既存 HTTP API との共存

## 技術仕様
### アーキテクチャ変更
- WebServer クラスの拡張
- SSL/TLS ライブラリの追加
- 証明書管理機能の実装

### インターフェース変更
- HTTPS ポート (443) の追加
- SSL 設定 API の追加
- 証明書更新機能

## テスト計画
1. SSL ハンドシェイクテスト
2. 暗号化通信テスト
3. パフォーマンステスト
4. 互換性テスト

## リスク分析
- メモリ使用量増加 (SSL ライブラリ)
- CPU 負荷増加 (暗号化処理)
- 証明書管理の複雑さ
```

#### 2. 実装手順

```cpp
/**
 * Step 1: インターフェース定義
 */

// SSL設定構造体
struct SslConfig {
    bool enabled;
    uint16_t https_port;
    char certificate_path[128];
    char private_key_path[128];
    bool redirect_http_to_https;
};

// WebServer クラス拡張
class WebServer {
private:
    SslConfig ssl_config_;
    
public:
    // 既存メソッド
    bool initialize();
    void handleRequest();
    
    // 新規メソッド (HTTPS対応)
    bool initializeSSL();
    bool loadSSLCertificate();
    void handleHTTPSRequest();
    bool redirectToHTTPS();
};

/**
 * Step 2: 実装
 */
bool WebServer::initializeSSL() {
    if (!ssl_config_.enabled) {
        return true;  // SSL無効時は成功として処理
    }
    
    // SSL ライブラリ初期化
    if (!initSSLLibrary()) {
        LOG_ERROR("WEB", "Failed to initialize SSL library");
        return false;
    }
    
    // 証明書読み込み
    if (!loadSSLCertificate()) {
        LOG_ERROR("WEB", "Failed to load SSL certificate");
        return false;
    }
    
    // HTTPS ポートでリッスン開始
    if (!startHTTPSListener(ssl_config_.https_port)) {
        LOG_ERROR("WEB", "Failed to start HTTPS listener");
        return false;
    }
    
    LOG_INFO("WEB", "HTTPS server started on port %d", ssl_config_.https_port);
    return true;
}

/**
 * Step 3: 設定統合
 */
void ConfigManager::addSSLConfig() {
    // システム設定にSSL設定を追加
    config_.ssl.enabled = false;  // デフォルトは無効
    config_.ssl.https_port = 443;
    strcpy(config_.ssl.certificate_path, "/certs/server.crt");
    strcpy(config_.ssl.private_key_path, "/certs/server.key");
    config_.ssl.redirect_http_to_https = false;
}

/**
 * Step 4: テスト実装
 */
void test_https_initialization() {
    // Arrange
    WebServer web_server;
    SslConfig ssl_config = {
        .enabled = true,
        .https_port = 443,
        .certificate_path = "/test/cert.pem",
        .private_key_path = "/test/key.pem",
        .redirect_http_to_https = true
    };
    web_server.setSSLConfig(ssl_config);
    
    // Act
    bool result = web_server.initialize();
    
    // Assert
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(web_server.isHTTPSEnabled());
}
```

#### 3. ドキュメント更新

```markdown
# API仕様書更新例

## HTTPS エンドポイント

### Base URL
- HTTP: `http://[device_ip_address]/`
- HTTPS: `https://[device_ip_address]/` (SSL証明書使用)

### SSL設定 API

#### POST /api/config/ssl

SSL/HTTPS設定を更新します。

**リクエスト例:**
```json
{
  "enabled": true,
  "https_port": 443,
  "redirect_http": true,
  "certificate_info": {
    "subject": "CN=gps-ntp-server",
    "valid_from": "2025-01-01T00:00:00Z",
    "valid_until": "2026-01-01T00:00:00Z"
  }
}
```

**レスポンス例:**
```json
{
  "status": "success",
  "message": "SSL configuration updated",
  "restart_required": true
}
```
```

### カスタムサービス作成

#### サービス基底クラス

```cpp
/**
 * @brief カスタムサービス作成テンプレート
 */
class CustomService : public ServiceBase {
private:
    const char* service_name_;
    bool is_initialized_;
    bool is_healthy_;
    uint32_t last_update_time_;
    
public:
    CustomService(const char* name) 
        : service_name_(name), 
          is_initialized_(false),
          is_healthy_(false),
          last_update_time_(0) {}
    
    bool initialize() override {
        LOG_INFO("CUSTOM", "Initializing %s service...", service_name_);
        
        // カスタム初期化処理
        if (!performCustomInitialization()) {
            LOG_ERROR("CUSTOM", "Failed to initialize %s", service_name_);
            return false;
        }
        
        is_initialized_ = true;
        is_healthy_ = true;
        last_update_time_ = millis();
        
        LOG_INFO("CUSTOM", "%s service initialized successfully", service_name_);
        return true;
    }
    
    void update() override {
        if (!is_initialized_) return;
        
        uint32_t current_time = millis();
        
        // 更新間隔チェック (例: 1秒間隔)
        if (current_time - last_update_time_ < 1000) {
            return;
        }
        
        // カスタム更新処理
        performCustomUpdate();
        
        last_update_time_ = current_time;
    }
    
    bool isHealthy() const override {
        return is_initialized_ && is_healthy_;
    }
    
    void shutdown() override {
        LOG_INFO("CUSTOM", "Shutting down %s service...", service_name_);
        
        // カスタムクリーンアップ処理
        performCustomCleanup();
        
        is_initialized_ = false;
        is_healthy_ = false;
    }
    
    const char* getName() const override {
        return service_name_;
    }
    
protected:
    // サブクラスで実装する仮想メソッド
    virtual bool performCustomInitialization() = 0;
    virtual void performCustomUpdate() = 0;
    virtual void performCustomCleanup() = 0;
    
    // ヘルスステータス更新
    void setHealthStatus(bool healthy) {
        is_healthy_ = healthy;
    }
};
```

#### 具象サービス実装例

```cpp
/**
 * @brief MQTT クライアントサービス実装例
 */
class MqttClientService : public CustomService {
private:
    struct MqttConfig {
        char broker_host[64];
        uint16_t broker_port;
        char client_id[32];
        char username[32];
        char password[32];
        uint16_t keepalive_interval;
    } mqtt_config_;
    
    // MQTT クライアントライブラリのインスタンス
    // (実際のライブラリに依存)
    MqttClient mqtt_client_;
    
    uint32_t last_publish_time_;
    uint32_t reconnect_attempts_;
    
public:
    MqttClientService() : CustomService("MQTT Client"), 
                         last_publish_time_(0),
                         reconnect_attempts_(0) {
        // デフォルト設定
        strcpy(mqtt_config_.broker_host, "localhost");
        mqtt_config_.broker_port = 1883;
        strcpy(mqtt_config_.client_id, "gps-ntp-server");
        mqtt_config_.keepalive_interval = 60;
    }
    
protected:
    bool performCustomInitialization() override {
        // MQTT クライアント初期化
        mqtt_client_.setServer(mqtt_config_.broker_host, 
                              mqtt_config_.broker_port);
        mqtt_client_.setClientId(mqtt_config_.client_id);
        
        if (strlen(mqtt_config_.username) > 0) {
            mqtt_client_.setCredentials(mqtt_config_.username, 
                                       mqtt_config_.password);
        }
        
        // 接続試行
        return connectToBroker();
    }
    
    void performCustomUpdate() override {
        // 接続状態チェック
        if (!mqtt_client_.connected()) {
            LOG_WARNING("MQTT", "Connection lost, attempting to reconnect...");
            if (connectToBroker()) {
                reconnect_attempts_ = 0;
            } else {
                reconnect_attempts_++;
                if (reconnect_attempts_ > 5) {
                    setHealthStatus(false);
                    return;
                }
            }
        }
        
        // MQTT メッセージ処理
        mqtt_client_.loop();
        
        // 定期的なステータス送信 (30秒間隔)
        uint32_t current_time = millis();
        if (current_time - last_publish_time_ > 30000) {
            publishSystemStatus();
            last_publish_time_ = current_time;
        }
    }
    
    void performCustomCleanup() override {
        if (mqtt_client_.connected()) {
            mqtt_client_.disconnect();
        }
    }
    
private:
    bool connectToBroker() {
        LOG_INFO("MQTT", "Connecting to broker %s:%d...", 
                mqtt_config_.broker_host, mqtt_config_.broker_port);
        
        bool connected = mqtt_client_.connect();
        if (connected) {
            LOG_INFO("MQTT", "Connected to MQTT broker");
            
            // トピック購読
            mqtt_client_.subscribe("gps-ntp/config");
            mqtt_client_.subscribe("gps-ntp/command");
            
            setHealthStatus(true);
        } else {
            LOG_ERROR("MQTT", "Failed to connect to MQTT broker");
            setHealthStatus(false);
        }
        
        return connected;
    }
    
    void publishSystemStatus() {
        if (!mqtt_client_.connected()) return;
        
        // システム状態をJSON形式でパブリッシュ
        char status_json[512];
        snprintf(status_json, sizeof(status_json),
            "{"
            "\"timestamp\":%u,"
            "\"uptime\":%u,"
            "\"gps_fix\":true,"
            "\"satellites\":%d,"
            "\"ntp_clients\":%d,"
            "\"memory_free\":%u"
            "}",
            getCurrentTime(),
            getSystemUptime(),
            getGpsSatelliteCount(),
            getNtpClientCount(),
            getFreeMemory()
        );
        
        mqtt_client_.publish("gps-ntp/status", status_json);
        LOG_DEBUG("MQTT", "Published system status");
    }
};
```

#### サービス登録・統合

```cpp
/**
 * @brief カスタムサービスをシステムに統合
 */
void SystemController::registerCustomServices() {
    // MQTT サービスを追加
    auto mqtt_service = std::make_unique<MqttClientService>();
    addService(std::move(mqtt_service));
    
    // 他のカスタムサービスも同様に追加可能
    // auto custom_service = std::make_unique<AnotherCustomService>();
    // addService(std::move(custom_service));
}

void SystemController::addService(std::unique_ptr<ServiceBase> service) {
    if (services_.size() >= MAX_SERVICES) {
        LOG_ERROR("SYSTEM", "Maximum number of services reached");
        return;
    }
    
    const char* service_name = service->getName();
    LOG_INFO("SYSTEM", "Registering service: %s", service_name);
    
    services_.push_back(std::move(service));
}

// main.cpp での使用例
void setup() {
    SystemController& system = SystemController::getInstance();
    
    // 標準サービス初期化
    system.initializeStandardServices();
    
    // カスタムサービス登録
    system.registerCustomServices();
    
    // 全サービス開始
    if (!system.startAllServices()) {
        LOG_ERROR("SYSTEM", "Failed to start services");
        return;
    }
    
    LOG_INFO("SYSTEM", "All services started successfully");
}
```

### プラグインアーキテクチャ

#### プラグインインターフェース

```cpp
/**
 * @brief プラグインベースインターフェース
 */
class PluginInterface {
public:
    virtual ~PluginInterface() = default;
    
    // プラグイン情報
    virtual const char* getName() const = 0;
    virtual const char* getVersion() const = 0;
    virtual const char* getDescription() const = 0;
    
    // ライフサイクル
    virtual bool initialize() = 0;
    virtual void update() = 0;
    virtual void shutdown() = 0;
    
    // イベントハンドラ
    virtual void onSystemEvent(SystemEvent event) = 0;
    virtual void onConfigChanged(const char* section) = 0;
    
    // プラグイン固有機能
    virtual bool handleCommand(const char* command, 
                              const char* params,
                              char* response, 
                              size_t response_size) = 0;
};

/**
 * @brief プラグイン管理クラス
 */
class PluginManager {
private:
    std::vector<std::unique_ptr<PluginInterface>> plugins_;
    bool plugins_initialized_;
    
public:
    PluginManager() : plugins_initialized_(false) {}
    
    // プラグイン登録
    void registerPlugin(std::unique_ptr<PluginInterface> plugin) {
        LOG_INFO("PLUGIN", "Registering plugin: %s v%s", 
                plugin->getName(), plugin->getVersion());
        plugins_.push_back(std::move(plugin));
    }
    
    // 全プラグイン初期化
    bool initializeAll() {
        LOG_INFO("PLUGIN", "Initializing %d plugins...", plugins_.size());
        
        for (auto& plugin : plugins_) {
            if (!plugin->initialize()) {
                LOG_ERROR("PLUGIN", "Failed to initialize plugin: %s", 
                         plugin->getName());
                return false;
            }
        }
        
        plugins_initialized_ = true;
        LOG_INFO("PLUGIN", "All plugins initialized successfully");
        return true;
    }
    
    // 全プラグイン更新
    void updateAll() {
        if (!plugins_initialized_) return;
        
        for (auto& plugin : plugins_) {
            plugin->update();
        }
    }
    
    // イベント配信
    void broadcastEvent(SystemEvent event) {
        for (auto& plugin : plugins_) {
            plugin->onSystemEvent(event);
        }
    }
    
    // コマンド処理
    bool handleCommand(const char* plugin_name,
                      const char* command,
                      const char* params,
                      char* response,
                      size_t response_size) {
        for (auto& plugin : plugins_) {
            if (strcmp(plugin->getName(), plugin_name) == 0) {
                return plugin->handleCommand(command, params, 
                                           response, response_size);
            }
        }
        return false;
    }
};
```

## パフォーマンス最適化

### メモリ最適化

#### バッファ管理

```cpp
/**
 * @brief 効率的なリングバッファ実装
 */
template<typename T, size_t Capacity>
class RingBuffer {
private:
    T buffer_[Capacity];
    size_t head_;
    size_t tail_;
    size_t size_;
    
public:
    RingBuffer() : head_(0), tail_(0), size_(0) {}
    
    bool push(const T& item) {
        if (size_ >= Capacity) {
            return false;  // バッファ満杯
        }
        
        buffer_[tail_] = item;
        tail_ = (tail_ + 1) % Capacity;
        ++size_;
        return true;
    }
    
    bool pop(T& item) {
        if (size_ == 0) {
            return false;  // バッファ空
        }
        
        item = buffer_[head_];
        head_ = (head_ + 1) % Capacity;
        --size_;
        return true;
    }
    
    constexpr size_t capacity() const { return Capacity; }
    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    bool full() const { return size_ >= Capacity; }
};

// 使用例: ログバッファ
using LogEntryBuffer = RingBuffer<LogEntry, 100>;
using NtpRequestBuffer = RingBuffer<NtpRequest, 50>;
```

#### 文字列最適化

```cpp
/**
 * @brief スタック上の固定長文字列クラス
 * std::string の代替として使用
 */
template<size_t MaxLength>
class FixedString {
private:
    char data_[MaxLength + 1];  // null終端用
    size_t length_;
    
public:
    FixedString() : length_(0) {
        data_[0] = '\0';
    }
    
    FixedString(const char* str) {
        copyFrom(str);
    }
    
    FixedString& operator=(const char* str) {
        copyFrom(str);
        return *this;
    }
    
    const char* c_str() const { return data_; }
    size_t length() const { return length_; }
    constexpr size_t capacity() const { return MaxLength; }
    
    bool append(const char* str) {
        size_t str_len = strlen(str);
        if (length_ + str_len > MaxLength) {
            return false;  // 容量超過
        }
        
        strcpy(data_ + length_, str);
        length_ += str_len;
        return true;
    }
    
private:
    void copyFrom(const char* str) {
        if (!str) {
            data_[0] = '\0';
            length_ = 0;
            return;
        }
        
        length_ = strlen(str);
        if (length_ > MaxLength) {
            length_ = MaxLength;
        }
        
        strncpy(data_, str, length_);
        data_[length_] = '\0';
    }
};

// 使用例
using Hostname = FixedString<63>;
using IpAddress = FixedString<15>;
using LogMessage = FixedString<256>;
```

### CPU最適化

#### 効率的なアルゴリズム

```cpp
/**
 * @brief 高速CRC32実装 (テーブル使用)
 */
class FastCRC32 {
private:
    static constexpr uint32_t CRC_TABLE[256] = {
        // IEEE 802.3 CRC32 テーブル (省略)
        0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
        // ... 残り252エントリ
    };
    
public:
    static uint32_t calculate(const void* data, size_t length) {
        const uint8_t* bytes = static_cast<const uint8_t*>(data);
        uint32_t crc = 0xFFFFFFFF;
        
        // テーブル参照による高速計算
        for (size_t i = 0; i < length; ++i) {
            uint8_t table_index = (crc ^ bytes[i]) & 0xFF;
            crc = (crc >> 8) ^ CRC_TABLE[table_index];
        }
        
        return crc ^ 0xFFFFFFFF;
    }
};

/**
 * @brief NTPタイムスタンプ高速変換
 */
class NtpTimeConverter {
public:
    // Unix時刻からNTPタイムスタンプへの変換
    static void unixToNtp(uint32_t unix_time, uint32_t microseconds,
                         uint32_t& ntp_seconds, uint32_t& ntp_fraction) {
        // NTP epoch (1900-01-01) とUnix epoch (1970-01-01) の差: 70年
        constexpr uint32_t NTP_UNIX_OFFSET = 2208988800UL;
        
        ntp_seconds = unix_time + NTP_UNIX_OFFSET;
        
        // マイクロ秒を32bit固定小数点に変換
        // fraction = microseconds * 2^32 / 1000000
        // 最適化: (microseconds * 4294967296) / 1000000
        uint64_t temp = static_cast<uint64_t>(microseconds) * 4294967296ULL;
        ntp_fraction = static_cast<uint32_t>(temp / 1000000ULL);
    }
    
    // NTPタイムスタンプからUnix時刻への変換
    static void ntpToUnix(uint32_t ntp_seconds, uint32_t ntp_fraction,
                         uint32_t& unix_time, uint32_t& microseconds) {
        constexpr uint32_t NTP_UNIX_OFFSET = 2208988800UL;
        
        unix_time = ntp_seconds - NTP_UNIX_OFFSET;
        
        // 32bit固定小数点をマイクロ秒に変換
        uint64_t temp = static_cast<uint64_t>(ntp_fraction) * 1000000ULL;
        microseconds = static_cast<uint32_t>(temp / 4294967296ULL);
    }
};
```

#### 分岐予測最適化

```cpp
/**
 * @brief 分岐予測ヒントマクロ (GCC/Clang)
 */
#ifdef __GNUC__
    #define LIKELY(x)   __builtin_expect(!!(x), 1)
    #define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
    #define LIKELY(x)   (x)
    #define UNLIKELY(x) (x)
#endif

/**
 * @brief 最適化された条件分岐
 */
void processNtpRequest(const NtpPacket& request) {
    // よくあるケース: 通常のNTP要求
    if (LIKELY(request.mode == NTP_MODE_CLIENT)) {
        handleClientRequest(request);
        return;
    }
    
    // 稀なケース: 不正なパケット
    if (UNLIKELY(request.version < 3 || request.version > 4)) {
        LOG_WARNING("NTP", "Invalid NTP version: %d", request.version);
        return;
    }
    
    // その他のモード処理
    handleSpecialRequest(request);
}

/**
 * @brief ループ最適化
 */
void processGpsSatellites(const SatelliteData* satellites, size_t count) {
    size_t used_count = 0;
    
    // ループアンロール + 分岐予測
    for (size_t i = 0; i < count; ++i) {
        if (LIKELY(satellites[i].signal_strength > MINIMUM_SIGNAL_STRENGTH)) {
            if (LIKELY(satellites[i].used_in_navigation)) {
                ++used_count;
            }
            processSatelliteData(&satellites[i]);
        }
    }
    
    updateSatelliteCount(used_count);
}
```

## トラブルシューティング

### 一般的な問題と解決法

#### ビルドエラー

```bash
# 問題: ライブラリ依存関係エラー
Error: Library not found: SparkFun u-blox GNSS Arduino Library

# 解決法:
pio lib install  # 依存関係を再インストール
pio lib update   # ライブラリを最新版に更新

# 問題: メモリ不足エラー
region `RAM' overflowed by 1024 bytes

# 解決法:
# 1. バッファサイズを削減
#define LOG_BUFFER_SIZE 2048  // 4096から削減

# 2. 未使用機能の無効化
#undef ENABLE_PROMETHEUS_METRICS
#undef ENABLE_WEB_INTERFACE

# 3. 最適化レベル調整 (platformio.ini)
build_flags = -Os  # サイズ最適化
```

#### 実行時エラー

```cpp
/**
 * @brief ランタイムエラー診断ツール
 */
class RuntimeDiagnostics {
public:
    static void performHealthCheck() {
        LOG_INFO("DIAG", "=== System Health Check ===");
        
        // メモリ使用量チェック
        checkMemoryUsage();
        
        // ハードウェア通信チェック
        checkHardwareCommunication();
        
        // システム状態チェック
        checkSystemState();
        
        LOG_INFO("DIAG", "=== Health Check Complete ===");
    }
    
private:
    static void checkMemoryUsage() {
        size_t free_memory = getFreeMemory();
        size_t total_memory = getTotalMemory();
        float usage_percent = (float)(total_memory - free_memory) / total_memory * 100;
        
        LOG_INFO("DIAG", "Memory: %d/%d bytes (%.1f%% used)", 
                total_memory - free_memory, total_memory, usage_percent);
        
        if (usage_percent > 80) {
            LOG_WARNING("DIAG", "High memory usage detected!");
        }
    }
    
    static void checkHardwareCommunication() {
        // I2C バススキャン
        LOG_INFO("DIAG", "Scanning I2C buses...");
        
        std::vector<uint8_t> i2c0_devices, i2c1_devices;
        I2CUtils::scanBus(0, i2c0_devices);
        I2CUtils::scanBus(1, i2c1_devices);
        
        LOG_INFO("DIAG", "I2C0 devices found: %d", i2c0_devices.size());
        for (uint8_t addr : i2c0_devices) {
            LOG_INFO("DIAG", "  Device at 0x%02X", addr);
        }
        
        LOG_INFO("DIAG", "I2C1 devices found: %d", i2c1_devices.size());
        for (uint8_t addr : i2c1_devices) {
            LOG_INFO("DIAG", "  Device at 0x%02X", addr);
        }
        
        // 期待されるデバイスの存在確認
        bool oled_found = std::find(i2c0_devices.begin(), i2c0_devices.end(), 0x3C) != i2c0_devices.end();
        bool gps_found = std::find(i2c1_devices.begin(), i2c1_devices.end(), 0x42) != i2c1_devices.end();
        
        if (!oled_found) LOG_WARNING("DIAG", "OLED display not found (0x3C)");
        if (!gps_found) LOG_WARNING("DIAG", "GPS module not found (0x42)");
    }
    
    static void checkSystemState() {
        SystemController& system = SystemController::getInstance();
        SystemState state = system.getState();
        
        LOG_INFO("DIAG", "System state: %s", getSystemStateName(state));
        
        // サービス健全性チェック
        auto services = system.getServices();
        for (const auto& service : services) {
            bool healthy = service->isHealthy();
            LOG_INFO("DIAG", "Service %s: %s", 
                    service->getName(), 
                    healthy ? "HEALTHY" : "UNHEALTHY");
        }
    }
};
```

#### ネットワーク問題診断

```cpp
/**
 * @brief ネットワーク診断ツール
 */
class NetworkDiagnostics {
public:
    struct DiagnosticResult {
        bool success;
        char message[128];
        uint32_t response_time_ms;
    };
    
    static DiagnosticResult testEthernetConnection() {
        DiagnosticResult result = {false, "", 0};
        
        uint32_t start_time = millis();
        
        // W5500 ハードウェア検出
        if (!W5500_HAL::detectHardware()) {
            strcpy(result.message, "W5500 hardware not detected");
            return result;
        }
        
        // リンク状態チェック
        if (!W5500_HAL::isLinkUp()) {
            strcpy(result.message, "Ethernet cable not connected");
            return result;
        }
        
        // IP設定確認
        uint32_t ip_address = W5500_HAL::getIPAddress();
        if (ip_address == 0) {
            strcpy(result.message, "No IP address assigned");
            return result;
        }
        
        result.success = true;
        result.response_time_ms = millis() - start_time;
        snprintf(result.message, sizeof(result.message), 
                "Connection OK (IP: %s)", formatIPAddress(ip_address));
        
        return result;
    }
    
    static DiagnosticResult testDNSResolution(const char* hostname) {
        DiagnosticResult result = {false, "", 0};
        uint32_t start_time = millis();
        
        uint32_t resolved_ip = DNS::resolve(hostname);
        result.response_time_ms = millis() - start_time;
        
        if (resolved_ip == 0) {
            snprintf(result.message, sizeof(result.message),
                    "Failed to resolve %s", hostname);
        } else {
            result.success = true;
            snprintf(result.message, sizeof(result.message),
                    "%s -> %s", hostname, formatIPAddress(resolved_ip));
        }
        
        return result;
    }
    
    static DiagnosticResult testPing(uint32_t target_ip) {
        DiagnosticResult result = {false, "", 0};
        
        // ICMP ping実装 (簡略版)
        uint32_t start_time = millis();
        bool ping_success = performICMPPing(target_ip);
        result.response_time_ms = millis() - start_time;
        
        if (ping_success) {
            result.success = true;
            snprintf(result.message, sizeof(result.message),
                    "Ping to %s successful", formatIPAddress(target_ip));
        } else {
            snprintf(result.message, sizeof(result.message),
                    "Ping to %s failed", formatIPAddress(target_ip));
        }
        
        return result;
    }
};
```

### ログ分析ツール

```python
#!/usr/bin/env python3
"""
log_analyzer.py - ログ分析ツール
"""

import re
import sys
from datetime import datetime
from collections import defaultdict, Counter

class LogAnalyzer:
    def __init__(self, log_file_path):
        self.log_file_path = log_file_path
        self.log_entries = []
        self.parse_logs()
    
    def parse_logs(self):
        log_pattern = r'\[(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3})\] \[(\w+)\] \[(\w+)\] (.+)'
        
        with open(self.log_file_path, 'r') as f:
            for line in f:
                match = re.match(log_pattern, line.strip())
                if match:
                    timestamp_str, level, component, message = match.groups()
                    timestamp = datetime.strptime(timestamp_str, '%Y-%m-%d %H:%M:%S.%f')
                    
                    self.log_entries.append({
                        'timestamp': timestamp,
                        'level': level,
                        'component': component,
                        'message': message
                    })
    
    def analyze_error_patterns(self):
        """エラーパターン分析"""
        error_entries = [entry for entry in self.log_entries if entry['level'] in ['ERROR', 'CRITICAL']]
        error_messages = [entry['message'] for entry in error_entries]
        
        print("=== Error Pattern Analysis ===")
        print(f"Total errors: {len(error_entries)}")
        
        # エラーメッセージ頻度分析
        error_counter = Counter(error_messages)
        print("\nMost common errors:")
        for error, count in error_counter.most_common(10):
            print(f"  {count:3d}x: {error}")
    
    def analyze_component_activity(self):
        """コンポーネント別活動分析"""
        component_counter = Counter(entry['component'] for entry in self.log_entries)
        
        print("\n=== Component Activity Analysis ===")
        for component, count in component_counter.most_common():
            print(f"  {component:12s}: {count:5d} messages")
    
    def analyze_time_patterns(self):
        """時系列パターン分析"""
        if not self.log_entries:
            return
        
        start_time = self.log_entries[0]['timestamp']
        end_time = self.log_entries[-1]['timestamp']
        duration = end_time - start_time
        
        print(f"\n=== Time Pattern Analysis ===")
        print(f"Log duration: {duration}")
        print(f"Total entries: {len(self.log_entries)}")
        print(f"Average rate: {len(self.log_entries) / duration.total_seconds():.2f} msg/sec")
        
        # 時間別メッセージ数
        hourly_counts = defaultdict(int)
        for entry in self.log_entries:
            hour = entry['timestamp'].hour
            hourly_counts[hour] += 1
        
        print("\nHourly message distribution:")
        for hour in sorted(hourly_counts.keys()):
            count = hourly_counts[hour]
            bar = '#' * (count // 10) if count > 0 else ''
            print(f"  {hour:2d}:00 {count:4d} {bar}")
    
    def find_critical_events(self):
        """重要イベント検出"""
        critical_patterns = [
            r'GPS.*timeout',
            r'Memory.*low',
            r'Hardware.*failure',
            r'Network.*disconnected',
            r'System.*restart'
        ]
        
        print("\n=== Critical Events ===")
        for entry in self.log_entries:
            message = entry['message']
            for pattern in critical_patterns:
                if re.search(pattern, message, re.IGNORECASE):
                    print(f"  {entry['timestamp']} [{entry['level']}] {message}")
                    break

def main():
    if len(sys.argv) != 2:
        print("Usage: python log_analyzer.py <log_file>")
        sys.exit(1)
    
    log_file = sys.argv[1]
    analyzer = LogAnalyzer(log_file)
    
    analyzer.analyze_error_patterns()
    analyzer.analyze_component_activity() 
    analyzer.analyze_time_patterns()
    analyzer.find_critical_events()

if __name__ == "__main__":
    main()
```

## リリース管理

### バージョン管理戦略

#### セマンティックバージョニング

```
Version Format: MAJOR.MINOR.PATCH[-PRERELEASE][+BUILD]

Examples:
├─ 1.0.0         # 初回リリース
├─ 1.0.1         # バグフィックス
├─ 1.1.0         # 新機能追加
├─ 2.0.0         # 破壊的変更
├─ 1.2.0-beta.1  # ベータ版
└─ 1.0.0+20250730 # ビルド情報付き

Version Rules:
├─ MAJOR: 破壊的変更時にインクリメント
├─ MINOR: 後方互換性を保つ新機能追加時
├─ PATCH: 後方互換性を保つバグ修正時
├─ PRERELEASE: alpha, beta, rc 形式
└─ BUILD: ビルド日付やコミットハッシュ
```

#### リリースブランチ戦略

```bash
# Git Flow ベースのブランチ戦略

# 基本ブランチ
main/               # 安定版リリース
develop/            # 開発統合ブランチ

# 機能ブランチ
feature/add-https   # 新機能開発
feature/mqtt-client # MQTT機能追加

# リリースブランチ
release/1.1.0       # リリース準備

# ホットフィックスブランチ
hotfix/fix-memory-leak  # 緊急修正

# リリース手順
git checkout develop
git checkout -b release/1.1.0

# 最終テスト・バグ修正
./run_full_tests.sh
git commit -am "Fix minor bugs for v1.1.0"

# タグ作成とmainへマージ
git checkout main
git merge --no-ff release/1.1.0
git tag -a v1.1.0 -m "Release version 1.1.0"

# developにもマージ
git checkout develop
git merge --no-ff release/1.1.0
git branch -d release/1.1.0
```

#### 自動ビルドとデプロイ

```yaml
# .github/workflows/release.yml
name: Release Build

on:
  push:
    tags:
      - 'v*'

jobs:
  build-release:
    runs-on: ubuntu-latest
    
    steps:
    - name: Checkout code
      uses: actions/checkout@v3
    
    - name: Extract version
      id: version
      run: echo "VERSION=${GITHUB_REF#refs/tags/v}" >> $GITHUB_OUTPUT
    
    - name: Set up PlatformIO
      run: |
        pip install platformio
        pio platform update
    
    - name: Build firmware
      run: |
        # バージョン情報を埋め込み
        echo "#define FIRMWARE_VERSION \"${{ steps.version.outputs.VERSION }}\"" > src/version.h
        echo "#define BUILD_DATE \"$(date -u +'%Y-%m-%d %H:%M:%S UTC')\"" >> src/version.h
        echo "#define GIT_COMMIT \"${GITHUB_SHA:0:8}\"" >> src/version.h
        
        # ビルド実行
        pio run -e pico
    
    - name: Run tests
      run: pio test -e pico
    
    - name: Generate checksums
      run: |
        cd .pio/build/pico/
        sha256sum firmware.elf > checksums.txt
        sha256sum firmware.uf2 >> checksums.txt
    
    - name: Create release
      uses: actions/create-release@v1
      env:
        GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
      with:
        tag_name: ${{ github.ref }}
        release_name: GPS NTP Server v${{ steps.version.outputs.VERSION }}
        draft: false
        prerelease: ${{ contains(github.ref, 'beta') || contains(github.ref, 'alpha') }}
    
    - name: Upload firmware
      uses: actions/upload-release-asset@v1
      env:
        GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
      with:
        upload_url: ${{ steps.create_release.outputs.upload_url }}
        asset_path: .pio/build/pico/firmware.uf2
        asset_name: gps-ntp-server-v${{ steps.version.outputs.VERSION }}.uf2
        asset_content_type: application/octet-stream
```

### 品質保証プロセス

#### リリース前チェックリスト

```markdown
# Release Checklist - v1.1.0

## Pre-Release Testing
- [ ] All unit tests pass (pio test)
- [ ] Integration tests pass
- [ ] Performance benchmarks within limits
- [ ] Memory usage < 80% RAM, < 90% Flash
- [ ] 24-hour stability test completed
- [ ] Hardware compatibility verified

## Documentation Updates
- [ ] User manual updated
- [ ] API specification current
- [ ] Hardware interface docs reviewed
- [ ] CHANGELOG.md updated
- [ ] Version numbers updated in code

## Security Review
- [ ] No hardcoded credentials
- [ ] Input validation comprehensive
- [ ] Error messages don't leak sensitive info
- [ ] Rate limiting functional
- [ ] Logging sanitized

## Compatibility Testing
- [ ] Backward compatibility maintained
- [ ] NTP client compatibility verified
- [ ] Web browser compatibility tested
- [ ] Configuration migration tested

## Build Verification
- [ ] Clean build from source
- [ ] All dependencies available
- [ ] Build reproducibility verified
- [ ] Binary signatures valid

## Deployment Testing
- [ ] Fresh installation tested
- [ ] Upgrade from previous version tested
- [ ] Factory reset functional
- [ ] Configuration backup/restore tested

## Final Approval
- [ ] Technical review completed
- [ ] QA sign-off obtained
- [ ] Release notes finalized
- [ ] Distribution channels prepared
```

#### 継続的品質監視

```python
#!/usr/bin/env python3
"""
quality_monitor.py - 品質メトリクス監視
"""

import subprocess
import json
import requests
from datetime import datetime

class QualityMonitor:
    def __init__(self, config_file):
        with open(config_file, 'r') as f:
            self.config = json.load(f)
    
    def check_build_quality(self):
        """ビルド品質チェック"""
        result = {
            'timestamp': datetime.now().isoformat(),
            'build_success': False,
            'test_results': {},
            'metrics': {}
        }
        
        try:
            # ビルド実行
            build_result = subprocess.run(['pio', 'run', '-e', 'pico'], 
                                        capture_output=True, text=True)
            result['build_success'] = build_result.returncode == 0
            
            if not result['build_success']:
                result['build_error'] = build_result.stderr
                return result
            
            # テスト実行
            test_result = subprocess.run(['pio', 'test', '-e', 'pico'], 
                                       capture_output=True, text=True)
            result['test_results']['success'] = test_result.returncode == 0
            result['test_results']['output'] = test_result.stdout
            
            # パフォーマンステスト
            perf_result = subprocess.run(['python3', 'test/benchmark_system_performance.py'],
                                       capture_output=True, text=True)
            if perf_result.returncode == 0:
                with open('test/benchmark_results.json', 'r') as f:
                    result['metrics'] = json.load(f)
            
        except Exception as e:
            result['error'] = str(e)
        
        return result
    
    def report_quality_metrics(self, metrics):
        """品質メトリクスを報告"""
        # Slack/Discord通知
        if 'slack_webhook' in self.config:
            self.send_slack_notification(metrics)
        
        # 品質データベース記録
        if 'quality_db_url' in self.config:
            self.store_quality_data(metrics)
    
    def send_slack_notification(self, metrics):
        """Slack通知送信"""
        webhook_url = self.config['slack_webhook']
        
        color = 'good' if metrics['build_success'] else 'danger'
        message = {
            'attachments': [{
                'color': color,
                'title': 'GPS NTP Server Build Quality Report',
                'fields': [
                    {
                        'title': 'Build Status',
                        'value': '✅ Success' if metrics['build_success'] else '❌ Failed',
                        'short': True
                    },
                    {
                        'title': 'Test Results', 
                        'value': '✅ Pass' if metrics['test_results'].get('success') else '❌ Fail',
                        'short': True
                    }
                ]
            }]
        }
        
        if 'metrics' in metrics:
            perf_metrics = metrics['metrics']
            message['attachments'][0]['fields'].extend([
                {
                    'title': 'RAM Usage',
                    'value': f"{perf_metrics['memory_analysis']['ram']['used_percent']}%",
                    'short': True
                },
                {
                    'title': 'Flash Usage',
                    'value': f"{perf_metrics['memory_analysis']['flash']['used_percent']}%",
                    'short': True
                }
            ])
        
        requests.post(webhook_url, json=message)

def main():
    monitor = QualityMonitor('quality_config.json')
    metrics = monitor.check_build_quality()
    monitor.report_quality_metrics(metrics)
    
    print(json.dumps(metrics, indent=2))

if __name__ == "__main__":
    main()
```

---

## まとめ

本開発ガイドでは、GPS NTP Serverの開発・拡張・保守に必要な包括的な情報を提供しました。以下の要点を守ることで、品質の高いソフトウェアを効率的に開発できます：

### 開発の基本原則

1. **テスト駆動開発**: 機能実装前にテストを作成
2. **コーディング規約の遵守**: 一貫したコードスタイルの維持
3. **ドキュメント作成**: コードと同時にドキュメント更新
4. **継続的品質改善**: 定期的なリファクタリングと最適化

### アーキテクチャ設計

1. **階層化アーキテクチャ**: 関心の分離による保守性向上
2. **依存関係注入**: テスタビリティとモジュール性の確保
3. **設計パターン活用**: 実績のあるパターンによる品質向上
4. **組み込み制約対応**: メモリ・CPU制約を考慮した実装

### 品質保証

1. **包括的テスト**: ユニット・統合・システムテストの実施
2. **パフォーマンス監視**: 継続的な性能測定と最適化
3. **エラーハンドリング**: 堅牢なエラー処理とログ出力
4. **セキュリティ対策**: 入力検証とセキュアな実装

本ガイドを活用して、信頼性の高いGPS NTPサーバーの開発を成功させてください。

---

**Document Version**: 1.0  
**Last Updated**: 2025-07-30  
**Author**: GPS NTP Server Development Team  
**Next Review**: 2025-10-30
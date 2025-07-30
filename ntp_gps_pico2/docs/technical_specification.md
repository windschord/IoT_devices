# GPS NTP Server - 技術仕様書

## 目次

1. [システムアーキテクチャ](#システムアーキテクチャ)
2. [API仕様](#api仕様)
3. [ハードウェアインターフェース](#ハードウェアインターフェース)
4. [ソフトウェア構成](#ソフトウェア構成)
5. [プロトコル仕様](#プロトコル仕様)
6. [パフォーマンス仕様](#パフォーマンス仕様)
7. [セキュリティ仕様](#セキュリティ仕様)
8. [開発ガイド](#開発ガイド)

## システムアーキテクチャ

### 概要

GPS NTP Serverは、Raspberry Pi Pico 2（RP2350）をベースとした組み込みNTPサーバーです。GPS/GNSS受信機からの高精度時刻情報を用いて、ネットワーク経由でNTPサービスを提供します。

### 階層アーキテクチャ

```
┌─────────────────────────────────────────┐
│           Application Layer             │
├─────────────────────────────────────────┤
│  NTP Server  │  Web Server  │ Metrics   │
├─────────────────────────────────────────┤
│           Service Layer                 │
├─────────────────────────────────────────┤
│Time Service│Log Service│Config Service │
├─────────────────────────────────────────┤
│      Hardware Abstraction Layer        │
├─────────────────────────────────────────┤
│ GPS HAL │Network HAL│Display HAL│Storage│
├─────────────────────────────────────────┤
│           Hardware Layer                │
├─────────────────────────────────────────┤
│ZED-F9T│W5500│SH1106│Flash│PPS│Buttons  │
└─────────────────────────────────────────┘
```

### コンポーネント関係図

```
     ┌─────────────┐    ┌─────────────┐
     │ GpsClient   │────│SystemMonitor│
     └─────────────┘    └─────────────┘
            │                   │
            ▼                   ▼
     ┌─────────────┐    ┌─────────────┐
     │ TimeService │────│ErrorHandler │
     └─────────────┘    └─────────────┘
            │                   │
            ▼                   ▼
     ┌─────────────┐    ┌─────────────┐
     │  NtpServer  │────│ LogService  │
     └─────────────┘    └─────────────┘
            │                   │
            ▼                   ▼
     ┌─────────────┐    ┌─────────────┐
     │ WebServer   │────│ConfigManager│
     └─────────────┘    └─────────────┘
            │                   │
            ▼                   ▼
     ┌─────────────┐    ┌─────────────┐
     │PrometheusMetrics│ DisplayManager
     └─────────────┘    └─────────────┘
```

### システム状態管理

```cpp
enum class SystemState {
    INITIALIZING,    // システム初期化中
    STARTUP,         // 起動シーケンス実行中
    RUNNING,         // 正常動作中
    DEGRADED,        // 機能制限状態
    ERROR,           // エラー状態
    RECOVERY,        // 復旧処理中
    SHUTDOWN         // シャットダウン中
};
```

## API仕様

### REST API エンドポイント

#### 1. 設定管理API

**ベースパス**: `/api/config`

| エンドポイント | メソッド | 説明 |
|---------------|---------|------|
| `/api/config` | GET | 全設定取得 |
| `/api/config` | POST | 設定更新 |
| `/api/config/network` | GET/POST | ネットワーク設定 |
| `/api/config/gnss` | GET/POST | GNSS設定 |
| `/api/config/ntp` | GET/POST | NTP設定 |
| `/api/config/system` | GET/POST | システム設定 |
| `/api/config/log` | GET/POST | ログ設定 |

**レスポンス形式**:
```json
{
  "status": "success|error",
  "message": "説明メッセージ",
  "data": {
    // 設定データ
  },
  "timestamp": "ISO 8601形式"
}
```

#### 2. システム状態API

**ベースパス**: `/api/status`

| エンドポイント | メソッド | 説明 |
|---------------|---------|------|
| `/api/status` | GET | システム全体状態 |
| `/api/status/gps` | GET | GPS状態詳細 |
| `/api/status/ntp` | GET | NTP統計情報 |
| `/api/status/network` | GET | ネットワーク状態 |
| `/api/status/health` | GET | ヘルスチェック |

#### 3. 診断・制御API

| エンドポイント | メソッド | 説明 |
|---------------|---------|------|
| `/api/diagnostic/network` | POST | ネットワーク診断実行 |
| `/api/diagnostic/gps` | POST | GPS診断実行 |
| `/api/reset` | POST | 工場出荷時リセット |
| `/api/restart` | POST | システム再起動 |

#### 4. メトリクスAPI

| エンドポイント | メソッド | 説明 |
|---------------|---------|------|
| `/metrics` | GET | Prometheus形式メトリクス |
| `/api/metrics` | GET | JSON形式メトリクス |

### HTTP ステータスコード

| コード | 説明 | 用途 |
|-------|------|------|
| 200 | OK | 正常応答 |
| 201 | Created | 設定作成成功 |
| 400 | Bad Request | 不正なリクエスト |
| 404 | Not Found | リソース不存在 |
| 429 | Too Many Requests | レート制限 |
| 500 | Internal Server Error | サーバーエラー |
| 503 | Service Unavailable | サービス利用不可 |

### JSON スキーマ定義

#### 設定データ構造

```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "type": "object",
  "properties": {
    "network": {
      "type": "object",
      "properties": {
        "hostname": {
          "type": "string",
          "maxLength": 63,
          "pattern": "^[a-zA-Z0-9-]+$"
        },
        "dhcp_enabled": {"type": "boolean"},
        "static_ip": {"type": "string", "format": "ipv4"},
        "netmask": {"type": "string", "format": "ipv4"},
        "gateway": {"type": "string", "format": "ipv4"}
      },
      "required": ["hostname", "dhcp_enabled"]
    },
    "gnss": {
      "type": "object",
      "properties": {
        "gps_enabled": {"type": "boolean"},
        "glonass_enabled": {"type": "boolean"},
        "galileo_enabled": {"type": "boolean"},
        "beidou_enabled": {"type": "boolean"},
        "qzss_enabled": {"type": "boolean"},
        "update_rate": {"type": "integer", "enum": [1, 5, 10]},
        "l1s_enabled": {"type": "boolean"},
        "alert_priority": {"type": "integer", "minimum": 0, "maximum": 2}
      }
    },
    "ntp": {
      "type": "object",
      "properties": {
        "server_enabled": {"type": "boolean"},
        "port": {"type": "integer", "minimum": 1, "maximum": 65535},
        "stratum": {"type": "integer", "minimum": 1, "maximum": 15}
      }
    }
  }
}
```

## ハードウェアインターフェース

### ピン配置仕様

#### GPIO ピン配置

```
RP2350 GPIO Configuration:

I2C Interfaces:
├─ I2C0 (OLED専用)
│  ├─ GPIO 0: SDA (4.7kΩ プルアップ)
│  └─ GPIO 1: SCL (4.7kΩ プルアップ)
└─ I2C1 (GPS/RTC共有)
   ├─ GPIO 6: SDA (4.7kΩ プルアップ)
   └─ GPIO 7: SCL (4.7kΩ プルアップ)

SPI Interface (W5500):
├─ GPIO 16: MISO
├─ GPIO 17: CS (Chip Select)
├─ GPIO 18: SCLK
├─ GPIO 19: MOSI
├─ GPIO 20: RST (Reset)
└─ GPIO 21: INT (Interrupt)

Digital I/O:
├─ GPIO 8: PPS Input (GPS)
├─ GPIO 11: Reset Button (プルアップ有効)
├─ GPIO 4: Status LED 1 (GPS Fix - 緑)
├─ GPIO 5: Status LED 2 (Network - 青)
├─ GPIO 14: Status LED 3 (Error - 赤)
└─ GPIO 15: Status LED 4 (PPS - 黄)
```

#### I2C デバイスアドレス

| デバイス | バス | アドレス | 説明 |
|---------|-----|---------|------|
| SH1106 OLED | I2C0 | 0x3C | ディスプレイ |
| ZED-F9T GPS | I2C1 | 0x42 | GPS受信機 |
| DS3231 RTC | I2C1 | 0x68 | リアルタイムクロック |

#### 電源仕様

```
電源要件:
├─ 入力電圧: 5V DC (USB-C)
├─ 消費電流: 平均 500mA, 最大 800mA
├─ 動作電圧: 3.3V (内部レギュレータ)
└─ 待機電流: 50mA (低電力モード)

電源分配:
├─ RP2350: 3.3V @ 150mA
├─ ZED-F9T: 3.3V @ 200mA
├─ W5500: 3.3V @ 100mA
├─ SH1106: 3.3V @ 20mA
└─ LED群: 3.3V @ 30mA
```

### ハードウェア制御仕様

#### LED制御パターン

```cpp
// LED状態定義
enum class LedState {
    OFF,           // 消灯
    ON,            // 点灯
    BLINK_SLOW,    // 低速点滅 (2秒間隔)
    BLINK_FAST,    // 高速点滅 (0.5秒間隔)
    FLASH_BRIEF    // 短時間点灯 (50ms)
};

// GPS Fix LED (緑) - GPIO 4
LED_GPS_FIX:
├─ OFF: GPS未接続/信号なし
├─ BLINK_SLOW: GPS接続、測位中 (fixType < 2)
├─ BLINK_FAST: 2D Fix (fixType = 2)
└─ ON: 3D Fix以上 (fixType ≥ 3)

// Network LED (青) - GPIO 5
LED_NETWORK:
├─ OFF: ネットワーク未接続
└─ ON: IP取得完了

// Error LED (赤) - GPIO 14
LED_ERROR:
├─ OFF: 正常動作
└─ ON: 重大エラー発生

// PPS LED (黄) - GPIO 15
LED_PPS:
├─ OFF: PPS信号なし
└─ FLASH_BRIEF: PPS信号受信 (1秒間隔)
```

#### ボタン制御仕様

```cpp
// ボタン制御パラメータ
#define BUTTON_DEBOUNCE_MS 20      // デバウンス時間
#define BUTTON_SHORT_PRESS_MS 2000 // 短押し判定時間
#define BUTTON_LONG_PRESS_MS 5000  // 長押し判定時間

// ボタン動作
Physical Button (GPIO 11):
├─ 短押し (<2秒): ディスプレイモード切り替え
├─ 長押し (>5秒): 工場出荷時リセット開始
└─ 超長押し (>8秒): 緊急シャットダウン
```

## ソフトウェア構成

### クラス設計

#### 主要クラス階層

```cpp
// コアシステムクラス
class SystemController {
public:
    void initialize();
    void run();
    SystemState getState() const;
    void shutdown();
private:
    std::array<ServiceBase*, 8> services_;
    SystemState state_;
    ErrorHandler error_handler_;
};

// サービス基底クラス
class ServiceBase {
public:
    virtual bool initialize() = 0;
    virtual void update() = 0;
    virtual bool isHealthy() const = 0;
    virtual void shutdown() = 0;
    virtual const char* getName() const = 0;
};

// 主要サービスクラス
class GpsClient : public ServiceBase { /* GPS制御 */ };
class NtpServer : public ServiceBase { /* NTPサーバー */ };
class WebServer : public ServiceBase { /* Webサーバー */ };
class ConfigManager : public ServiceBase { /* 設定管理 */ };
class LoggingService : public ServiceBase { /* ログ管理 */ };
class DisplayManager : public ServiceBase { /* 表示制御 */ };
class PrometheusMetrics : public ServiceBase { /* メトリクス */ };
class NetworkManager : public ServiceBase { /* ネットワーク */ };
```

#### HAL (Hardware Abstraction Layer)

```cpp
namespace HAL {
    // I2C抽象化
    class I2CManager {
    public:
        bool initialize(uint8_t bus, uint8_t sda, uint8_t scl);
        bool scanDevices(uint8_t bus, std::vector<uint8_t>& addresses);
        bool writeRegister(uint8_t bus, uint8_t addr, uint8_t reg, uint8_t data);
        bool readRegister(uint8_t bus, uint8_t addr, uint8_t reg, uint8_t& data);
    };

    // GPIO抽象化
    class GPIOManager {
    public:
        bool setPinMode(uint8_t pin, PinMode mode);
        bool digitalWrite(uint8_t pin, bool value);
        bool digitalRead(uint8_t pin);
        bool attachInterrupt(uint8_t pin, void(*callback)(), InterruptMode mode);
    };

    // Storage抽象化
    class StorageManager {
    public:
        bool initialize();
        bool write(uint32_t address, const void* data, size_t size);
        bool read(uint32_t address, void* data, size_t size);
        uint32_t calculateCRC32(const void* data, size_t size);
    };
}
```

### データ構造

#### 設定データ構造

```cpp
struct SystemConfig {
    // ネットワーク設定
    struct {
        char hostname[64];
        bool dhcp_enabled;
        uint32_t static_ip;
        uint32_t netmask;
        uint32_t gateway;
        uint8_t mac_address[6];
    } network;

    // GNSS設定
    struct {
        bool gps_enabled;
        bool glonass_enabled;
        bool galileo_enabled;
        bool beidou_enabled;
        bool qzss_enabled;
        uint8_t update_rate;        // 1, 5, 10 Hz
        bool l1s_enabled;           // QZSS L1S災害警報
        uint8_t alert_priority;     // 0=High, 1=Medium, 2=Low
    } gnss;

    // NTP設定
    struct {
        bool server_enabled;
        uint16_t port;              // デフォルト: 123
        uint8_t stratum;            // 1-15
    } ntp;

    // システム設定
    struct {
        bool auto_restart_enabled;
        uint16_t restart_interval_hours;
        bool debug_mode_enabled;
    } system;

    // ログ設定
    struct {
        char syslog_server[64];
        uint16_t syslog_port;       // デフォルト: 514 
        uint8_t log_level;          // 0=DEBUG, 4=ERROR
    } log;
};
```

#### GPS データ構造

```cpp
struct GpsSummaryData {
    // 時刻情報
    uint32_t utc_time;              // Unix timestamp
    uint32_t time_accuracy;         // 時刻精度 (ns)
    
    // 測位情報
    uint8_t fix_type;               // 0=No Fix, 2=2D, 3=3D, 4=RTK
    double latitude;                // 緯度 (degrees)
    double longitude;               // 経度 (degrees)
    float altitude;                 // 高度 (m)
    
    // 精度情報
    float pdop, hdop, vdop;         // 精度劣化要因
    float accuracy_3d;              // 3D精度 (m)
    float accuracy_2d;              // 2D精度 (m)
    
    // 衛星情報
    uint8_t satellites_gps;
    uint8_t satellites_glonass;
    uint8_t satellites_galileo;
    uint8_t satellites_beidou;
    uint8_t satellites_qzss;
    uint8_t satellites_used;
    
    // PPS情報
    bool pps_active;
    uint32_t last_pps_time;
    
    // QZSS L1S情報
    bool l1s_signal_detected;
    uint8_t disaster_category;
    char disaster_message[256];
    uint32_t message_timestamp;
};
```

## プロトコル仕様

### NTP Protocol Implementation

#### NTPv4 パケット構造

```cpp
struct NtpPacket {
    uint8_t li_vn_mode;             // Leap Indicator, Version, Mode
    uint8_t stratum;                // Stratum level
    uint8_t poll;                   // Poll interval
    int8_t precision;               // Clock precision
    uint32_t root_delay;            // Root delay
    uint32_t root_dispersion;       // Root dispersion
    uint32_t reference_id;          // Reference identifier
    uint32_t reference_timestamp_s; // Reference timestamp (seconds)
    uint32_t reference_timestamp_f; // Reference timestamp (fraction)
    uint32_t origin_timestamp_s;    // Origin timestamp (seconds)
    uint32_t origin_timestamp_f;    // Origin timestamp (fraction)
    uint32_t receive_timestamp_s;   // Receive timestamp (seconds)  
    uint32_t receive_timestamp_f;   // Receive timestamp (fraction)
    uint32_t transmit_timestamp_s;  // Transmit timestamp (seconds)
    uint32_t transmit_timestamp_f;  // Transmit timestamp (fraction)
} __attribute__((packed));
```

#### NTP処理フロー

```
NTP Request Processing:
├─ 1. UDP packet reception (port 123)
├─ 2. Packet validation (size, version, mode)
├─ 3. Timestamp capture (T2: receive time)
├─ 4. Response packet construction
│  ├─ Copy origin timestamp (T1 → T2)
│  ├─ Set reference timestamp (GPS time)
│  ├─ Set stratum level (1 if GPS sync)
│  └─ Set transmit timestamp (T4)
├─ 5. Response transmission
└─ 6. Statistics update
```

### GNSS Protocol Support

#### U-blox UBX Protocol

```cpp
// UBX メッセージクラス
enum class UbxMessageClass : uint8_t {
    NAV = 0x01,    // Navigation results
    RXM = 0x02,    // Receiver manager
    INF = 0x04,    // Information
    ACK = 0x05,    // Acknowledge
    CFG = 0x06,    // Configuration
    MON = 0x0A,    // Monitoring
    TIM = 0x0D     // Timing
};

// 主要メッセージID
enum class UbxNavMessageId : uint8_t {
    PVT = 0x07,    // Position, velocity, time
    SAT = 0x35,    // Satellite information
    TIMEGPS = 0x20 // GPS time solution
};
```

#### QZSS L1S Signal Processing

```cpp
// QZSS L1S災害警報メッセージ構造
struct QzssL1SMessage {
    uint8_t preamble[8];           // プリアンブル
    uint16_t message_type;         // メッセージタイプ
    uint8_t disaster_category;     // 災害カテゴリ
    uint8_t disaster_sub_category; // 災害サブカテゴリ
    uint16_t area_code;           // 地域コード
    char message_text[200];       // 警報メッセージ
    uint32_t timestamp;           // タイムスタンプ
    uint16_t crc;                 // CRC16チェックサム
} __attribute__((packed));

// 災害カテゴリ定義
enum class DisasterCategory : uint8_t {
    EARTHQUAKE = 0x01,    // 地震
    TSUNAMI = 0x02,       // 津波
    VOLCANO = 0x03,       // 火山
    WEATHER = 0x04,       // 気象
    OTHER = 0xFF          // その他
};
```

### HTTP Protocol Extensions

#### Custom Headers

```http
# セキュリティヘッダー
X-GPS-NTP-Version: 1.0.0
X-Frame-Options: DENY
X-Content-Type-Options: nosniff
X-XSS-Protection: 1; mode=block
Content-Security-Policy: default-src 'self'

# API応答ヘッダー
X-GPS-Fix-Status: 3D-Fix
X-Satellites-Count: 12
X-System-Uptime: 3600
X-Memory-Free: 485320
```

## パフォーマンス仕様

### システム性能要件

#### ハードウェア仕様

```
CPU Performance:
├─ Processor: RP2350 (ARM Cortex-M33)
├─ Clock Speed: 150MHz (デュアルコア)
├─ Architecture: 32-bit ARMv8-M
├─ FPU: Single-precision floating point
└─ Cache: 8KB Instruction, 8KB Data

Memory Specification:
├─ SRAM: 512KB (実効容量)
├─ Flash: 4MB (実効容量)
├─ EEPROM Emulation: 4KB (Flash内)
└─ Stack Size: 8KB per core
```

#### 性能測定結果

```json
{
  "memory_usage": {
    "ram": {
      "used": "21KB (4.0%)",
      "free": "491KB (96.0%)",
      "efficiency": "優秀"
    },
    "flash": {
      "used": "494KB (12.2%)",
      "free": "3.4MB (87.8%)",
      "efficiency": "優秀"
    }
  },
  "performance_metrics": {
    "ntp_response_time": "<10ms",
    "web_response_time": "<200ms",
    "gps_processing_time": "<100ms",
    "concurrent_capacity": "491 operations",
    "estimated_cpu_usage": "70%"
  }
}
```

### 処理能力仕様

#### NTP Server Performance

```
NTP Service Capacity:
├─ 同時クライアント数: 50-100 (推奨)
├─ 要求処理レート: 100 req/sec (最大)
├─ 応答時間: <10ms (平均)
├─ 精度: ±1μs (GPS PPS同期時)
└─ Stratum Level: 1 (GPS同期時)

Throughput Metrics:
├─ UDP packet processing: 1000 pps
├─ JSON API requests: 10 req/sec
├─ Configuration updates: 5 req/sec
└─ Metrics collection: 1 sample/sec
```

#### Resource Limits

```cpp
// システムリソース制限
constexpr size_t MAX_CONCURRENT_CLIENTS = 100;
constexpr size_t MAX_LOG_BUFFER_SIZE = 4096;
constexpr size_t MAX_CONFIG_SIZE = 2048;
constexpr size_t MAX_JSON_RESPONSE_SIZE = 8192;
constexpr size_t MAX_PROMETHEUS_RESPONSE_SIZE = 4096;

// タイミング制約
constexpr uint32_t GPS_TIMEOUT_MS = 30000;      // GPS信号タイムアウト
constexpr uint32_t PPS_TIMEOUT_MS = 2000;       // PPS信号タイムアウト
constexpr uint32_t NTP_RESPONSE_TIMEOUT_MS = 5000; // NTP応答タイムアウト
constexpr uint32_t WEB_REQUEST_TIMEOUT_MS = 10000; // Web要求タイムアウト
```

## セキュリティ仕様

### セキュリティ対策

#### 入力検証

```cpp
// 入力サニタイゼーション
class InputValidator {
public:
    static bool validateIPAddress(const char* ip);
    static bool validateHostname(const char* hostname);
    static bool validatePortNumber(uint16_t port);
    static bool sanitizeString(char* input, size_t max_length);
    static bool validateJSONStructure(const char* json);
private:
    static bool isValidCharacter(char c, InputType type);
    static void removeInvalidCharacters(char* str);
};

// 許可文字セット
enum class InputType {
    HOSTNAME,     // a-z, A-Z, 0-9, -, .
    IP_ADDRESS,   // 0-9, .
    JSON_STRING,  // Printable ASCII + escaped chars
    LOG_MESSAGE   // Printable ASCII
};
```

#### レート制限

```cpp
// レート制限実装
class RateLimiter {
private:
    struct ClientRecord {
        uint32_t ip_address;
        uint32_t last_request_time;
        uint16_t request_count;
        bool is_blocked;
    };
    
    static constexpr size_t MAX_CLIENTS = 50;
    static constexpr uint16_t MAX_REQUESTS_PER_MINUTE = 30;
    static constexpr uint32_t BLOCK_DURATION_MS = 60000;
    
public:
    bool allowRequest(uint32_t client_ip);
    void updateClientRecord(uint32_t client_ip);
    void cleanupExpiredRecords();
};
```

#### データ保護

```cpp
// 設定データ保護
struct SecureConfig {
    uint32_t magic;           // "GPSA" マジックナンバー
    uint16_t version;         // 設定バージョン
    uint32_t crc32;           // CRC32チェックサム
    uint32_t timestamp;       // 作成/更新時刻
    SystemConfig data;        // 実際の設定データ
    uint8_t padding[32];      // セキュリティパディング
    
    bool validateIntegrity() const;
    void updateChecksum();
    bool isExpired() const;
};
```

### セキュリティベストプラクティス

#### ネットワークセキュリティ

```
Network Security Measures:
├─ Default Deny Policy
│  ├─ Only essential ports open (80, 123)
│  ├─ No unnecessary services
│  └─ Minimal attack surface
├─ Input Validation
│  ├─ All user inputs sanitized
│  ├─ JSON schema validation
│  └─ SQL injection prevention
├─ Rate Limiting
│  ├─ 30 requests/minute per IP
│  ├─ DDoS protection
│  └─ Automatic blocking
└─ Logging & Monitoring
   ├─ All security events logged
   ├─ Failed access attempts tracked
   └─ Anomaly detection
```

#### Data Integrity

```cpp
// データ整合性検証
namespace Security {
    class IntegrityChecker {
    public:
        static uint32_t calculateCRC32(const void* data, size_t size);
        static bool verifyChecksum(const void* data, size_t size, uint32_t expected);
        static void addSecurityHeaders(char* http_response);
        static bool validateConfigData(const SystemConfig& config);
    };
    
    // セキュリティログ
    enum class SecurityEvent {
        INVALID_LOGIN_ATTEMPT,
        RATE_LIMIT_EXCEEDED,
        CONFIG_CORRUPTION_DETECTED,
        SUSPICIOUS_REQUEST_PATTERN,
        SYSTEM_INTEGRITY_VIOLATION
    };
}
```

## 開発ガイド

### 開発環境セットアップ

#### 必要なツール

```bash
# PlatformIO開発環境
pip install platformio

# プロジェクトセットアップ
git clone <repository_url>
cd ntp_gps_pico2
pio lib install

# ビルド & アップロード
make build
make upload
make uploadfs
```

#### 依存ライブラリ

```ini
[env:pico]
platform = https://github.com/maxgerhardt/platform-raspberrypi.git
board = pico2
framework = arduino

lib_deps = 
    sparkfun/SparkFun u-blox GNSS Arduino Library @ ^2.2.27
    adafruit/Adafruit GFX Library @ ^1.11.5
    bblanchon/ArduinoJson @ ^6.21.3
    
build_flags = 
    -DPIO_FRAMEWORK_ARDUINO_ENABLE_CDC
    -DBOARD_NAME="Raspberry Pi Pico 2"
```

### コーディング規約

#### 命名規則

```cpp
// クラス名: PascalCase
class SystemController {};
class GpsClient {};

// 関数名: camelCase
void initializeSystem();
bool isSystemHealthy();

// 変数名: snake_case
uint32_t system_uptime;
bool gps_fix_valid;

// 定数名: UPPER_SNAKE_CASE
const uint16_t DEFAULT_NTP_PORT = 123;
constexpr size_t MAX_BUFFER_SIZE = 4096;

// 列挙型: PascalCase
enum class SystemState {
    INITIALIZING,
    RUNNING,
    ERROR
};
```

#### コメント規約

```cpp
/**
 * @brief GPS時刻同期処理を実行
 * 
 * GPS受信機からのPPS信号を使用してシステム時刻を同期します。
 * GPS信号が利用不可の場合は内部RTCにフォールバックします。
 * 
 * @param pps_timestamp PPS信号のタイムスタンプ（マイクロ秒）
 * @param gps_time GPS時刻（Unix timestamp）
 * @return true: 同期成功, false: 同期失敗
 * 
 * @note この関数はPPS割り込みハンドラから呼び出されます
 * @warning 処理時間は100μs以内に収める必要があります
 */
bool synchronizeWithGPS(uint64_t pps_timestamp, uint32_t gps_time);
```

### テスト戦略

#### テスト分類

```
Test Hierarchy:
├─ Unit Tests (test_*)
│  ├─ Class method testing
│  ├─ Algorithm verification
│  └─ Error condition handling
├─ Integration Tests (test_integration_*)
│  ├─ Component interaction
│  ├─ Hardware communication
│  └─ System state transitions
├─ System Tests (test_system_*)
│  ├─ End-to-end functionality
│  ├─ Performance benchmarks
│  └─ Load testing
└─ Manual Tests
   ├─ Hardware verification
   ├─ User interface testing
   └─ Real-world scenarios
```

#### テスト実行

```bash
# 全テスト実行
make test

# 特定テスト実行
pio test --filter "test_gps_client"
pio test --filter "test_ntp_server"

# 統合テスト実行
pio test --filter "test_integration"

# パフォーマンステスト
python3 test/benchmark_system_performance.py
```

### デバッグガイド

#### ログレベル設定

```cpp
// ログレベル定義
enum class LogLevel : uint8_t {
    DEBUG = 0,     // 詳細デバッグ情報
    INFO = 1,      // 一般的な情報
    NOTICE = 2,    // 重要な情報
    WARNING = 3,   // 警告メッセージ
    ERROR = 4,     // エラーメッセージ
    CRITICAL = 5,  // 致命的エラー
    ALERT = 6,     // アラート
    EMERGENCY = 7  // 緊急事態
};
```

#### デバッグ出力

```cpp  
// デバッグマクロ使用例
LOG_DEBUG("GPS", "Scanning satellites: GPS=%d, GLONASS=%d", gps_count, glonass_count);
LOG_INFO("NTP", "Client request from %s", client_ip);
LOG_WARNING("SYSTEM", "Memory usage high: %d%%", memory_usage_percent);
LOG_ERROR("NETWORK", "W5500 communication failed: error code %d", error_code);
```

### 拡張ガイド

#### 新機能追加手順

```
Feature Addition Process:
├─ 1. Requirements Definition
│  ├─ Feature specification
│  ├─ API design
│  └─ Test plan
├─ 2. Design Phase
│  ├─ Architecture impact analysis
│  ├─ Interface definition
│  └─ Resource requirement assessment
├─ 3. Implementation
│  ├─ TDD approach (test first)
│  ├─ Code implementation
│  └─ Documentation update
├─ 4. Testing
│  ├─ Unit tests
│  ├─ Integration tests
│  └─ System tests
└─ 5. Integration
   ├─ Code review
   ├─ Performance validation
   └─ Production deployment
```

#### カスタムサービス作成

```cpp
// カスタムサービステンプレート
class CustomService : public ServiceBase {
public:
    CustomService(const char* name) : name_(name) {}
    
    bool initialize() override {
        // 初期化処理
        LOG_INFO("CUSTOM", "%s service initializing...", name_);
        return true;
    }
    
    void update() override {
        // 定期更新処理
        // 注意: 100ms以内で完了すること
    }
    
    bool isHealthy() const override {
        // ヘルスチェック
        return health_status_;
    }
    
    void shutdown() override {
        // シャットダウン処理
        LOG_INFO("CUSTOM", "%s service shutting down...", name_);
    }
    
    const char* getName() const override {
        return name_;
    }
    
private:
    const char* name_;
    bool health_status_ = true;
};
```

### トラブルシューティング

#### 一般的な問題と解決法

```
Common Issues:
├─ Build Errors
│  ├─ Library version conflicts
│  ├─ Missing dependencies
│  └─ Compiler configuration
├─ Runtime Errors
│  ├─ Memory allocation failures
│  ├─ Hardware communication timeouts
│  └─ Configuration corruption
├─ Performance Issues
│  ├─ High CPU usage
│  ├─ Memory leaks
│  └─ Network bottlenecks
└─ Integration Problems
   ├─ GPS signal acquisition
   ├─ Network connectivity
   └─ Time synchronization
```

#### デバッグツール

```bash
# シリアル出力監視
make monitor

# メモリ使用量確認
pio run --target size

# 静的解析
pio check

# プロファイリング
python3 test/benchmark_system_performance.py
```

---

## 付録

### リビジョン履歴

| バージョン | 日付 | 変更内容 |
|-----------|------|----------|
| 1.0.0 | 2025-07-30 | 初版リリース |

### 参考文献

- [RFC 5905 - Network Time Protocol Version 4](https://tools.ietf.org/html/rfc5905)
- [u-blox ZED-F9T Integration Manual](https://www.u-blox.com/en/docs/UBX-19009093)
- [RP2350 Datasheet](https://datasheets.raspberrypi.org/rp2350/rp2350-datasheet.pdf)
- [W5500 Datasheet](https://docs.wiznet.io/Product/iEthernet/W5500/overview)

### 技術サポート

**開発者**: GPS NTP Server Development Team  
**プロジェクト**: GPS NTP Server v1.0  
**リポジトリ**: GitHub Repository  
**ドキュメント**: `docs/`フォルダ参照
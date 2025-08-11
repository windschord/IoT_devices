#ifndef CONFIG_DEFAULTS_H
#define CONFIG_DEFAULTS_H

#include "Constants.h"
#include <Arduino.h>

// Forward declaration
struct SystemConfig;

/**
 * @file ConfigDefaults.h
 * @brief システム設定のデフォルト値一元管理
 * 
 * 全ての設定デフォルト値をconstexprで定義し、型安全性とコンパイル時最適化を実現
 */

namespace ConfigDefaults {

// =============================================================================
// ネットワーク設定デフォルト値
// =============================================================================
namespace Network {
    // ホスト名設定
    constexpr const char* HOSTNAME = "gps-ntp-server";
    
    // IP設定 (0 = DHCP使用)
    constexpr uint32_t IP_ADDRESS = 0;          // DHCP使用
    constexpr uint32_t NETMASK = 0;             // DHCP使用
    constexpr uint32_t GATEWAY = 0;             // DHCP使用
    constexpr uint32_t DNS_SERVER = 0;          // DHCP使用
    
    // ポート設定
    constexpr uint16_t NTP_SERVER_PORT = Constants::Network::NTP_SERVER_PORT;
    constexpr uint16_t HTTP_SERVER_PORT = Constants::Network::HTTP_SERVER_PORT;
    constexpr uint16_t SYSLOG_DEFAULT_PORT = Constants::Network::SYSLOG_DEFAULT_PORT;
}

// =============================================================================
// ログ設定デフォルト値
// =============================================================================
namespace Logging {
    // Syslogサーバー設定
    constexpr const char* SYSLOG_SERVER = "";  // 空文字列 = syslog無効
    constexpr uint16_t SYSLOG_PORT = Network::SYSLOG_DEFAULT_PORT;
    constexpr uint8_t LOG_LEVEL = Constants::Status::LOG_LEVEL_INFO;
    
    // ローカルログ設定
    constexpr bool CONSOLE_LOG_ENABLED = true;
    constexpr bool SERIAL_LOG_ENABLED = true;
    constexpr uint8_t MAX_LOG_ENTRIES = Constants::Limits::MAX_LOG_ENTRIES;
}

// =============================================================================
// 監視設定デフォルト値
// =============================================================================
namespace Monitoring {
    // Prometheus設定
    constexpr bool PROMETHEUS_ENABLED = true;
    constexpr uint16_t PROMETHEUS_PORT = Network::HTTP_SERVER_PORT;  // Same as web server
    
    // システム監視設定
    constexpr uint32_t SYSTEM_MONITOR_INTERVAL = Constants::Timing::SYSTEM_MONITOR_INTERVAL;
    constexpr uint32_t NETWORK_STATUS_CHECK_INTERVAL = Constants::Timing::NETWORK_STATUS_CHECK_INTERVAL;
    constexpr uint32_t WATCHDOG_TIMEOUT = Constants::Limits::WATCHDOG_TIMEOUT_MS;
}

// =============================================================================
// GNSS設定デフォルト値
// =============================================================================
namespace GNSS {
    // コンステレーション設定 (全て有効)
    constexpr bool GPS_ENABLED = true;
    constexpr bool GLONASS_ENABLED = true;
    constexpr bool GALILEO_ENABLED = true;
    constexpr bool BEIDOU_ENABLED = true;
    constexpr bool QZSS_ENABLED = true;
    constexpr bool QZSS_L1S_ENABLED = true;    // QZSS L1S災害警報有効
    
    // 更新レート設定
    constexpr uint8_t GNSS_UPDATE_RATE = 1;    // 1Hz (標準)
    
    // 災害警報設定
    constexpr uint8_t DISASTER_ALERT_PRIORITY = 2;  // 0=low, 1=medium, 2=high
    
    // GPS品質閾値
    constexpr float HDOP_THRESHOLD_EXCELLENT = Constants::Quality::TIME_QUALITY_EXCELLENT_US / 1000.0f;
    constexpr float HDOP_THRESHOLD_GOOD = Constants::Quality::TIME_QUALITY_GOOD_US / 1000.0f;
    constexpr float HDOP_THRESHOLD_MODERATE = Constants::Quality::TIME_QUALITY_MODERATE_US / 1000.0f;
}

// =============================================================================
// NTPサーバー設定デフォルト値
// =============================================================================
namespace NTP {
    // NTPサーバー基本設定
    constexpr bool NTP_ENABLED = true;
    constexpr uint16_t NTP_PORT = Network::NTP_SERVER_PORT;
    constexpr uint8_t NTP_STRATUM = Constants::Quality::NTP_STRATUM_REFERENCE_CLOCK;  // GPS = Stratum 1
    
    // NTP精度設定
    constexpr uint32_t NTP_PRECISION_MICROSECONDS = 1000;  // 1ms precision target
    constexpr uint8_t NTP_MAX_CLIENTS = 16;                // Maximum concurrent NTP clients
    constexpr uint32_t NTP_RATE_LIMIT_PER_MINUTE = 60;     // Rate limiting
    
    // NTP統計設定
    constexpr bool NTP_STATISTICS_ENABLED = true;
    constexpr uint32_t NTP_STATS_RESET_INTERVAL = Constants::Timing::SECS_PER_HOUR; // 1 hour
}

// =============================================================================
// システム設定デフォルト値
// =============================================================================
namespace System {
    // 自動再起動設定
    constexpr bool AUTO_RESTART_ENABLED = false;         // デフォルトでは無効
    constexpr uint32_t RESTART_INTERVAL_HOURS = 24;      // 24時間毎
    
    // デバッグ設定
    constexpr bool DEBUG_ENABLED = false;                // 本番環境ではfalse
    constexpr bool HARDWARE_TEST_MODE = false;           // ハードウェアテストモード
    
    // 設定管理
    constexpr uint32_t CONFIG_VERSION = 1;               // 設定バージョン
    constexpr bool CONFIG_AUTO_SAVE = true;              // 設定自動保存
    constexpr uint32_t CONFIG_SAVE_INTERVAL = 300;       // 5分毎自動保存
}

// =============================================================================
// ディスプレイ設定デフォルト値
// =============================================================================
namespace Display {
    // 基本ディスプレイ設定
    constexpr bool DISPLAY_ENABLED = true;
    constexpr uint8_t DISPLAY_BRIGHTNESS = 255;          // 最大輝度
    constexpr bool DISPLAY_AUTO_SLEEP = true;            // 自動スリープ有効
    constexpr uint8_t DISPLAY_SLEEP_TIMEOUT = Constants::Limits::DISPLAY_SLEEP_TIMEOUT_COUNT;
    
    // 表示モード設定
    constexpr uint8_t DEFAULT_DISPLAY_MODE = 0;          // ステータス表示モード
    constexpr uint32_t MODE_SWITCH_INTERVAL = 5000;      // 5秒毎自動切替（無効化可能）
    constexpr bool AUTO_MODE_SWITCH = false;             // 自動モード切替無効
    
    // アニメーション設定
    constexpr bool ANIMATIONS_ENABLED = true;
    constexpr uint32_t ANIMATION_SPEED = Constants::Animation::SCROLL_SPEED_MS;
}

// =============================================================================
// ハードウェア設定デフォルト値  
// =============================================================================
namespace Hardware {
    // I2C設定
    constexpr uint32_t I2C_CLOCK_SPEED = 100000;         // 100kHz (安定動作)
    constexpr uint8_t I2C_TIMEOUT_MS = 100;              // 100ms timeout
    constexpr uint8_t I2C_MAX_RETRIES = Constants::Buffer::I2C_MAX_RETRY;
    
    // SPI設定（W5500 Ethernet）
    constexpr uint32_t SPI_CLOCK_SPEED = 8000000;        // 8MHz
    
    // ボタン設定
    constexpr uint32_t BUTTON_DEBOUNCE_MS = Constants::Limits::DEBOUNCE_DELAY_MS;
    constexpr uint32_t BUTTON_SHORT_PRESS_MS = Constants::Limits::SHORT_PRESS_THRESHOLD_MS;
    constexpr uint32_t BUTTON_LONG_PRESS_MS = Constants::Limits::LONG_PRESS_THRESHOLD_MS;
    
    // LED設定
    constexpr bool STATUS_LEDS_ENABLED = true;
    constexpr uint8_t LED_BRIGHTNESS = 128;              // 50% brightness
    constexpr uint32_t LED_BLINK_INTERVAL = Constants::Animation::BLINK_INTERVAL_MS;
}

// =============================================================================
// セキュリティ設定デフォルト値
// =============================================================================
namespace Security {
    // Web認証設定（現在は基本認証のみ）
    constexpr bool WEB_AUTH_ENABLED = false;             // デフォルト無効
    constexpr const char* WEB_USERNAME = "admin";
    constexpr const char* WEB_PASSWORD = "admin";        // 本番環境では変更必須
    
    // アクセス制限
    constexpr uint16_t MAX_HTTP_CONNECTIONS = Constants::Limits::MAX_HTTP_CONNECTIONS;
    constexpr uint32_t HTTP_TIMEOUT = Constants::Limits::HTTP_TIMEOUT_MS;
    constexpr uint32_t RATE_LIMIT_REQUESTS_PER_MINUTE = 60;
    
    // データ整合性
    constexpr bool CONFIG_CHECKSUM_VALIDATION = true;    // 設定データチェックサム検証
    constexpr bool STORAGE_CRC_VALIDATION = true;        // ストレージCRC検証
}

// =============================================================================
// 電力管理設定デフォルト値
// =============================================================================
namespace Power {
    // 電圧監視
    constexpr bool VOLTAGE_MONITORING_ENABLED = true;
    constexpr float VOLTAGE_THRESHOLD_WARNING = 3.1f;    // 3.1V警告レベル
    constexpr float VOLTAGE_THRESHOLD_CRITICAL = 2.9f;   // 2.9V臨界レベル
    
    // 低電力モード
    constexpr bool LOW_POWER_MODE_ENABLED = false;       // デフォルト無効
    constexpr uint32_t IDLE_SLEEP_DURATION_MS = 100;     // アイドル時スリープ時間
    
    // ウォッチドッグ
    constexpr bool WATCHDOG_ENABLED = true;
    constexpr uint32_t WATCHDOG_TIMEOUT = Monitoring::WATCHDOG_TIMEOUT;
}

/**
 * @brief 設定値の妥当性チェック用の制限値
 */
namespace ValidationLimits {
    // Network limits
    constexpr size_t HOSTNAME_MIN_LENGTH = 1;
    constexpr size_t HOSTNAME_MAX_LENGTH = Constants::Buffer::HOSTNAME_MAX_LENGTH - 1;
    constexpr size_t SYSLOG_SERVER_MAX_LENGTH = Constants::Buffer::SYSLOG_SERVER_MAX_LENGTH - 1;
    
    // GNSS limits
    constexpr uint8_t GNSS_UPDATE_RATE_MIN = Constants::Limits::GNSS_UPDATE_RATE_MIN;
    constexpr uint8_t GNSS_UPDATE_RATE_MAX = Constants::Limits::GNSS_UPDATE_RATE_MAX;
    
    // NTP limits
    constexpr uint8_t NTP_STRATUM_MIN = 1;
    constexpr uint8_t NTP_STRATUM_MAX = 15;
    constexpr uint16_t NTP_PORT_MIN = 1;
    constexpr uint16_t NTP_PORT_MAX = 65535;
    
    // Log level limits
    constexpr uint8_t LOG_LEVEL_MIN = Constants::Status::LOG_LEVEL_DEBUG;
    constexpr uint8_t LOG_LEVEL_MAX = Constants::Status::LOG_LEVEL_ERROR;
    
    // Disaster alert priority limits
    constexpr uint8_t DISASTER_ALERT_PRIORITY_MIN = 0;
    constexpr uint8_t DISASTER_ALERT_PRIORITY_MAX = 2;
} // namespace ValidationLimits

} // namespace ConfigDefaults

// =============================================================================
// デフォルト設定構造体の生成関数（宣言のみ）
// =============================================================================

/**
 * @brief SystemConfig構造体にデフォルト値を設定
 * @return デフォルト値で初期化されたSystemConfig構造体
 * 
 * 注：実装はConfigManager.cppで行い、循環依存を回避
 */
SystemConfig createDefaultSystemConfig();

#endif // CONFIG_DEFAULTS_H
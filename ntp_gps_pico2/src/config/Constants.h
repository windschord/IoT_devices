#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <Arduino.h>

/**
 * @file Constants.h
 * @brief 全システム定数定義ファイル
 * 
 * 全てのマジックナンバーを統一管理し、constexprによるコンパイル時最適化を実現
 * 既存のライブラリ定数との衝突を避けるため、namespace内で独自の名前を使用
 */

namespace SystemConstants {

// =============================================================================
// ハードウェア定数
// =============================================================================
namespace Hardware {
    // GPIO Pin Assignments (既存のHardwareConfig.hからの定数を参照)
    constexpr uint8_t GPS_PPS = 8;
    constexpr uint8_t GPS_SDA = 6;      // I2C1 bus shared with RTC
    constexpr uint8_t GPS_SCL = 7;      // I2C1 bus shared with RTC
    constexpr uint8_t BTN_DISPLAY = 11;
    
    // LED Pin Assignments
    constexpr uint8_t LED_GNSS_FIX = 4;    // GNSS Fix Status LED (Green)
    constexpr uint8_t LED_NETWORK = 5;     // Network Status LED (Blue)
    constexpr uint8_t LED_ERROR = 14;      // Error Status LED (Red)
    constexpr uint8_t LED_PPS = 15;        // PPS Status LED (Yellow)
    constexpr uint8_t LED_ONBOARD = 25;    // Onboard LED
    
    // W5500 Ethernet Module Pins
    constexpr uint8_t W5500_RST = 20;
    constexpr uint8_t W5500_INT = 21;
    constexpr uint8_t W5500_CS = 17;
    
    // OLED Display Configuration
    constexpr uint8_t SCREEN_WIDTH = 128;       // OLED display width, in pixels
    constexpr uint8_t SCREEN_HEIGHT = 64;       // OLED display height, in pixels
    constexpr int8_t OLED_RESET = -1;          // Reset pin # (or -1 if sharing Arduino reset pin)
    constexpr uint8_t SCREEN_ADDRESS = 0x3D;    // 0x3D for 128x64, 0x3C for 128x32
}

// =============================================================================
// ネットワーク定数
// =============================================================================
namespace Network {
    // NTP Configuration
    constexpr uint16_t NTP_SERVER_PORT = 123;         // NTP standard port
    constexpr uint8_t NTP_PACKET_LENGTH = 48;         // NTP packet size
    
    // Web Server Configuration
    constexpr uint16_t HTTP_SERVER_PORT = 80;         // HTTP standard port
    constexpr uint16_t PROMETHEUS_METRICS_PORT = 80;  // Prometheus metrics port (same as web)
    
    // Syslog Configuration
    constexpr uint16_t SYSLOG_DEFAULT_PORT = 514;     // Syslog standard port
    
    // Serial Communication
    constexpr uint32_t SERIAL_BAUDRATE = 9600;
    
    // Default MAC Address (Generated from: https://www.hellion.org.uk/cgi-bin/randmac.pl)
    constexpr uint8_t DEFAULT_MAC[6] = {0x6e, 0xc9, 0x4c, 0x32, 0x3a, 0xf6};
}

// =============================================================================
// システムタイミング定数
// =============================================================================
namespace Timing {
    // タイムスタンプ関連
    constexpr uint64_t MICROS_PER_SEC = 1000000ULL;
    constexpr uint32_t MILLIS_PER_SEC = 1000;
    constexpr uint32_t SECS_PER_MIN = 60;
    constexpr uint32_t MINS_PER_HOUR = 60;
    constexpr uint32_t HOURS_PER_DAY = 24;
    constexpr uint32_t SECS_PER_HOUR = 3600;
    constexpr uint32_t SECS_IN_DAY = 86400;
    
    // NTP Epoch Constants
    constexpr uint32_t NTP_EPOCH_OFFSET = 2208988800UL; // Seconds between 1900-1970
    constexpr uint64_t NTP_FRACTION_DIVISOR = 4294967296ULL; // 2^32 for NTP fraction conversion
    
    // System Update Intervals (ms)
    constexpr uint32_t HIGH_PRIORITY_INTERVAL = 10;    // 10ms for critical operations
    constexpr uint32_t MEDIUM_PRIORITY_INTERVAL = 100; // 100ms for regular operations
    constexpr uint32_t LOW_PRIORITY_INTERVAL = 1000;   // 1000ms for low priority operations
    
    // Hardware Update Intervals (ms)
    constexpr uint32_t DISPLAY_UPDATE_MIN_INTERVAL = 100;           // 100ms minimum between I2C updates
    constexpr uint32_t GPS_UPDATE_INTERVAL = 1000;                 // 1000ms GPS data update
    constexpr uint32_t NETWORK_STATUS_CHECK_INTERVAL = 5000;       // 5000ms network status check
    constexpr uint32_t SYSTEM_MONITOR_INTERVAL = 10000;            // 10000ms system monitoring
}

// =============================================================================
// バッファサイズ定数
// =============================================================================
namespace Buffer {
    // String Buffer Sizes
    constexpr size_t HOSTNAME_MAX_LENGTH = 32;
    constexpr size_t SYSLOG_SERVER_MAX_LENGTH = 64;
    constexpr size_t LOG_MESSAGE_MAX_LENGTH = 256;
    constexpr size_t JSON_BUFFER_SIZE = 1024;
    constexpr size_t HTTP_HEADER_BUFFER_SIZE = 512;
    constexpr size_t HTTP_RESPONSE_BUFFER_SIZE = 2048;
    constexpr size_t FILE_PATH_MAX_LENGTH = 64;
    
    // I2C Buffer Configuration
    constexpr uint8_t I2C_BUFFER_SIZE = 32;    // I2Cバッファオーバーフロー防止
    constexpr uint8_t I2C_MAX_RETRY = 3;       // I2C再試行回数
    
    // Storage Buffer Sizes
    constexpr size_t STORAGE_SECTOR_SIZE = 4096;    // 4KB storage sector
    constexpr size_t CONFIG_DATA_MAX_SIZE = 1024;   // 1KB config data
}

// =============================================================================
// 閾値・制限値定数
// =============================================================================
namespace Limits {
    // Button Configuration
    constexpr uint32_t DEBOUNCE_DELAY_MS = 20;         // 20ms button debounce
    constexpr uint32_t SHORT_PRESS_THRESHOLD_MS = 100; // 100ms short press threshold
    constexpr uint32_t LONG_PRESS_THRESHOLD_MS = 2000; // 2000ms long press threshold
    constexpr uint32_t FACTORY_RESET_DURATION_MS = 5000; // 5000ms factory reset duration
    
    // Display Configuration
    constexpr uint8_t DISPLAY_SLEEP_TIMEOUT_COUNT = 30; // 30 update cycles (≈30 seconds)
    constexpr uint8_t DISPLAY_MAX_MODES = 5;            // Maximum display modes
    
    // Network Limits
    constexpr uint16_t MAX_HTTP_CONNECTIONS = 4;        // Maximum concurrent HTTP connections
    constexpr uint32_t HTTP_TIMEOUT_MS = 10000;         // 10 second HTTP timeout
    constexpr uint32_t NETWORK_RECONNECT_DELAY_MS = 5000; // 5 second network reconnect delay
    
    // GPS/GNSS Limits
    constexpr uint8_t GNSS_UPDATE_RATE_MIN = 1;         // 1Hz minimum update rate
    constexpr uint8_t GNSS_UPDATE_RATE_MAX = 10;        // 10Hz maximum update rate
    constexpr uint8_t GPS_SATELLITES_MAX = 32;          // Maximum GPS satellites to track
    constexpr float GPS_HDOP_EXCELLENT = 1.0f;          // Excellent GPS accuracy threshold
    constexpr float GPS_HDOP_GOOD = 2.0f;               // Good GPS accuracy threshold
    constexpr float GPS_HDOP_MODERATE = 5.0f;           // Moderate GPS accuracy threshold
    
    // System Resource Limits
    constexpr uint8_t MAX_SERVICES = 16;                // Maximum services in container
    constexpr uint32_t WATCHDOG_TIMEOUT_MS = 30000;     // 30 second watchdog timeout
    constexpr uint8_t MAX_LOG_ENTRIES = 100;            // Maximum log entries in memory
}

// =============================================================================
// 品質・精度定数
// =============================================================================
namespace Quality {
    // Time Quality Thresholds
    constexpr uint32_t TIME_QUALITY_EXCELLENT_US = 1000;    // 1ms excellent time quality
    constexpr uint32_t TIME_QUALITY_GOOD_US = 10000;        // 10ms good time quality  
    constexpr uint32_t TIME_QUALITY_MODERATE_US = 100000;   // 100ms moderate time quality
    
    // NTP Stratum Levels
    constexpr uint8_t NTP_STRATUM_REFERENCE_CLOCK = 1;      // Reference clock (GPS)
    constexpr uint8_t NTP_STRATUM_PRIMARY_SERVER = 2;       // Primary time server
    constexpr uint8_t NTP_STRATUM_SECONDARY_SERVER = 3;     // Secondary time server
    constexpr uint8_t NTP_STRATUM_UNSYNCHRONIZED = 16;      // Unsynchronized
    
    // Signal Quality Thresholds
    constexpr uint8_t GPS_SIGNAL_STRENGTH_EXCELLENT = 40;   // dBHz
    constexpr uint8_t GPS_SIGNAL_STRENGTH_GOOD = 35;        // dBHz
    constexpr uint8_t GPS_SIGNAL_STRENGTH_MODERATE = 30;    // dBHz
    constexpr uint8_t GPS_SIGNAL_STRENGTH_POOR = 25;        // dBHz
}

// =============================================================================
// エラー・ステータスコード定数
// =============================================================================
namespace Status {
    // HTTP Status Codes
    constexpr uint16_t HTTP_OK = 200;
    constexpr uint16_t HTTP_BAD_REQUEST = 400;
    constexpr uint16_t HTTP_NOT_FOUND = 404;
    constexpr uint16_t HTTP_INTERNAL_SERVER_ERROR = 500;
    
    // System Error Codes
    constexpr uint8_t ERROR_NONE = 0;
    constexpr uint8_t ERROR_INITIALIZATION_FAILED = 1;
    constexpr uint8_t ERROR_HARDWARE_FAILURE = 2;
    constexpr uint8_t ERROR_NETWORK_FAILURE = 3;
    constexpr uint8_t ERROR_GPS_FAILURE = 4;
    constexpr uint8_t ERROR_STORAGE_FAILURE = 5;
    constexpr uint8_t ERROR_CONFIGURATION_INVALID = 6;
    
    // Log Levels
    constexpr uint8_t LOG_LEVEL_DEBUG = 0;
    constexpr uint8_t LOG_LEVEL_INFO = 1;
    constexpr uint8_t LOG_LEVEL_WARN = 2;
    constexpr uint8_t LOG_LEVEL_ERROR = 3;
    
    // System States
    constexpr uint8_t SYSTEM_STATE_INITIALIZING = 0;
    constexpr uint8_t SYSTEM_STATE_RUNNING = 1;
    constexpr uint8_t SYSTEM_STATE_ERROR = 2;
    constexpr uint8_t SYSTEM_STATE_SHUTDOWN = 3;
}

// =============================================================================
// 数学・計算定数
// =============================================================================
namespace Math {
    // 進数変換
    constexpr uint8_t BINARY_BASE = 2;
    constexpr uint8_t DECIMAL_BASE = 10;
    constexpr uint8_t HEX_BASE = 16;
    
    // ビット操作
    constexpr uint8_t BITS_PER_BYTE = 8;
    constexpr uint8_t BITS_PER_WORD = 16;
    constexpr uint8_t BITS_PER_DWORD = 32;
    constexpr uint64_t BITS_PER_QWORD = 64;
    
    // 文字コード
    constexpr uint8_t ASCII_SPACE = 32;
    constexpr uint8_t ASCII_ZERO = 48;
    constexpr uint8_t ASCII_A = 65;
    constexpr uint8_t ASCII_a = 97;
    
    // パーセンテージ計算
    constexpr uint8_t PERCENTAGE_MAX = 100;
    constexpr uint16_t PERCENTAGE_SCALE = 10000; // 0.01% precision
}

// =============================================================================
// アニメーション・UI定数
// =============================================================================
namespace Animation {
    // Progress Bar Configuration
    constexpr uint8_t PROGRESS_BAR_WIDTH = 100;
    constexpr uint8_t PROGRESS_BAR_HEIGHT = 8;
    constexpr uint8_t PROGRESS_CHAR_FILLED = '#';
    constexpr uint8_t PROGRESS_CHAR_EMPTY = '-';
    
    // Display Animation Timing
    constexpr uint32_t FADE_IN_DURATION_MS = 500;
    constexpr uint32_t FADE_OUT_DURATION_MS = 500;
    constexpr uint32_t SCROLL_SPEED_MS = 100;
    constexpr uint32_t BLINK_INTERVAL_MS = 500;
}

} // namespace SystemConstants

// 後方互換性のための短縮エイリアス
namespace Constants = SystemConstants;

#endif // CONSTANTS_H
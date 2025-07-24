#include "ConfigManager.h"
#include "HardwareConfig.h"
#include "logging.h"
#include <ArduinoJson.h>

ConfigManager::ConfigManager() : 
    configValid(false),
    storageHal(&g_storage_hal) {
}

void ConfigManager::init() {
    LOG_INFO_MSG("ConfigManager: 初期化開始...");
    
    // Storage HAL初期化
    if (!storageHal->initialize()) {
        LOG_ERR_MSG("ConfigManager: Storage HAL初期化失敗");
        loadDefaults();
        configValid = true;
        return;
    }
    
    // 設定読み込み試行
    if (!loadConfig()) {
        LOG_WARN_MSG("ConfigManager: 設定読み込み失敗、デフォルト設定使用");
        loadDefaults();
        saveConfig(); // デフォルト設定を保存
    }
    
    configValid = true;
    LOG_INFO_MSG("ConfigManager: 初期化完了");
    printConfig();
}

bool ConfigManager::loadConfig() {
    StorageResult result = storageHal->readConfig(&currentConfig, sizeof(SystemConfig));
    
    switch (result) {
        case STORAGE_SUCCESS:
            if (validateConfig(currentConfig)) {
                LOG_INFO_MSG("ConfigManager: 設定読み込み成功");
                return true;
            } else {
                LOG_ERR_MSG("ConfigManager: 設定検証失敗");
                return false;
            }
            
        case STORAGE_ERROR_MAGIC:
            LOG_WARN_MSG("ConfigManager: 初回起動 - 設定が存在しません");
            return false;
            
        case STORAGE_ERROR_CRC:
            LOG_ERR_MSG("ConfigManager: 設定データ破損 (CRC32エラー)");
            return false;
            
        case STORAGE_ERROR_SIZE:
            LOG_ERR_MSG("ConfigManager: 設定サイズ不一致");
            return false;
            
        default:
            LOG_ERR_MSG("ConfigManager: 設定読み込みエラー (%d)", result);
            return false;
    }
}

bool ConfigManager::saveConfig() {
    if (!validateConfig(currentConfig)) {
        LOG_ERR_MSG("ConfigManager: 無効な設定のため保存中止");
        return false;
    }
    
    // タイムスタンプ更新
    currentConfig.config_version = 1;
    
    StorageResult result = storageHal->writeConfig(&currentConfig, sizeof(SystemConfig));
    
    if (result == STORAGE_SUCCESS) {
        LOG_INFO_MSG("ConfigManager: 設定保存完了");
        return true;
    } else {
        LOG_ERR_MSG("ConfigManager: 設定保存失敗 (%d)", result);
        return false;
    }
}

void ConfigManager::loadDefaults() {
    LOG_INFO_MSG("ConfigManager: デフォルト設定読み込み...");
    memset(&currentConfig, 0, sizeof(SystemConfig));
    
    // Network Configuration
    strcpy(currentConfig.hostname, "gps-ntp-server");
    currentConfig.ip_address = 0;  // DHCP by default
    currentConfig.netmask = 0;
    currentConfig.gateway = 0;
    currentConfig.dns_server = 0;
    
    // Logging Configuration
    strcpy(currentConfig.syslog_server, "192.168.1.100");
    currentConfig.syslog_port = 514;
    currentConfig.log_level = 1;  // INFO level
    
    // Monitoring
    currentConfig.prometheus_enabled = true;
    currentConfig.prometheus_port = 80;  // Same as web server
    
    // GNSS Configuration
    currentConfig.gps_enabled = true;
    currentConfig.glonass_enabled = true;
    currentConfig.galileo_enabled = true;
    currentConfig.beidou_enabled = false;   // Optional for power saving
    currentConfig.qzss_enabled = true;      // Important for Japan
    currentConfig.qzss_l1s_enabled = true;  // Disaster alert system
    currentConfig.gnss_update_rate = 1;     // 1Hz default
    currentConfig.disaster_alert_priority = 2; // High priority
    
    // NTP Server Configuration
    currentConfig.ntp_enabled = true;
    currentConfig.ntp_port = 123;
    currentConfig.ntp_stratum = 1;
    
    // System Configuration
    currentConfig.auto_restart_enabled = false;
    currentConfig.restart_interval = 24;   // 24 hours
    currentConfig.debug_enabled = false;
    
    // Configuration metadata
    currentConfig.config_version = 1;
    
    LOG_INFO_MSG("ConfigManager: デフォルト設定設定完了");
}

void ConfigManager::resetToDefaults() {
    LOG_WARN_MSG("ConfigManager: 工場出荷時リセット実行...");
    
    // Storage HAL経由で工場出荷時リセット
    StorageResult result = storageHal->factoryReset();
    if (result != STORAGE_SUCCESS) {
        LOG_ERR_MSG("ConfigManager: ストレージリセット失敗 (%d)", result);
    }
    
    // デフォルト設定読み込み
    loadDefaults();
    
    // 新しい設定を保存
    if (saveConfig()) {
        LOG_INFO_MSG("ConfigManager: 工場出荷時リセット完了");
    } else {
        LOG_ERR_MSG("ConfigManager: 工場出荷時リセット - 設定保存失敗");
    }
}

bool ConfigManager::setConfig(const SystemConfig& newConfig) {
    if (!validateConfig(newConfig)) {
        LOG_ERR_MSG("ConfigManager: 無効な設定");
        return false;
    }
    
    currentConfig = newConfig;
    return saveConfig();
}

bool ConfigManager::validateConfig(const SystemConfig& config) const {
    // 基本的なバリデーション
    
    // ホスト名チェック
    if (strlen(config.hostname) == 0 || strlen(config.hostname) >= sizeof(config.hostname)) {
        LOG_ERR_MSG("ConfigManager: 無効なホスト名");
        return false;
    }
    
    // ログレベルチェック
    if (config.log_level > 7) {  // 0=DEBUG to 7=EMERGENCY
        LOG_ERR_MSG("ConfigManager: 無効なログレベル (%d)", config.log_level);
        return false;
    }
    
    // Syslogポートチェック
    if (config.syslog_port == 0 || config.syslog_port > 65535) {
        LOG_ERR_MSG("ConfigManager: 無効なSyslogポート (%d)", config.syslog_port);
        return false;
    }
    
    // GNSS更新レートチェック
    if (config.gnss_update_rate == 0 || config.gnss_update_rate > 10) {
        LOG_ERR_MSG("ConfigManager: 無効なGNSS更新レート (%d)", config.gnss_update_rate);
        return false;
    }
    
    // NTPポートチェック
    if (config.ntp_port == 0 || config.ntp_port > 65535) {
        LOG_ERR_MSG("ConfigManager: 無効なNTPポート (%d)", config.ntp_port);
        return false;
    }
    
    // 設定バージョンチェック
    if (config.config_version == 0) {
        LOG_ERR_MSG("ConfigManager: 無効な設定バージョン");
        return false;
    }
    
    return true;
}

// Individual setting setters with validation
bool ConfigManager::setHostname(const char* hostname) {
    if (!hostname || strlen(hostname) == 0 || strlen(hostname) >= sizeof(currentConfig.hostname)) {
        return false;
    }
    strcpy(currentConfig.hostname, hostname);
    return saveConfig();
}

bool ConfigManager::setNetworkConfig(uint32_t ip, uint32_t netmask, uint32_t gateway) {
    currentConfig.ip_address = ip;
    currentConfig.netmask = netmask;
    currentConfig.gateway = gateway;
    return saveConfig();
}

bool ConfigManager::setSyslogConfig(const char* server, uint16_t port) {
    if (!server || strlen(server) >= sizeof(currentConfig.syslog_server) || port == 0) {
        return false;
    }
    strcpy(currentConfig.syslog_server, server);
    currentConfig.syslog_port = port;
    return saveConfig();
}

bool ConfigManager::setLogLevel(uint8_t level) {
    if (level > 7) {
        return false;
    }
    currentConfig.log_level = level;
    return saveConfig();
}

bool ConfigManager::setPrometheusEnabled(bool enabled) {
    currentConfig.prometheus_enabled = enabled;
    return saveConfig();
}

bool ConfigManager::setGnssConstellations(bool gps, bool glonass, bool galileo, bool beidou, bool qzss) {
    currentConfig.gps_enabled = gps;
    currentConfig.glonass_enabled = glonass;
    currentConfig.galileo_enabled = galileo;
    currentConfig.beidou_enabled = beidou;
    currentConfig.qzss_enabled = qzss;
    return saveConfig();
}

bool ConfigManager::setGnssUpdateRate(uint8_t rate) {
    if (rate == 0 || rate > 10) {
        return false;
    }
    currentConfig.gnss_update_rate = rate;
    return saveConfig();
}

// JSON serialization for web interface
String ConfigManager::configToJson() const {
    DynamicJsonDocument doc(2048);
    
    // Network
    doc["hostname"] = currentConfig.hostname;
    doc["ip_address"] = currentConfig.ip_address;
    doc["netmask"] = currentConfig.netmask;
    doc["gateway"] = currentConfig.gateway;
    doc["dns_server"] = currentConfig.dns_server;
    
    // Logging
    doc["syslog_server"] = currentConfig.syslog_server;
    doc["syslog_port"] = currentConfig.syslog_port;
    doc["log_level"] = currentConfig.log_level;
    
    // Monitoring
    doc["prometheus_enabled"] = currentConfig.prometheus_enabled;
    doc["prometheus_port"] = currentConfig.prometheus_port;
    
    // GNSS
    doc["gps_enabled"] = currentConfig.gps_enabled;
    doc["glonass_enabled"] = currentConfig.glonass_enabled;
    doc["galileo_enabled"] = currentConfig.galileo_enabled;
    doc["beidou_enabled"] = currentConfig.beidou_enabled;
    doc["qzss_enabled"] = currentConfig.qzss_enabled;
    doc["qzss_l1s_enabled"] = currentConfig.qzss_l1s_enabled;
    doc["gnss_update_rate"] = currentConfig.gnss_update_rate;
    doc["disaster_alert_priority"] = currentConfig.disaster_alert_priority;
    
    // NTP
    doc["ntp_enabled"] = currentConfig.ntp_enabled;
    doc["ntp_port"] = currentConfig.ntp_port;
    doc["ntp_stratum"] = currentConfig.ntp_stratum;
    
    // System
    doc["auto_restart_enabled"] = currentConfig.auto_restart_enabled;
    doc["restart_interval"] = currentConfig.restart_interval;
    doc["debug_enabled"] = currentConfig.debug_enabled;
    doc["config_version"] = currentConfig.config_version;
    
    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}

bool ConfigManager::configFromJson(const String& json) {
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        LOG_ERR_MSG("ConfigManager: JSON解析エラー - %s", error.c_str());
        return false;
    }
    
    SystemConfig tempConfig = currentConfig; // バックアップ
    
    // Network
    if (doc.containsKey("hostname")) {
        strncpy(tempConfig.hostname, doc["hostname"], sizeof(tempConfig.hostname) - 1);
        tempConfig.hostname[sizeof(tempConfig.hostname) - 1] = '\0';
    }
    if (doc.containsKey("ip_address")) tempConfig.ip_address = doc["ip_address"];
    if (doc.containsKey("netmask")) tempConfig.netmask = doc["netmask"];
    if (doc.containsKey("gateway")) tempConfig.gateway = doc["gateway"];
    if (doc.containsKey("dns_server")) tempConfig.dns_server = doc["dns_server"];
    
    // Logging
    if (doc.containsKey("syslog_server")) {
        strncpy(tempConfig.syslog_server, doc["syslog_server"], sizeof(tempConfig.syslog_server) - 1);
        tempConfig.syslog_server[sizeof(tempConfig.syslog_server) - 1] = '\0';
    }
    if (doc.containsKey("syslog_port")) tempConfig.syslog_port = doc["syslog_port"];
    if (doc.containsKey("log_level")) tempConfig.log_level = doc["log_level"];
    
    // Monitoring
    if (doc.containsKey("prometheus_enabled")) tempConfig.prometheus_enabled = doc["prometheus_enabled"];
    if (doc.containsKey("prometheus_port")) tempConfig.prometheus_port = doc["prometheus_port"];
    
    // GNSS
    if (doc.containsKey("gps_enabled")) tempConfig.gps_enabled = doc["gps_enabled"];
    if (doc.containsKey("glonass_enabled")) tempConfig.glonass_enabled = doc["glonass_enabled"];
    if (doc.containsKey("galileo_enabled")) tempConfig.galileo_enabled = doc["galileo_enabled"];
    if (doc.containsKey("beidou_enabled")) tempConfig.beidou_enabled = doc["beidou_enabled"];
    if (doc.containsKey("qzss_enabled")) tempConfig.qzss_enabled = doc["qzss_enabled"];
    if (doc.containsKey("qzss_l1s_enabled")) tempConfig.qzss_l1s_enabled = doc["qzss_l1s_enabled"];
    if (doc.containsKey("gnss_update_rate")) tempConfig.gnss_update_rate = doc["gnss_update_rate"];
    if (doc.containsKey("disaster_alert_priority")) tempConfig.disaster_alert_priority = doc["disaster_alert_priority"];
    
    // NTP
    if (doc.containsKey("ntp_enabled")) tempConfig.ntp_enabled = doc["ntp_enabled"];
    if (doc.containsKey("ntp_port")) tempConfig.ntp_port = doc["ntp_port"];
    if (doc.containsKey("ntp_stratum")) tempConfig.ntp_stratum = doc["ntp_stratum"];
    
    // System
    if (doc.containsKey("auto_restart_enabled")) tempConfig.auto_restart_enabled = doc["auto_restart_enabled"];
    if (doc.containsKey("restart_interval")) tempConfig.restart_interval = doc["restart_interval"];
    if (doc.containsKey("debug_enabled")) tempConfig.debug_enabled = doc["debug_enabled"];
    
    // 設定バージョン更新
    tempConfig.config_version = 1;
    
    // 設定を適用
    if (setConfig(tempConfig)) {
        LOG_INFO_MSG("ConfigManager: JSON設定適用成功");
        return true;
    } else {
        LOG_ERR_MSG("ConfigManager: JSON設定適用失敗");
        return false;
    }
}

void ConfigManager::printConfig() const {
    LOG_INFO_MSG("=== Current Configuration ===");
    LOG_INFO_MSG("Hostname: %s", currentConfig.hostname);
    LOG_INFO_MSG("IP Address: %s", currentConfig.ip_address == 0 ? "DHCP" : "Static");
    LOG_INFO_MSG("Syslog Server: %s", currentConfig.syslog_server);
    LOG_INFO_MSG("Syslog Port: %d", currentConfig.syslog_port);
    LOG_INFO_MSG("Log Level: %d", currentConfig.log_level);
    LOG_INFO_MSG("Prometheus: %s", currentConfig.prometheus_enabled ? "Enabled" : "Disabled");
    LOG_INFO_MSG("GPS: %s", currentConfig.gps_enabled ? "On" : "Off");
    LOG_INFO_MSG("GLONASS: %s", currentConfig.glonass_enabled ? "On" : "Off");
    LOG_INFO_MSG("Galileo: %s", currentConfig.galileo_enabled ? "On" : "Off");
    LOG_INFO_MSG("BeiDou: %s", currentConfig.beidou_enabled ? "On" : "Off");
    LOG_INFO_MSG("QZSS: %s", currentConfig.qzss_enabled ? "On" : "Off");
    LOG_INFO_MSG("QZSS L1S: %s", currentConfig.qzss_l1s_enabled ? "On" : "Off");
    LOG_INFO_MSG("GNSS Update Rate: %d Hz", currentConfig.gnss_update_rate);
    LOG_INFO_MSG("NTP: %s", currentConfig.ntp_enabled ? "Enabled" : "Disabled");
    LOG_INFO_MSG("Config Version: %d", currentConfig.config_version);
}
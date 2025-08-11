#include "ConfigManager.h"
#include "Constants.h"
#include "ConfigDefaults.h"
#include "../hal/HardwareConfig.h"
#include "LoggingService.h"
#include <ArduinoJson.h>
#include "../utils/TimeUtils.h"

ConfigManager::ConfigManager() : 
    configValid(false),
    storageHal(&g_storage_hal),
    changeCallback(nullptr),
    notificationsEnabled(true),
    lastSaveTime(0),
    configChanged(false),
    autoSaveEnabled(ConfigDefaults::System::CONFIG_AUTO_SAVE) {
}

void ConfigManager::init() {
    LOG_INFO_MSG("CONFIG", "ConfigManager: 初期化開始...");
    
    // Storage HAL初期化
    if (!storageHal->initialize()) {
        LOG_ERR_MSG("CONFIG", "ConfigManager: Storage HAL初期化失敗");
        loadDefaults();
        configValid = true;
        return;
    }
    
    // 設定読み込み試行
    if (!loadConfig()) {
        LOG_WARN_MSG("CONFIG", "ConfigManager: 設定読み込み失敗、デフォルト設定使用");
        loadDefaults();
        saveConfig(); // デフォルト設定を保存
    }
    
    configValid = true;
    LOG_INFO_MSG("CONFIG", "ConfigManager: 初期化完了");
    printConfig();
}

bool ConfigManager::loadConfig() {
    StorageResult result = storageHal->readConfig(&currentConfig, sizeof(SystemConfig));
    
    switch (result) {
        case STORAGE_SUCCESS:
            if (validateConfig(currentConfig)) {
                LOG_INFO_MSG("CONFIG", "ConfigManager: 設定読み込み成功");
                return true;
            } else {
                LOG_ERR_MSG("CONFIG", "ConfigManager: 設定検証失敗");
                return false;
            }
            
        case STORAGE_ERROR_MAGIC:
            LOG_WARN_MSG("CONFIG", "ConfigManager: 初回起動 - 設定が存在しません");
            return false;
            
        case STORAGE_ERROR_CRC:
            LOG_ERR_MSG("CONFIG", "ConfigManager: 設定データ破損 (CRC32エラー)");
            return false;
            
        case STORAGE_ERROR_SIZE:
            LOG_ERR_MSG("CONFIG", "ConfigManager: 設定サイズ不一致");
            return false;
            
        default:
            LOG_ERR_F("CONFIG", "ConfigManager: 設定読み込みエラー (%d)", result);
            return false;
    }
}

bool ConfigManager::saveConfig() {
    if (!validateConfig(currentConfig)) {
        LOG_ERR_MSG("CONFIG", "ConfigManager: 無効な設定のため保存中止");
        return false;
    }
    
    // タイムスタンプ更新
    currentConfig.config_version = ConfigDefaults::System::CONFIG_VERSION;
    
    StorageResult result = storageHal->writeConfig(&currentConfig, sizeof(SystemConfig));
    
    if (result == STORAGE_SUCCESS) {
        updateLastSaveTime();
        configChanged = false;
        LOG_INFO_MSG("CONFIG", "ConfigManager: 設定保存完了");
        return true;
    } else {
        LOG_ERR_F("CONFIG", "ConfigManager: 設定保存失敗 (%d)", (int)result);
        return false;
    }
}

void ConfigManager::loadDefaults() {
    LOG_INFO_MSG("CONFIG", "ConfigManager: デフォルト設定読み込み...");
    
    // 新しいデフォルト設定システムを使用
    SystemConfig oldConfig = currentConfig;
    currentConfig = createDefaultSystemConfig();
    
    // 変更通知（デフォルト読み込みの場合は通知しない）
    configChanged = true;
    
    LOG_INFO_MSG("CONFIG", "ConfigManager: デフォルト設定読み込み完了");
}

void ConfigManager::resetToDefaults() {
    LOG_WARN_MSG("CONFIG", "ConfigManager: 工場出荷時リセット実行...");
    
    // Storage HAL経由で工場出荷時リセット
    StorageResult result = storageHal->factoryReset();
    if (result != STORAGE_SUCCESS) {
        LOG_ERR_F("CONFIG", "ConfigManager: ストレージリセット失敗 (%d)", (int)result);
    }
    
    // デフォルト設定読み込み
    loadDefaults();
    
    // 新しい設定を保存
    if (saveConfig()) {
        LOG_INFO_MSG("CONFIG", "ConfigManager: 工場出荷時リセット完了");
    } else {
        LOG_ERR_MSG("CONFIG", "ConfigManager: 工場出荷時リセット - 設定保存失敗");
    }
}

bool ConfigManager::setConfig(const SystemConfig& newConfig) {
    if (!validateConfig(newConfig)) {
        LOG_ERR_MSG("CONFIG", "ConfigManager: 無効な設定");
        return false;
    }
    
    currentConfig = newConfig;
    return saveConfig();
}

// validateConfig function is implemented below using performDeepValidation

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
        LOG_ERR_F("CONFIG", "ConfigManager: JSON解析エラー - %s", error.c_str());
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
        LOG_INFO_MSG("CONFIG", "ConfigManager: JSON設定適用成功");
        return true;
    } else {
        LOG_ERR_MSG("CONFIG", "ConfigManager: JSON設定適用失敗");
        return false;
    }
}

void ConfigManager::printConfig() const {
    LOG_INFO_MSG("CONFIG", "=== Current Configuration ===");
    LOG_INFO_F("CONFIG", "Hostname: %s", currentConfig.hostname);
    LOG_INFO_F("CONFIG", "IP Address: %s", currentConfig.ip_address == 0 ? "DHCP" : "Static");
    LOG_INFO_F("CONFIG", "Syslog Server: %s", currentConfig.syslog_server);
    LOG_INFO_F("CONFIG", "Syslog Port: %d", (int)currentConfig.syslog_port);
    LOG_INFO_F("CONFIG", "Log Level: %d", (int)currentConfig.log_level);
    LOG_INFO_F("CONFIG", "Prometheus: %s", currentConfig.prometheus_enabled ? "Enabled" : "Disabled");
    LOG_INFO_F("CONFIG", "GPS: %s", currentConfig.gps_enabled ? "On" : "Off");
    LOG_INFO_F("CONFIG", "GLONASS: %s", currentConfig.glonass_enabled ? "On" : "Off");
    LOG_INFO_F("CONFIG", "Galileo: %s", currentConfig.galileo_enabled ? "On" : "Off");
    LOG_INFO_F("CONFIG", "BeiDou: %s", currentConfig.beidou_enabled ? "On" : "Off");
    LOG_INFO_F("CONFIG", "QZSS: %s", currentConfig.qzss_enabled ? "On" : "Off");
    LOG_INFO_F("CONFIG", "QZSS L1S: %s", currentConfig.qzss_l1s_enabled ? "On" : "Off");
    LOG_INFO_F("CONFIG", "GNSS Update Rate: %d Hz", (int)currentConfig.gnss_update_rate);
    LOG_INFO_F("CONFIG", "NTP: %s", currentConfig.ntp_enabled ? "Enabled" : "Disabled");
    LOG_INFO_F("CONFIG", "Config Version: %d", (int)currentConfig.config_version);
}

// =============================================================================
// 新しい拡張機能の実装
// =============================================================================

// Auto-save configuration management
void ConfigManager::checkAutoSave() {
    if (!autoSaveEnabled || !configChanged) {
        return;
    }
    
    uint32_t currentTime = millis();
    uint32_t saveInterval = ConfigDefaults::System::CONFIG_SAVE_INTERVAL * Constants::Timing::MILLIS_PER_SEC;
    
    if (shouldAutoSave() && (currentTime - lastSaveTime > saveInterval)) {
        if (saveConfig()) {
            LOG_INFO_MSG("CONFIG", "ConfigManager: Auto-save completed");
        } else {
            LOG_WARN_MSG("CONFIG", "ConfigManager: Auto-save failed");
        }
    }
}

// Enhanced validation methods using new constants
bool ConfigManager::validateConfig(const SystemConfig& config) const {
    return performDeepValidation(config);
}

bool ConfigManager::validateNetworkConfig(uint32_t ip, uint32_t netmask, uint32_t gateway) const {
    // Allow DHCP (all zeros)
    if (ip == 0 && netmask == 0 && gateway == 0) {
        return true;
    }
    
    // Basic IP validation (simplified)
    return ip != 0 && netmask != 0;
}

bool ConfigManager::validateHostname(const char* hostname) const {
    if (!hostname) return false;
    
    size_t len = strlen(hostname);
    return len >= ConfigDefaults::ValidationLimits::HOSTNAME_MIN_LENGTH && 
           len <= ConfigDefaults::ValidationLimits::HOSTNAME_MAX_LENGTH;
}

bool ConfigManager::validateSyslogServer(const char* server) const {
    if (!server) return false;
    
    // Empty string means syslog disabled
    size_t len = strlen(server);
    return len <= ConfigDefaults::ValidationLimits::SYSLOG_SERVER_MAX_LENGTH;
}

bool ConfigManager::validatePortNumber(uint16_t port) const {
    return port >= ConfigDefaults::ValidationLimits::NTP_PORT_MIN && 
           port <= ConfigDefaults::ValidationLimits::NTP_PORT_MAX;
}

bool ConfigManager::validateGnssUpdateRate(uint8_t rate) const {
    return rate >= ConfigDefaults::ValidationLimits::GNSS_UPDATE_RATE_MIN && 
           rate <= ConfigDefaults::ValidationLimits::GNSS_UPDATE_RATE_MAX;
}

bool ConfigManager::validateLogLevel(uint8_t level) const {
    return level >= ConfigDefaults::ValidationLimits::LOG_LEVEL_MIN && 
           level <= ConfigDefaults::ValidationLimits::LOG_LEVEL_MAX;
}

bool ConfigManager::validateNtpStratum(uint8_t stratum) const {
    return stratum >= ConfigDefaults::ValidationLimits::NTP_STRATUM_MIN && 
           stratum <= ConfigDefaults::ValidationLimits::NTP_STRATUM_MAX;
}

bool ConfigManager::validateDisasterAlertPriority(uint8_t priority) const {
    return priority >= ConfigDefaults::ValidationLimits::DISASTER_ALERT_PRIORITY_MIN && 
           priority <= ConfigDefaults::ValidationLimits::DISASTER_ALERT_PRIORITY_MAX;
}

// Extended setter methods
bool ConfigManager::setNtpConfig(bool enabled, uint16_t port, uint8_t stratum) {
    if (!validatePortNumber(port) || !validateNtpStratum(stratum)) {
        LOG_ERR_MSG("CONFIG", "ConfigManager: Invalid NTP configuration");
        return false;
    }
    
    SystemConfig oldConfig = currentConfig;
    currentConfig.ntp_enabled = enabled;
    currentConfig.ntp_port = port;
    currentConfig.ntp_stratum = stratum;
    
    markConfigChanged();
    notifyConfigChange(oldConfig, currentConfig);
    
    return autoSaveEnabled ? saveConfig() : true;
}

bool ConfigManager::setSystemConfig(bool autoRestart, uint32_t restartInterval, bool debugEnabled) {
    SystemConfig oldConfig = currentConfig;
    currentConfig.auto_restart_enabled = autoRestart;
    currentConfig.restart_interval = restartInterval;
    currentConfig.debug_enabled = debugEnabled;
    
    markConfigChanged();
    notifyConfigChange(oldConfig, currentConfig);
    
    return autoSaveEnabled ? saveConfig() : true;
}

bool ConfigManager::setQzssL1sConfig(bool enabled, uint8_t priority) {
    if (!validateDisasterAlertPriority(priority)) {
        LOG_ERR_MSG("CONFIG", "ConfigManager: Invalid QZSS L1S priority");
        return false;
    }
    
    SystemConfig oldConfig = currentConfig;
    currentConfig.qzss_l1s_enabled = enabled;
    currentConfig.disaster_alert_priority = priority;
    
    markConfigChanged();
    notifyConfigChange(oldConfig, currentConfig);
    
    return autoSaveEnabled ? saveConfig() : true;
}

bool ConfigManager::setMonitoringConfig(bool prometheusEnabled, uint16_t port) {
    if (!validatePortNumber(port)) {
        LOG_ERR_MSG("CONFIG", "ConfigManager: Invalid monitoring port");
        return false;
    }
    
    SystemConfig oldConfig = currentConfig;
    currentConfig.prometheus_enabled = prometheusEnabled;
    currentConfig.prometheus_port = port;
    
    markConfigChanged();
    notifyConfigChange(oldConfig, currentConfig);
    
    return autoSaveEnabled ? saveConfig() : true;
}

// Batch update methods
bool ConfigManager::updateNetworkSettings(const char* hostname, uint32_t ip, uint32_t netmask, uint32_t gateway) {
    if (!validateHostname(hostname) || !validateNetworkConfig(ip, netmask, gateway)) {
        LOG_ERR_MSG("CONFIG", "ConfigManager: Invalid network settings");
        return false;
    }
    
    SystemConfig oldConfig = currentConfig;
    strncpy(currentConfig.hostname, hostname, sizeof(currentConfig.hostname) - 1);
    currentConfig.hostname[sizeof(currentConfig.hostname) - 1] = '\0';
    currentConfig.ip_address = ip;
    currentConfig.netmask = netmask;
    currentConfig.gateway = gateway;
    
    markConfigChanged();
    notifyConfigChange(oldConfig, currentConfig);
    
    return autoSaveEnabled ? saveConfig() : true;
}

bool ConfigManager::updateGnssSettings(bool gps, bool glonass, bool galileo, bool beidou, bool qzss, uint8_t rate) {
    if (!validateGnssUpdateRate(rate)) {
        LOG_ERR_MSG("CONFIG", "ConfigManager: Invalid GNSS update rate");
        return false;
    }
    
    SystemConfig oldConfig = currentConfig;
    currentConfig.gps_enabled = gps;
    currentConfig.glonass_enabled = glonass;
    currentConfig.galileo_enabled = galileo;
    currentConfig.beidou_enabled = beidou;
    currentConfig.qzss_enabled = qzss;
    currentConfig.gnss_update_rate = rate;
    
    markConfigChanged();
    notifyConfigChange(oldConfig, currentConfig);
    
    return autoSaveEnabled ? saveConfig() : true;
}

bool ConfigManager::updateLoggingSettings(const char* server, uint16_t port, uint8_t level) {
    if (!validateSyslogServer(server) || !validatePortNumber(port) || !validateLogLevel(level)) {
        LOG_ERR_MSG("CONFIG", "ConfigManager: Invalid logging settings");
        return false;
    }
    
    SystemConfig oldConfig = currentConfig;
    strncpy(currentConfig.syslog_server, server, sizeof(currentConfig.syslog_server) - 1);
    currentConfig.syslog_server[sizeof(currentConfig.syslog_server) - 1] = '\0';
    currentConfig.syslog_port = port;
    currentConfig.log_level = level;
    
    markConfigChanged();
    notifyConfigChange(oldConfig, currentConfig);
    
    return autoSaveEnabled ? saveConfig() : true;
}

// Configuration comparison and utilities
bool ConfigManager::configEquals(const SystemConfig& config1, const SystemConfig& config2) const {
    return memcmp(&config1, &config2, sizeof(SystemConfig)) == 0;
}

String ConfigManager::getConfigDifference(const SystemConfig& oldConfig, const SystemConfig& newConfig) const {
    String diff = "Config changes: ";
    bool hasChanges = false;
    
    if (strcmp(oldConfig.hostname, newConfig.hostname) != 0) {
        diff += "hostname, ";
        hasChanges = true;
    }
    if (oldConfig.ip_address != newConfig.ip_address) {
        diff += "ip_address, ";
        hasChanges = true;
    }
    if (oldConfig.log_level != newConfig.log_level) {
        diff += "log_level, ";
        hasChanges = true;
    }
    if (oldConfig.gnss_update_rate != newConfig.gnss_update_rate) {
        diff += "gnss_update_rate, ";
        hasChanges = true;
    }
    
    if (!hasChanges) {
        diff = "No configuration changes detected";
    } else {
        // Remove trailing ", "
        diff.remove(diff.length() - 2);
    }
    
    return diff;
}

// Factory reset and backup
void ConfigManager::resetToFactoryDefaults() {
    LOG_WARN_MSG("CONFIG", "ConfigManager: Factory reset initiated");
    
    SystemConfig oldConfig = currentConfig;
    
    // Clear storage
    if (storageHal) {
        StorageResult result = storageHal->factoryReset();
        if (result != STORAGE_SUCCESS) {
            LOG_ERR_F("CONFIG", "ConfigManager: Storage reset failed (%d)", (int)result);
        }
    }
    
    // Load factory defaults
    loadDefaults();
    
    // Save new defaults
    if (saveConfig()) {
        LOG_INFO_MSG("CONFIG", "ConfigManager: Factory reset completed successfully");
        notifyConfigChange(oldConfig, currentConfig);
    } else {
        LOG_ERR_MSG("CONFIG", "ConfigManager: Factory reset - failed to save defaults");
    }
}

bool ConfigManager::isFactoryDefault() const {
    SystemConfig defaultConfig = createDefaultSystemConfig();
    return configEquals(currentConfig, defaultConfig);
}

// Configuration integrity and diagnostics
uint32_t ConfigManager::getConfigChecksum() const {
    // Simple checksum calculation
    uint32_t checksum = 0;
    const uint8_t* data = reinterpret_cast<const uint8_t*>(&currentConfig);
    
    for (size_t i = 0; i < sizeof(SystemConfig); i++) {
        checksum += data[i];
    }
    
    return checksum;
}

bool ConfigManager::verifyConfigIntegrity() const {
    return validateConfig(currentConfig) && configValid;
}

String ConfigManager::getConfigSummary() const {
    String summary = "Config Summary: ";
    summary += "Host=" + String(currentConfig.hostname) + ", ";
    summary += "GNSS=" + String(currentConfig.gnss_update_rate) + "Hz, ";
    summary += "NTP=" + String(currentConfig.ntp_enabled ? "ON" : "OFF") + ", ";
    summary += "Log=" + String(currentConfig.log_level) + ", ";
    summary += "Ver=" + String(currentConfig.config_version);
    
    return summary;
}

void ConfigManager::printConfigStats() const {
    LOG_INFO_MSG("CONFIG", "=== Configuration Statistics ===");
    LOG_INFO_F("CONFIG", "Config size: %d bytes", sizeof(SystemConfig));
    LOG_INFO_F("CONFIG", "Checksum: 0x%08X", getConfigChecksum());
    LOG_INFO_F("CONFIG", "Last saved: %lu ms ago", millis() - lastSaveTime);
    LOG_INFO_F("CONFIG", "Unsaved changes: %s", configChanged ? "Yes" : "No");
    LOG_INFO_F("CONFIG", "Auto-save: %s", autoSaveEnabled ? "Enabled" : "Disabled");
    LOG_INFO_F("CONFIG", "Factory default: %s", isFactoryDefault() ? "Yes" : "No");
    LOG_INFO_F("CONFIG", "Integrity check: %s", verifyConfigIntegrity() ? "PASS" : "FAIL");
}

// Private helper methods
void ConfigManager::notifyConfigChange(const SystemConfig& oldConfig, const SystemConfig& newConfig) {
    if (notificationsEnabled && changeCallback) {
        changeCallback(oldConfig, newConfig, this);
    }
}

bool ConfigManager::performDeepValidation(const SystemConfig& config) const {
    // Hostname validation
    if (!validateHostname(config.hostname)) {
        LOG_ERR_MSG("CONFIG", "Validation failed: Invalid hostname");
        return false;
    }
    
    // Network validation
    if (!validateNetworkConfig(config.ip_address, config.netmask, config.gateway)) {
        LOG_ERR_MSG("CONFIG", "Validation failed: Invalid network configuration");
        return false;
    }
    
    // Syslog validation
    if (!validateSyslogServer(config.syslog_server)) {
        LOG_ERR_MSG("CONFIG", "Validation failed: Invalid syslog server");
        return false;
    }
    
    if (!validatePortNumber(config.syslog_port)) {
        LOG_ERR_F("CONFIG", "Validation failed: Invalid syslog port (%d)", config.syslog_port);
        return false;
    }
    
    // Log level validation
    if (!validateLogLevel(config.log_level)) {
        LOG_ERR_F("CONFIG", "Validation failed: Invalid log level (%d)", config.log_level);
        return false;
    }
    
    // GNSS validation
    if (!validateGnssUpdateRate(config.gnss_update_rate)) {
        LOG_ERR_F("CONFIG", "Validation failed: Invalid GNSS update rate (%d)", config.gnss_update_rate);
        return false;
    }
    
    // NTP validation
    if (!validatePortNumber(config.ntp_port)) {
        LOG_ERR_F("CONFIG", "Validation failed: Invalid NTP port (%d)", config.ntp_port);
        return false;
    }
    
    if (!validateNtpStratum(config.ntp_stratum)) {
        LOG_ERR_F("CONFIG", "Validation failed: Invalid NTP stratum (%d)", config.ntp_stratum);
        return false;
    }
    
    // Disaster alert priority validation
    if (!validateDisasterAlertPriority(config.disaster_alert_priority)) {
        LOG_ERR_F("CONFIG", "Validation failed: Invalid disaster alert priority (%d)", config.disaster_alert_priority);
        return false;
    }
    
    // Version validation
    if (config.config_version == 0) {
        LOG_ERR_MSG("CONFIG", "Validation failed: Invalid configuration version");
        return false;
    }
    
    return true;
}

void ConfigManager::updateLastSaveTime() {
    lastSaveTime = millis();
}

bool ConfigManager::shouldAutoSave() const {
    if (!autoSaveEnabled || !configChanged) {
        return false;
    }
    
    uint32_t currentTime = millis();
    uint32_t saveInterval = ConfigDefaults::System::CONFIG_SAVE_INTERVAL * Constants::Timing::MILLIS_PER_SEC;
    
    return (currentTime - lastSaveTime) >= saveInterval;
}

// =============================================================================
// Global utility function implementation
// =============================================================================

/**
 * @brief SystemConfig構造体にデフォルト値を設定
 * @return デフォルト値で初期化されたSystemConfig構造体
 */
SystemConfig createDefaultSystemConfig() {
    SystemConfig config = {};
    
    // Network Configuration
    strncpy(config.hostname, ConfigDefaults::Network::HOSTNAME, sizeof(config.hostname) - 1);
    config.ip_address = ConfigDefaults::Network::IP_ADDRESS;
    config.netmask = ConfigDefaults::Network::NETMASK;
    config.gateway = ConfigDefaults::Network::GATEWAY;
    config.dns_server = ConfigDefaults::Network::DNS_SERVER;
    
    // Logging Configuration  
    strncpy(config.syslog_server, ConfigDefaults::Logging::SYSLOG_SERVER, sizeof(config.syslog_server) - 1);
    config.syslog_port = ConfigDefaults::Logging::SYSLOG_PORT;
    config.log_level = ConfigDefaults::Logging::LOG_LEVEL;
    
    // Monitoring
    config.prometheus_enabled = ConfigDefaults::Monitoring::PROMETHEUS_ENABLED;
    config.prometheus_port = ConfigDefaults::Monitoring::PROMETHEUS_PORT;
    
    // GNSS Configuration
    config.gps_enabled = ConfigDefaults::GNSS::GPS_ENABLED;
    config.glonass_enabled = ConfigDefaults::GNSS::GLONASS_ENABLED;
    config.galileo_enabled = ConfigDefaults::GNSS::GALILEO_ENABLED;
    config.beidou_enabled = ConfigDefaults::GNSS::BEIDOU_ENABLED;
    config.qzss_enabled = ConfigDefaults::GNSS::QZSS_ENABLED;
    config.qzss_l1s_enabled = ConfigDefaults::GNSS::QZSS_L1S_ENABLED;
    config.gnss_update_rate = ConfigDefaults::GNSS::GNSS_UPDATE_RATE;
    config.disaster_alert_priority = ConfigDefaults::GNSS::DISASTER_ALERT_PRIORITY;
    
    // NTP Server Configuration
    config.ntp_enabled = ConfigDefaults::NTP::NTP_ENABLED;
    config.ntp_port = ConfigDefaults::NTP::NTP_PORT;
    config.ntp_stratum = ConfigDefaults::NTP::NTP_STRATUM;
    
    // System Configuration
    config.auto_restart_enabled = ConfigDefaults::System::AUTO_RESTART_ENABLED;
    config.restart_interval = ConfigDefaults::System::RESTART_INTERVAL_HOURS;
    config.debug_enabled = ConfigDefaults::System::DEBUG_ENABLED;
    
    // Configuration metadata
    config.config_version = ConfigDefaults::System::CONFIG_VERSION;
    
    return config;
}
#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include "Constants.h"
#include "ConfigDefaults.h"
#include "../hal/StorageHal.h"

// Configuration structure matching design.md specifications
struct SystemConfig {
    // Network Configuration
    char hostname[32];
    uint32_t ip_address;        // 0 for DHCP
    uint32_t netmask;
    uint32_t gateway;
    uint32_t dns_server;
    
    // Logging Configuration  
    char syslog_server[64];
    uint16_t syslog_port;
    uint8_t log_level;          // 0=DEBUG, 1=INFO, 2=WARN, 3=ERROR
    
    // Monitoring
    bool prometheus_enabled;
    uint16_t prometheus_port;   // Default 80, same as web server
    
    // GNSS Configuration
    bool gps_enabled;
    bool glonass_enabled;
    bool galileo_enabled;
    bool beidou_enabled;
    bool qzss_enabled;
    bool qzss_l1s_enabled;      // QZSS L1S disaster alert enable
    uint8_t gnss_update_rate;   // Hz (1-10)
    uint8_t disaster_alert_priority; // 0=low, 1=medium, 2=high
    
    // NTP Server Configuration
    bool ntp_enabled;
    uint16_t ntp_port;          // Default 123
    uint8_t ntp_stratum;        // 1 for GPS, adjustable
    
    // System Configuration
    bool auto_restart_enabled;  // Auto restart on critical errors
    uint32_t restart_interval;  // Hours between automatic restarts
    bool debug_enabled;         // Debug output enabled
    
    // Configuration metadata (simplified)
    uint32_t config_version;    // For future migration
};

// Default configuration values
// Forward declaration for callback
class ConfigManager;
typedef void (*ConfigChangeCallback)(const SystemConfig& oldConfig, const SystemConfig& newConfig, ConfigManager* manager);

class ConfigManager {
private:
    SystemConfig currentConfig;
    bool configValid;
    StorageHAL* storageHal;
    
    // Configuration change notification
    ConfigChangeCallback changeCallback;
    bool notificationsEnabled;
    
    // Runtime configuration tracking
    uint32_t lastSaveTime;
    bool configChanged;
    bool autoSaveEnabled;
    
public:
    ConfigManager();
    
    // Initialization and persistence
    void init();
    bool loadConfig();
    bool saveConfig();
    void loadDefaults();
    void resetToDefaults();
    
    // Configuration access
    const SystemConfig& getConfig() const { return currentConfig; }
    bool setConfig(const SystemConfig& newConfig);
    bool isConfigValid() const { return configValid; }
    
    // Runtime configuration management
    bool hasUnsavedChanges() const { return configChanged; }
    uint32_t getLastSaveTime() const { return lastSaveTime; }
    void markConfigChanged() { configChanged = true; }
    
    // Auto-save configuration
    void enableAutoSave(bool enabled) { autoSaveEnabled = enabled; }
    bool isAutoSaveEnabled() const { return autoSaveEnabled; }
    void checkAutoSave(); // Call periodically to auto-save if needed
    
    // Change notifications
    void setChangeCallback(ConfigChangeCallback callback) { changeCallback = callback; }
    void enableNotifications(bool enabled) { notificationsEnabled = enabled; }
    bool areNotificationsEnabled() const { return notificationsEnabled; }
    
    // Individual setting getters
    const char* getHostname() const { return currentConfig.hostname; }
    uint32_t getIpAddress() const { return currentConfig.ip_address; }
    uint32_t getNetmask() const { return currentConfig.netmask; }
    uint32_t getGateway() const { return currentConfig.gateway; }
    const char* getSyslogServer() const { return currentConfig.syslog_server; }
    uint16_t getSyslogPort() const { return currentConfig.syslog_port; }
    uint8_t getLogLevel() const { return currentConfig.log_level; }
    bool isPrometheusEnabled() const { return currentConfig.prometheus_enabled; }
    bool isNtpEnabled() const { return currentConfig.ntp_enabled; }
    uint8_t getGnssUpdateRate() const { return currentConfig.gnss_update_rate; }
    
    // Individual setting setters with enhanced validation
    bool setHostname(const char* hostname);
    bool setNetworkConfig(uint32_t ip, uint32_t netmask, uint32_t gateway);
    bool setSyslogConfig(const char* server, uint16_t port);
    bool setLogLevel(uint8_t level);
    bool setPrometheusEnabled(bool enabled);
    bool setGnssConstellations(bool gps, bool glonass, bool galileo, bool beidou, bool qzss);
    bool setGnssUpdateRate(uint8_t rate);
    
    // Extended setters for new configuration options
    bool setNtpConfig(bool enabled, uint16_t port, uint8_t stratum);
    bool setSystemConfig(bool autoRestart, uint32_t restartInterval, bool debugEnabled);
    bool setQzssL1sConfig(bool enabled, uint8_t priority);
    bool setMonitoringConfig(bool prometheusEnabled, uint16_t port);
    
    // Batch configuration updates
    bool updateNetworkSettings(const char* hostname, uint32_t ip, uint32_t netmask, uint32_t gateway);
    bool updateGnssSettings(bool gps, bool glonass, bool galileo, bool beidou, bool qzss, uint8_t rate);
    bool updateLoggingSettings(const char* server, uint16_t port, uint8_t level);
    
    // Enhanced configuration validation
    bool validateConfig(const SystemConfig& config) const;
    bool validateNetworkConfig(uint32_t ip, uint32_t netmask, uint32_t gateway) const;
    bool validateHostname(const char* hostname) const;
    bool validateSyslogServer(const char* server) const;
    bool validatePortNumber(uint16_t port) const;
    bool validateGnssUpdateRate(uint8_t rate) const;
    bool validateLogLevel(uint8_t level) const;
    bool validateNtpStratum(uint8_t stratum) const;
    bool validateDisasterAlertPriority(uint8_t priority) const;
    
    // Configuration comparison and diff
    bool configEquals(const SystemConfig& config1, const SystemConfig& config2) const;
    String getConfigDifference(const SystemConfig& oldConfig, const SystemConfig& newConfig) const;
    
    // JSON serialization for web interface
    String configToJson() const;
    bool configFromJson(const String& json);
    
    // Reset and factory defaults
    void clearEEPROM();
    void resetToFactoryDefaults(); // More comprehensive reset
    bool isFactoryDefault() const;
    
    // Configuration backup and restore
    bool exportConfig(String& jsonOutput) const;
    bool importConfig(const String& jsonInput);
    bool createConfigBackup();
    bool restoreFromBackup();
    
    // Debug and diagnostics
    void printConfig() const;
    void printConfigDifferences(const SystemConfig& other) const;
    void printValidationErrors(const SystemConfig& config) const;
    void printConfigStats() const;
    String getConfigSummary() const;
    
    // Memory and performance monitoring
    size_t getConfigSize() const { return sizeof(SystemConfig); }
    uint32_t getConfigChecksum() const;
    bool verifyConfigIntegrity() const;
    
private:
    // Internal helper methods
    void notifyConfigChange(const SystemConfig& oldConfig, const SystemConfig& newConfig);
    bool performDeepValidation(const SystemConfig& config) const;
    void updateLastSaveTime();
    bool shouldAutoSave() const;
};

#endif // CONFIG_MANAGER_H
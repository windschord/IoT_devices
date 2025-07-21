#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <EEPROM.h>

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
    
    // Configuration metadata
    uint32_t config_version;    // For future migration
    uint32_t checksum;          // Configuration integrity check
};

// Default configuration values
class ConfigManager {
private:
    SystemConfig currentConfig;
    bool configValid;
    static constexpr uint32_t CONFIG_VERSION = 1;
    static constexpr uint32_t EEPROM_CONFIG_ADDR = 0;
    static constexpr uint32_t CONFIG_MAGIC = 0xC0FFEE42; // Magic number for validation
    
public:
    ConfigManager();
    
    // Initialization and persistence
    void init();
    bool loadFromEEPROM();
    bool saveToEEPROM();
    void loadDefaults();
    
    // Configuration access
    const SystemConfig& getConfig() const { return currentConfig; }
    bool setConfig(const SystemConfig& newConfig);
    bool isConfigValid() const { return configValid; }
    
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
    
    // Individual setting setters with validation
    bool setHostname(const char* hostname);
    bool setNetworkConfig(uint32_t ip, uint32_t netmask, uint32_t gateway);
    bool setSyslogConfig(const char* server, uint16_t port);
    bool setLogLevel(uint8_t level);
    bool setPrometheusEnabled(bool enabled);
    bool setGnssConstellations(bool gps, bool glonass, bool galileo, bool beidou, bool qzss);
    bool setGnssUpdateRate(uint8_t rate);
    
    // Configuration validation
    bool validateConfig(const SystemConfig& config) const;
    uint32_t calculateChecksum(const SystemConfig& config) const;
    
    // JSON serialization for web interface
    String configToJson() const;
    bool configFromJson(const String& json);
    
    // Reset and factory defaults
    void resetToDefaults();
    void clearEEPROM();
    
    // Debug and diagnostics
    void printConfig() const;
    void printConfigDifferences(const SystemConfig& other) const;
};

#endif // CONFIG_MANAGER_H
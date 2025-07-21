#include "ConfigManager.h"
#include "HardwareConfig.h"
#include <ArduinoJson.h>

ConfigManager::ConfigManager() : configValid(false) {
}

void ConfigManager::init() {
    Serial.println("Initializing Configuration Manager...");
    
    // Initialize EEPROM
    EEPROM.begin(512);  // Allocate 512 bytes for configuration
    
    // Try to load configuration from EEPROM
    if (!loadFromEEPROM()) {
        Serial.println("No valid configuration found, loading defaults");
        loadDefaults();
        saveToEEPROM();
    }
    
    configValid = true;
    Serial.println("Configuration Manager initialized successfully");
    printConfig();
}

bool ConfigManager::loadFromEEPROM() {
    // Read magic number to verify configuration exists
    uint32_t magic = 0;
    EEPROM.get(EEPROM_CONFIG_ADDR, magic);
    
    if (magic != CONFIG_MAGIC) {
        Serial.println("No configuration magic number found in EEPROM");
        return false;
    }
    
    // Read configuration structure
    SystemConfig tempConfig;
    EEPROM.get(EEPROM_CONFIG_ADDR + sizeof(CONFIG_MAGIC), tempConfig);
    
    // Validate configuration
    if (!validateConfig(tempConfig)) {
        Serial.println("Configuration validation failed");
        return false;
    }
    
    // Verify checksum
    uint32_t expectedChecksum = calculateChecksum(tempConfig);
    if (tempConfig.checksum != expectedChecksum) {
        Serial.print("Configuration checksum mismatch: expected ");
        Serial.print(expectedChecksum, HEX);
        Serial.print(", got ");
        Serial.println(tempConfig.checksum, HEX);
        return false;
    }
    
    currentConfig = tempConfig;
    Serial.println("Configuration loaded from EEPROM successfully");
    return true;
}

bool ConfigManager::saveToEEPROM() {
    // Calculate and set checksum
    currentConfig.checksum = calculateChecksum(currentConfig);
    
    // Write magic number first
    EEPROM.put(EEPROM_CONFIG_ADDR, CONFIG_MAGIC);
    
    // Write configuration structure
    EEPROM.put(EEPROM_CONFIG_ADDR + sizeof(CONFIG_MAGIC), currentConfig);
    
    // Commit to EEPROM
    bool success = EEPROM.commit();
    
    if (success) {
        Serial.println("Configuration saved to EEPROM successfully");
    } else {
        Serial.println("Failed to save configuration to EEPROM");
    }
    
    return success;
}

void ConfigManager::loadDefaults() {
    Serial.println("Loading default configuration...");
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
    currentConfig.beidou_enabled = true;
    currentConfig.qzss_enabled = true;
    currentConfig.qzss_l1s_enabled = true;
    currentConfig.gnss_update_rate = 1;  // 1 Hz
    currentConfig.disaster_alert_priority = 2;  // High priority
    
    // NTP Server Configuration
    currentConfig.ntp_enabled = true;
    currentConfig.ntp_port = NTP_PORT;
    currentConfig.ntp_stratum = 1;  // GPS primary reference
    
    // System Configuration
    currentConfig.auto_restart_enabled = false;
    currentConfig.restart_interval = 24;  // 24 hours
    currentConfig.debug_enabled = false;
    
    // Configuration metadata
    currentConfig.config_version = CONFIG_VERSION;
    currentConfig.checksum = 0;  // Will be calculated when saved
    
    Serial.println("Default configuration loaded");
}

bool ConfigManager::setConfig(const SystemConfig& newConfig) {
    if (!validateConfig(newConfig)) {
        Serial.println("Configuration validation failed");
        return false;
    }
    
    currentConfig = newConfig;
    currentConfig.config_version = CONFIG_VERSION;
    
    return saveToEEPROM();
}

bool ConfigManager::validateConfig(const SystemConfig& config) const {
    // Validate hostname
    if (strlen(config.hostname) == 0 || strlen(config.hostname) >= sizeof(config.hostname)) {
        Serial.println("Invalid hostname");
        return false;
    }
    
    // Validate syslog server
    if (strlen(config.syslog_server) >= sizeof(config.syslog_server)) {
        Serial.println("Invalid syslog server");
        return false;
    }
    
    // Validate syslog port
    if (config.syslog_port == 0) {
        Serial.println("Invalid syslog port");
        return false;
    }
    
    // Validate log level
    if (config.log_level > 3) {
        Serial.println("Invalid log level");
        return false;
    }
    
    // Validate GNSS update rate
    if (config.gnss_update_rate == 0 || config.gnss_update_rate > 10) {
        Serial.println("Invalid GNSS update rate");
        return false;
    }
    
    // Validate disaster alert priority
    if (config.disaster_alert_priority > 2) {
        Serial.println("Invalid disaster alert priority");
        return false;
    }
    
    // Validate configuration version
    if (config.config_version != CONFIG_VERSION) {
        Serial.println("Invalid configuration version");
        return false;
    }
    
    return true;
}

uint32_t ConfigManager::calculateChecksum(const SystemConfig& config) const {
    uint32_t checksum = 0;
    const uint8_t* data = reinterpret_cast<const uint8_t*>(&config);
    
    // Calculate checksum for all fields except the checksum field itself
    size_t checksumOffset = offsetof(SystemConfig, checksum);
    
    for (size_t i = 0; i < sizeof(SystemConfig); i++) {
        if (i < checksumOffset || i >= checksumOffset + sizeof(config.checksum)) {
            checksum ^= data[i];
            checksum = (checksum << 1) | (checksum >> 31);  // Simple rotate left
        }
    }
    
    return checksum;
}

bool ConfigManager::setHostname(const char* hostname) {
    if (!hostname || strlen(hostname) == 0 || strlen(hostname) >= sizeof(currentConfig.hostname)) {
        return false;
    }
    
    strcpy(currentConfig.hostname, hostname);
    return saveToEEPROM();
}

bool ConfigManager::setNetworkConfig(uint32_t ip, uint32_t netmask, uint32_t gateway) {
    currentConfig.ip_address = ip;
    currentConfig.netmask = netmask;
    currentConfig.gateway = gateway;
    return saveToEEPROM();
}

bool ConfigManager::setSyslogConfig(const char* server, uint16_t port) {
    if (!server || strlen(server) >= sizeof(currentConfig.syslog_server) || port == 0) {
        return false;
    }
    
    strcpy(currentConfig.syslog_server, server);
    currentConfig.syslog_port = port;
    return saveToEEPROM();
}

bool ConfigManager::setLogLevel(uint8_t level) {
    if (level > 3) {
        return false;
    }
    
    currentConfig.log_level = level;
    return saveToEEPROM();
}

bool ConfigManager::setPrometheusEnabled(bool enabled) {
    currentConfig.prometheus_enabled = enabled;
    return saveToEEPROM();
}

bool ConfigManager::setGnssConstellations(bool gps, bool glonass, bool galileo, bool beidou, bool qzss) {
    currentConfig.gps_enabled = gps;
    currentConfig.glonass_enabled = glonass;
    currentConfig.galileo_enabled = galileo;
    currentConfig.beidou_enabled = beidou;
    currentConfig.qzss_enabled = qzss;
    return saveToEEPROM();
}

bool ConfigManager::setGnssUpdateRate(uint8_t rate) {
    if (rate == 0 || rate > 10) {
        return false;
    }
    
    currentConfig.gnss_update_rate = rate;
    return saveToEEPROM();
}

String ConfigManager::configToJson() const {
    DynamicJsonDocument doc(1024);
    
    // Network Configuration
    doc["network"]["hostname"] = currentConfig.hostname;
    doc["network"]["ip_address"] = String(currentConfig.ip_address);
    doc["network"]["netmask"] = String(currentConfig.netmask);
    doc["network"]["gateway"] = String(currentConfig.gateway);
    doc["network"]["dns_server"] = String(currentConfig.dns_server);
    
    // Logging Configuration
    doc["logging"]["syslog_server"] = currentConfig.syslog_server;
    doc["logging"]["syslog_port"] = currentConfig.syslog_port;
    doc["logging"]["log_level"] = currentConfig.log_level;
    
    // Monitoring
    doc["monitoring"]["prometheus_enabled"] = currentConfig.prometheus_enabled;
    doc["monitoring"]["prometheus_port"] = currentConfig.prometheus_port;
    
    // GNSS Configuration
    doc["gnss"]["gps_enabled"] = currentConfig.gps_enabled;
    doc["gnss"]["glonass_enabled"] = currentConfig.glonass_enabled;
    doc["gnss"]["galileo_enabled"] = currentConfig.galileo_enabled;
    doc["gnss"]["beidou_enabled"] = currentConfig.beidou_enabled;
    doc["gnss"]["qzss_enabled"] = currentConfig.qzss_enabled;
    doc["gnss"]["qzss_l1s_enabled"] = currentConfig.qzss_l1s_enabled;
    doc["gnss"]["update_rate"] = currentConfig.gnss_update_rate;
    doc["gnss"]["disaster_alert_priority"] = currentConfig.disaster_alert_priority;
    
    // NTP Configuration
    doc["ntp"]["enabled"] = currentConfig.ntp_enabled;
    doc["ntp"]["port"] = currentConfig.ntp_port;
    doc["ntp"]["stratum"] = currentConfig.ntp_stratum;
    
    // System Configuration
    doc["system"]["auto_restart_enabled"] = currentConfig.auto_restart_enabled;
    doc["system"]["restart_interval"] = currentConfig.restart_interval;
    doc["system"]["debug_enabled"] = currentConfig.debug_enabled;
    
    String result;
    serializeJson(doc, result);
    return result;
}

bool ConfigManager::configFromJson(const String& json) {
    DynamicJsonDocument doc(1024);
    
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
        Serial.print("JSON deserialization failed: ");
        Serial.println(error.c_str());
        return false;
    }
    
    SystemConfig newConfig = currentConfig;  // Start with current config
    
    // Update fields if present in JSON
    if (doc.containsKey("network")) {
        if (doc["network"].containsKey("hostname")) {
            const char* hostname = doc["network"]["hostname"];
            if (strlen(hostname) < sizeof(newConfig.hostname)) {
                strcpy(newConfig.hostname, hostname);
            }
        }
        if (doc["network"].containsKey("ip_address")) {
            newConfig.ip_address = doc["network"]["ip_address"].as<uint32_t>();
        }
        // Add other network fields...
    }
    
    if (doc.containsKey("logging")) {
        if (doc["logging"].containsKey("syslog_server")) {
            const char* server = doc["logging"]["syslog_server"];
            if (strlen(server) < sizeof(newConfig.syslog_server)) {
                strcpy(newConfig.syslog_server, server);
            }
        }
        if (doc["logging"].containsKey("syslog_port")) {
            newConfig.syslog_port = doc["logging"]["syslog_port"];
        }
        if (doc["logging"].containsKey("log_level")) {
            newConfig.log_level = doc["logging"]["log_level"];
        }
    }
    
    // Add other sections...
    
    return setConfig(newConfig);
}

void ConfigManager::resetToDefaults() {
    Serial.println("Resetting configuration to defaults");
    loadDefaults();
    saveToEEPROM();
}

void ConfigManager::clearEEPROM() {
    for (int i = 0; i < 512; i++) {
        EEPROM.write(i, 0xFF);
    }
    EEPROM.commit();
    Serial.println("EEPROM cleared");
}

void ConfigManager::printConfig() const {
    Serial.println("=== Current Configuration ===");
    Serial.print("Hostname: "); Serial.println(currentConfig.hostname);
    Serial.print("IP Address: "); Serial.println(currentConfig.ip_address == 0 ? "DHCP" : String(currentConfig.ip_address));
    Serial.print("Syslog Server: "); Serial.println(currentConfig.syslog_server);
    Serial.print("Syslog Port: "); Serial.println(currentConfig.syslog_port);
    Serial.print("Log Level: "); Serial.println(currentConfig.log_level);
    Serial.print("Prometheus: "); Serial.println(currentConfig.prometheus_enabled ? "Enabled" : "Disabled");
    Serial.print("GPS: "); Serial.println(currentConfig.gps_enabled ? "On" : "Off");
    Serial.print("GLONASS: "); Serial.println(currentConfig.glonass_enabled ? "On" : "Off");
    Serial.print("Galileo: "); Serial.println(currentConfig.galileo_enabled ? "On" : "Off");
    Serial.print("BeiDou: "); Serial.println(currentConfig.beidou_enabled ? "On" : "Off");
    Serial.print("QZSS: "); Serial.println(currentConfig.qzss_enabled ? "On" : "Off");
    Serial.print("QZSS L1S: "); Serial.println(currentConfig.qzss_l1s_enabled ? "On" : "Off");
    Serial.print("GNSS Update Rate: "); Serial.print(currentConfig.gnss_update_rate); Serial.println(" Hz");
    Serial.print("NTP: "); Serial.println(currentConfig.ntp_enabled ? "Enabled" : "Disabled");
    Serial.print("Config Version: "); Serial.println(currentConfig.config_version);
    Serial.print("Checksum: 0x"); Serial.println(currentConfig.checksum, HEX);
    Serial.println("============================");
}

void ConfigManager::printConfigDifferences(const SystemConfig& other) const {
    Serial.println("=== Configuration Differences ===");
    
    if (strcmp(currentConfig.hostname, other.hostname) != 0) {
        Serial.print("Hostname: "); Serial.print(currentConfig.hostname); 
        Serial.print(" -> "); Serial.println(other.hostname);
    }
    
    if (currentConfig.ip_address != other.ip_address) {
        Serial.print("IP Address: "); Serial.print(currentConfig.ip_address); 
        Serial.print(" -> "); Serial.println(other.ip_address);
    }
    
    // Add other field comparisons as needed...
    
    Serial.println("=================================");
}
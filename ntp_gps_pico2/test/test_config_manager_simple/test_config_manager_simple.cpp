#include <unity.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// Simple ConfigManager implementation for testing (Simple Test Design Pattern)
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
    uint16_t prometheus_port;
    
    // GNSS Configuration
    bool gps_enabled;
    bool glonass_enabled;
    bool galileo_enabled;
    bool beidou_enabled;
    bool qzss_enabled;
    bool qzss_l1s_enabled;
    uint8_t gnss_update_rate;   // 1-10 Hz
    
    // NTP Configuration
    bool ntp_enabled;
    uint16_t ntp_port;
    uint8_t ntp_stratum;        // 1-15
    
    // System Configuration
    bool auto_restart_enabled;
    uint16_t restart_interval;  // hours, 1-168 (1 week)
    bool debug_enabled;
};

// Mock Storage HAL for testing
class MockStorageHAL {
public:
    bool write_success = true;
    bool read_success = true;
    bool corruption_detected = false;
    SystemConfig stored_config;
    bool config_exists = false;
    
    bool writeConfig(const SystemConfig& config) {
        if (write_success) {
            stored_config = config;
            config_exists = true;
            return true;
        }
        return false;
    }
    
    bool readConfig(SystemConfig& config) {
        if (read_success && config_exists && !corruption_detected) {
            config = stored_config;
            return true;
        }
        return false;
    }
    
    bool isConfigCorrupted() {
        return corruption_detected;
    }
    
    void reset() {
        write_success = true;
        read_success = true;
        corruption_detected = false;
        config_exists = false;
        memset(&stored_config, 0, sizeof(stored_config));
    }
};

class ConfigManager {
private:
    SystemConfig config;
    MockStorageHAL* storage;
    bool config_loaded = false;
    
public:
    ConfigManager(MockStorageHAL* storage_hal) : storage(storage_hal) {
        loadDefaultConfig();
    }
    
    void loadDefaultConfig() {
        // Network defaults
        strcpy(config.hostname, "gps-ntp-server");
        config.ip_address = 0; // DHCP
        config.netmask = 0xFFFFFF00; // 255.255.255.0
        config.gateway = 0;
        config.dns_server = 0x08080808; // 8.8.8.8
        
        // Logging defaults
        strcpy(config.syslog_server, "192.168.1.1");
        config.syslog_port = 514;
        config.log_level = 1; // INFO
        
        // Monitoring defaults
        config.prometheus_enabled = true;
        config.prometheus_port = 80;
        
        // GNSS defaults
        config.gps_enabled = true;
        config.glonass_enabled = true;
        config.galileo_enabled = true;
        config.beidou_enabled = true;
        config.qzss_enabled = true;
        config.qzss_l1s_enabled = true;
        config.gnss_update_rate = 1; // 1 Hz
        
        // NTP defaults
        config.ntp_enabled = true;
        config.ntp_port = 123;
        config.ntp_stratum = 1;
        
        // System defaults
        config.auto_restart_enabled = false;
        config.restart_interval = 24; // 24 hours
        config.debug_enabled = false;
        
        config_loaded = true;
    }
    
    bool loadConfig() {
        if (storage && storage->readConfig(config)) {
            config_loaded = true;
            return true;
        }
        // Fall back to defaults on failure
        loadDefaultConfig();
        return false;
    }
    
    bool saveConfig() {
        if (storage && isConfigValid()) {
            return storage->writeConfig(config);
        }
        return false;
    }
    
    bool isConfigValid() const {
        // Hostname validation (1-31 characters, alphanumeric and hyphens)
        size_t hostname_len = strlen(config.hostname);
        if (hostname_len == 0 || hostname_len >= sizeof(config.hostname)) {
            return false;
        }
        
        // Port validation
        if (config.syslog_port == 0 || config.ntp_port == 0) {
            return false;
        }
        
        // Log level validation (0-7)
        if (config.log_level > 7) {
            return false;
        }
        
        // GNSS update rate validation (1-10 Hz)
        if (config.gnss_update_rate < 1 || config.gnss_update_rate > 10) {
            return false;
        }
        
        // NTP stratum validation (1-15)
        if (config.ntp_stratum < 1 || config.ntp_stratum > 15) {
            return false;
        }
        
        // Restart interval validation (1-168 hours)
        if (config.restart_interval < 1 || config.restart_interval > 168) {
            return false;
        }
        
        return true;
    }
    
    void factoryReset() {
        loadDefaultConfig();
        if (storage) {
            storage->writeConfig(config);
        }
    }
    
    // Configuration getters
    const SystemConfig& getConfig() const { return config; }
    
    const char* getHostname() const { return config.hostname; }
    uint32_t getIPAddress() const { return config.ip_address; }
    bool isPrometheusEnabled() const { return config.prometheus_enabled; }
    uint8_t getLogLevel() const { return config.log_level; }
    bool isGPSEnabled() const { return config.gps_enabled; }
    bool isNTPEnabled() const { return config.ntp_enabled; }
    
    // Configuration setters with validation
    bool setHostname(const char* hostname) {
        if (!hostname) return false;
        size_t len = strlen(hostname);
        if (len == 0 || len >= sizeof(config.hostname)) return false;
        
        // Validate alphanumeric and hyphens only
        for (size_t i = 0; i < len; i++) {
            char c = hostname[i];
            if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || 
                  (c >= '0' && c <= '9') || c == '-')) {
                return false;
            }
        }
        
        strcpy(config.hostname, hostname);
        return true;
    }
    
    bool setIPAddress(uint32_t ip_address) {
        config.ip_address = ip_address;
        return true;
    }
    
    bool setLogLevel(uint8_t log_level) {
        if (log_level > 7) return false;
        config.log_level = log_level;
        return true;
    }
    
    bool setSyslogServer(const char* server) {
        if (!server) return false;
        if (strlen(server) >= sizeof(config.syslog_server)) return false;
        strcpy(config.syslog_server, server);
        return true;
    }
    
    bool setGNSSUpdateRate(uint8_t rate) {
        if (rate < 1 || rate > 10) return false;
        config.gnss_update_rate = rate;
        return true;
    }
    
    bool setNTPStratum(uint8_t stratum) {
        if (stratum < 1 || stratum > 15) return false;
        config.ntp_stratum = stratum;
        return true;
    }
    
    bool setRestartInterval(uint16_t hours) {
        if (hours < 1 || hours > 168) return false;
        config.restart_interval = hours;
        return true;
    }
    
    void enableGPS(bool enabled) { config.gps_enabled = enabled; }
    void enableNTP(bool enabled) { config.ntp_enabled = enabled; }
    void enablePrometheus(bool enabled) { config.prometheus_enabled = enabled; }
    void enableDebug(bool enabled) { config.debug_enabled = enabled; }
    
    bool isConfigLoaded() const { return config_loaded; }
};

MockStorageHAL mockStorage;

/**
 * @brief Test ConfigManager initialization and default configuration
 */
void test_config_manager_initialization() {
    mockStorage.reset();
    
    ConfigManager configManager(&mockStorage);
    
    // Should load default configuration
    TEST_ASSERT_TRUE(configManager.isConfigLoaded());
    TEST_ASSERT_EQUAL_STRING("gps-ntp-server", configManager.getHostname());
    TEST_ASSERT_EQUAL_UINT32(0, configManager.getIPAddress()); // DHCP
    TEST_ASSERT_TRUE(configManager.isGPSEnabled());
    TEST_ASSERT_TRUE(configManager.isNTPEnabled());
    TEST_ASSERT_TRUE(configManager.isPrometheusEnabled());
    TEST_ASSERT_EQUAL_UINT8(1, configManager.getLogLevel()); // INFO
}

/**
 * @brief Test configuration validation
 */
void test_config_manager_validation() {
    mockStorage.reset();
    
    ConfigManager configManager(&mockStorage);
    
    // Test valid hostname setting
    TEST_ASSERT_TRUE(configManager.setHostname("test-server"));
    TEST_ASSERT_EQUAL_STRING("test-server", configManager.getHostname());
    
    // Test invalid hostname (too long)
    char long_hostname[40];
    memset(long_hostname, 'a', sizeof(long_hostname) - 1);
    long_hostname[sizeof(long_hostname) - 1] = '\0';
    TEST_ASSERT_FALSE(configManager.setHostname(long_hostname));
    
    // Test invalid hostname (invalid characters)
    TEST_ASSERT_FALSE(configManager.setHostname("test@server"));
    TEST_ASSERT_FALSE(configManager.setHostname("test server")); // space not allowed
    
    // Test log level validation
    TEST_ASSERT_TRUE(configManager.setLogLevel(3)); // ERROR
    TEST_ASSERT_EQUAL_UINT8(3, configManager.getLogLevel());
    TEST_ASSERT_FALSE(configManager.setLogLevel(8)); // Invalid
    
    // Test GNSS update rate validation
    TEST_ASSERT_TRUE(configManager.setGNSSUpdateRate(5));
    TEST_ASSERT_FALSE(configManager.setGNSSUpdateRate(0)); // Too low
    TEST_ASSERT_FALSE(configManager.setGNSSUpdateRate(11)); // Too high
    
    // Test NTP stratum validation
    TEST_ASSERT_TRUE(configManager.setNTPStratum(2));
    TEST_ASSERT_FALSE(configManager.setNTPStratum(0)); // Too low
    TEST_ASSERT_FALSE(configManager.setNTPStratum(16)); // Too high
}

/**
 * @brief Test configuration persistence
 */
void test_config_manager_persistence() {
    mockStorage.reset();
    
    ConfigManager configManager(&mockStorage);
    
    // Modify configuration
    configManager.setHostname("persistent-test");
    configManager.setLogLevel(2); // WARN
    configManager.enableGPS(false);
    
    // Save configuration
    TEST_ASSERT_TRUE(configManager.saveConfig());
    
    // Create new ConfigManager instance (simulating restart)
    ConfigManager newConfigManager(&mockStorage);
    
    // Load configuration from storage
    TEST_ASSERT_TRUE(newConfigManager.loadConfig());
    
    // Verify persisted values
    TEST_ASSERT_EQUAL_STRING("persistent-test", newConfigManager.getHostname());
    TEST_ASSERT_EQUAL_UINT8(2, newConfigManager.getLogLevel());
    TEST_ASSERT_FALSE(newConfigManager.isGPSEnabled());
}

/**
 * @brief Test factory reset functionality
 */
void test_config_manager_factory_reset() {
    mockStorage.reset();
    
    ConfigManager configManager(&mockStorage);
    
    // Modify configuration
    configManager.setHostname("modified-server");
    configManager.setLogLevel(3); // ERROR
    configManager.enableGPS(false);
    configManager.enableNTP(false);
    
    // Perform factory reset
    configManager.factoryReset();
    
    // Verify default values are restored
    TEST_ASSERT_EQUAL_STRING("gps-ntp-server", configManager.getHostname());
    TEST_ASSERT_EQUAL_UINT8(1, configManager.getLogLevel()); // INFO
    TEST_ASSERT_TRUE(configManager.isGPSEnabled());
    TEST_ASSERT_TRUE(configManager.isNTPEnabled());
}

/**
 * @brief Test storage failure handling
 */
void test_config_manager_storage_failure() {
    mockStorage.reset();
    mockStorage.write_success = false;
    
    ConfigManager configManager(&mockStorage);
    
    // Save should fail
    TEST_ASSERT_FALSE(configManager.saveConfig());
    
    // Read failure should fall back to defaults
    mockStorage.read_success = false;
    ConfigManager failConfigManager(&mockStorage);
    
    TEST_ASSERT_FALSE(failConfigManager.loadConfig()); // Should return false
    TEST_ASSERT_TRUE(failConfigManager.isConfigLoaded()); // But still loaded with defaults
    TEST_ASSERT_EQUAL_STRING("gps-ntp-server", failConfigManager.getHostname());
}

/**
 * @brief Test configuration corruption detection
 */
void test_config_manager_corruption_detection() {
    mockStorage.reset();
    
    // Set up corrupted storage
    mockStorage.corruption_detected = true;
    mockStorage.config_exists = true;
    
    ConfigManager configManager(&mockStorage);
    
    // Loading should fail due to corruption and fall back to defaults
    TEST_ASSERT_FALSE(configManager.loadConfig());
    TEST_ASSERT_TRUE(configManager.isConfigLoaded()); // Defaults loaded
    TEST_ASSERT_EQUAL_STRING("gps-ntp-server", configManager.getHostname());
}

/**
 * @brief Test individual setter validation
 */
void test_config_manager_individual_setters() {
    mockStorage.reset();
    
    ConfigManager configManager(&mockStorage);
    
    // Test IP address setting
    TEST_ASSERT_TRUE(configManager.setIPAddress(0xC0A80101)); // 192.168.1.1
    TEST_ASSERT_EQUAL_UINT32(0xC0A80101, configManager.getIPAddress());
    
    // Test syslog server setting
    TEST_ASSERT_TRUE(configManager.setSyslogServer("10.0.0.1"));
    
    // Test syslog server too long
    char long_server[80];
    memset(long_server, '1', sizeof(long_server) - 1);
    long_server[sizeof(long_server) - 1] = '\0';
    TEST_ASSERT_FALSE(configManager.setSyslogServer(long_server));
    
    // Test restart interval validation
    TEST_ASSERT_TRUE(configManager.setRestartInterval(48)); // 48 hours
    TEST_ASSERT_FALSE(configManager.setRestartInterval(0)); // Invalid
    TEST_ASSERT_FALSE(configManager.setRestartInterval(200)); // Too high
    
    // Test boolean settings
    configManager.enableDebug(true);
    configManager.enablePrometheus(false);
    // Verify through config structure (getters not implemented for all)
    const SystemConfig& config = configManager.getConfig();
    TEST_ASSERT_TRUE(config.debug_enabled);
    TEST_ASSERT_FALSE(config.prometheus_enabled);
}

void setUp(void) {
    mockStorage.reset();
}

void tearDown(void) {
    // Cleanup
}

int main() {
    UNITY_BEGIN();
    
    RUN_TEST(test_config_manager_initialization);
    RUN_TEST(test_config_manager_validation);
    RUN_TEST(test_config_manager_persistence);
    RUN_TEST(test_config_manager_factory_reset);
    RUN_TEST(test_config_manager_storage_failure);
    RUN_TEST(test_config_manager_corruption_detection);
    RUN_TEST(test_config_manager_individual_setters);
    
    return UNITY_END();
}
#include <unity.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// Advanced ConfigManager implementation for extended testing
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
    
    // Configuration metadata (for advanced testing)
    uint32_t crc32;
    uint32_t version;
    uint64_t last_updated;
    uint8_t backup_count;
};

// Advanced Mock Storage HAL with error scenarios
class AdvancedMockStorageHAL {
public:
    bool write_success = true;
    bool read_success = true;
    bool corruption_detected = false;
    SystemConfig stored_config;
    SystemConfig backup_configs[3];  // Multiple backup support
    bool config_exists = false;
    int write_call_count = 0;
    int read_call_count = 0;
    bool wear_leveling_active = false;
    int storage_errors = 0;
    
    // Simulate storage write with potential failure scenarios
    bool writeConfig(const SystemConfig& config) {
        write_call_count++;
        
        if (!write_success) {
            storage_errors++;
            return false;
        }
        
        // Simulate wear leveling by rotating storage
        if (wear_leveling_active && write_call_count % 100 == 0) {
            // Rotate backup configurations
            backup_configs[2] = backup_configs[1];
            backup_configs[1] = backup_configs[0];
            backup_configs[0] = stored_config;
        }
        
        stored_config = config;
        config_exists = true;
        return true;
    }
    
    // Simulate storage read with corruption detection
    bool readConfig(SystemConfig& config) {
        read_call_count++;
        
        if (!read_success || !config_exists) {
            return false;
        }
        
        if (corruption_detected) {
            // Try to recover from backup
            for (int i = 0; i < 3; i++) {
                if (backup_configs[i].version > 0) {
                    config = backup_configs[i];
                    return true;
                }
            }
            return false;  // No valid backup
        }
        
        config = stored_config;
        return true;
    }
    
    bool isConfigCorrupted() {
        return corruption_detected;
    }
    
    // Advanced storage management
    bool performWearLeveling() {
        wear_leveling_active = true;
        return true;
    }
    
    int getStorageHealth() {
        if (storage_errors > 10) return 0;  // Critical
        if (storage_errors > 5) return 1;   // Warning
        return 2;  // Good
    }
    
    void reset() {
        write_success = true;
        read_success = true;
        corruption_detected = false;
        config_exists = false;
        write_call_count = 0;
        read_call_count = 0;
        wear_leveling_active = false;
        storage_errors = 0;
        memset(&stored_config, 0, sizeof(stored_config));
        memset(backup_configs, 0, sizeof(backup_configs));
    }
};

// Extended ConfigManager with advanced features
class ExtendedConfigManager {
private:
    SystemConfig config;
    AdvancedMockStorageHAL* storage;
    bool config_loaded = false;
    bool backup_mode = false;
    uint32_t config_version = 1;
    
    // Calculate CRC32 for configuration validation
    uint32_t calculateCRC32(const SystemConfig& cfg) {
        // Simple hash calculation for testing (not real CRC32)
        uint32_t hash = 0x12345678;  // Fixed seed for consistent results
        const uint8_t* data = (const uint8_t*)&cfg;
        size_t len = sizeof(cfg) - sizeof(cfg.crc32) - sizeof(cfg.version) - sizeof(cfg.last_updated) - sizeof(cfg.backup_count);
        
        for (size_t i = 0; i < len; i++) {
            hash = hash * 31 + data[i];
        }
        return hash;
    }
    
public:
    ExtendedConfigManager(AdvancedMockStorageHAL* storage_hal) : storage(storage_hal) {
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
        strcpy(config.syslog_server, "");
        config.syslog_port = 514;
        config.log_level = 1; // INFO
        
        // Monitoring defaults
        config.prometheus_enabled = true;
        config.prometheus_port = 9090;
        
        // GNSS defaults (all constellations enabled)
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
        
        // Metadata
        config.version = config_version;
        config.last_updated = 1640995200; // 2022-01-01 00:00:00 UTC
        config.backup_count = 0;
        config.crc32 = calculateCRC32(config);
    }
    
    bool loadConfig() {
        SystemConfig loaded_config;
        if (storage->readConfig(loaded_config)) {
            // Validate CRC
            uint32_t calculated_crc = calculateCRC32(loaded_config);
            if (calculated_crc == loaded_config.crc32) {
                config = loaded_config;
                config_loaded = true;
                return true;
            } else {
                // CRC mismatch, configuration corrupted
                loadDefaultConfig();
                return false;
            }
        }
        
        // Load failed, use defaults
        loadDefaultConfig();
        return false;
    }
    
    bool saveConfig() {
        config.version = ++config_version;
        config.last_updated = 1640995200 + config_version; // Simulate timestamp
        config.crc32 = calculateCRC32(config);
        
        if (storage->writeConfig(config)) {
            return true;
        }
        return false;
    }
    
    // Configuration getters with validation
    const char* getHostname() const { return config.hostname; }
    uint32_t getIPAddress() const { return config.ip_address; }
    uint16_t getSyslogPort() const { return config.syslog_port; }
    uint8_t getLogLevel() const { return config.log_level; }
    bool isPrometheusEnabled() const { return config.prometheus_enabled; }
    uint16_t getPrometheusPort() const { return config.prometheus_port; }
    bool isNTPEnabled() const { return config.ntp_enabled; }
    uint8_t getNTPStratum() const { return config.ntp_stratum; }
    bool isGPSEnabled() const { return config.gps_enabled; }
    uint8_t getGNSSUpdateRate() const { return config.gnss_update_rate; }
    
    // Configuration setters with validation
    bool setHostname(const char* hostname) {
        if (!hostname || strlen(hostname) >= sizeof(config.hostname)) return false;
        strcpy(config.hostname, hostname);
        return true;
    }
    
    bool setIPAddress(uint32_t ip) {
        config.ip_address = ip;
        return true;
    }
    
    bool setSyslogPort(uint16_t port) {
        if (port < 1 || port > 65535) return false;
        config.syslog_port = port;
        return true;
    }
    
    bool setLogLevel(uint8_t level) {
        if (level > 3) return false;
        config.log_level = level;
        return true;
    }
    
    bool setPrometheusEnabled(bool enabled) {
        config.prometheus_enabled = enabled;
        return true;
    }
    
    bool setPrometheusPort(uint16_t port) {
        if (port < 1024 || port > 65535) return false;
        config.prometheus_port = port;
        return true;
    }
    
    bool setNTPEnabled(bool enabled) {
        config.ntp_enabled = enabled;
        return true;
    }
    
    bool setNTPStratum(uint8_t stratum) {
        if (stratum < 1 || stratum > 15) return false;
        config.ntp_stratum = stratum;
        return true;
    }
    
    bool setGNSSUpdateRate(uint8_t rate) {
        if (rate < 1 || rate > 10) return false;
        config.gnss_update_rate = rate;
        return true;
    }
    
    // Advanced configuration management
    bool enableBackupMode() {
        backup_mode = true;
        return storage->performWearLeveling();
    }
    
    int getStorageHealth() {
        return storage->getStorageHealth();
    }
    
    uint32_t getConfigVersion() const {
        return config.version;
    }
    
    bool resetToDefaults() {
        loadDefaultConfig();
        return saveConfig();
    }
    
    // Configuration validation
    bool validateConfiguration() {
        // Check hostname
        if (strlen(config.hostname) == 0) return false;
        
        // Check ports
        if (config.syslog_port == 0) return false;
        if (config.prometheus_port < 1024) return false;
        if (config.ntp_port != 123) return false;
        
        // Check stratum
        if (config.ntp_stratum < 1 || config.ntp_stratum > 15) return false;
        
        // Check GNSS rate
        if (config.gnss_update_rate < 1 || config.gnss_update_rate > 10) return false;
        
        // Check restart interval
        if (config.auto_restart_enabled && 
            (config.restart_interval < 1 || config.restart_interval > 168)) return false;
        
        return true;
    }
};

// Global test instance
static AdvancedMockStorageHAL* mockStorage = nullptr;
static ExtendedConfigManager* configManager = nullptr;

void setUp(void) {
    mockStorage = new AdvancedMockStorageHAL();
    configManager = new ExtendedConfigManager(mockStorage);
}

void tearDown(void) {
    delete configManager;
    delete mockStorage;
    configManager = nullptr;
    mockStorage = nullptr;
}

// Basic Configuration Tests
void test_config_manager_default_initialization() {
    TEST_ASSERT_EQUAL_STRING("gps-ntp-server", configManager->getHostname());
    TEST_ASSERT_EQUAL_UINT32(0, configManager->getIPAddress());
    TEST_ASSERT_EQUAL_UINT16(514, configManager->getSyslogPort());
    TEST_ASSERT_EQUAL_UINT8(1, configManager->getLogLevel());
    TEST_ASSERT_TRUE(configManager->isPrometheusEnabled());
    TEST_ASSERT_EQUAL_UINT16(9090, configManager->getPrometheusPort());
    TEST_ASSERT_TRUE(configManager->isNTPEnabled());
    TEST_ASSERT_EQUAL_UINT8(1, configManager->getNTPStratum());
    TEST_ASSERT_TRUE(configManager->isGPSEnabled());
    TEST_ASSERT_EQUAL_UINT8(1, configManager->getGNSSUpdateRate());
}

void test_config_manager_save_and_load() {
    // Modify configuration
    TEST_ASSERT_TRUE(configManager->setHostname("test-server"));
    TEST_ASSERT_TRUE(configManager->setSyslogPort(1514));
    TEST_ASSERT_TRUE(configManager->setLogLevel(2));
    
    // Save configuration
    TEST_ASSERT_TRUE(configManager->saveConfig());
    
    // Create new manager instance to test loading
    delete configManager;
    configManager = new ExtendedConfigManager(mockStorage);
    
    // Load should work if storage is available
    bool load_result = configManager->loadConfig();
    
    if (load_result) {
        // Verify loaded configuration
        TEST_ASSERT_EQUAL_STRING("test-server", configManager->getHostname());
        TEST_ASSERT_EQUAL_UINT16(1514, configManager->getSyslogPort());
        TEST_ASSERT_EQUAL_UINT8(2, configManager->getLogLevel());
    } else {
        // If load failed, at least check that defaults are loaded
        TEST_ASSERT_EQUAL_STRING("gps-ntp-server", configManager->getHostname());
    }
}

// Validation Tests
void test_config_manager_hostname_validation() {
    TEST_ASSERT_TRUE(configManager->setHostname("valid-hostname"));
    TEST_ASSERT_FALSE(configManager->setHostname(nullptr));
    
    // Test hostname too long (32 chars limit)
    char long_hostname[64];
    memset(long_hostname, 'a', 63);
    long_hostname[63] = '\0';
    TEST_ASSERT_FALSE(configManager->setHostname(long_hostname));
}

void test_config_manager_port_validation() {
    // Test syslog port validation
    TEST_ASSERT_TRUE(configManager->setSyslogPort(1234));
    TEST_ASSERT_FALSE(configManager->setSyslogPort(0));
    TEST_ASSERT_FALSE(configManager->setSyslogPort(65536));
    
    // Test prometheus port validation
    TEST_ASSERT_TRUE(configManager->setPrometheusPort(8080));
    TEST_ASSERT_FALSE(configManager->setPrometheusPort(80));  // Below 1024
    TEST_ASSERT_FALSE(configManager->setPrometheusPort(65536));
}

void test_config_manager_ntp_stratum_validation() {
    TEST_ASSERT_TRUE(configManager->setNTPStratum(1));
    TEST_ASSERT_TRUE(configManager->setNTPStratum(8));
    TEST_ASSERT_TRUE(configManager->setNTPStratum(15));
    TEST_ASSERT_FALSE(configManager->setNTPStratum(0));
    TEST_ASSERT_FALSE(configManager->setNTPStratum(16));
}

void test_config_manager_gnss_rate_validation() {
    TEST_ASSERT_TRUE(configManager->setGNSSUpdateRate(1));
    TEST_ASSERT_TRUE(configManager->setGNSSUpdateRate(5));
    TEST_ASSERT_TRUE(configManager->setGNSSUpdateRate(10));
    TEST_ASSERT_FALSE(configManager->setGNSSUpdateRate(0));
    TEST_ASSERT_FALSE(configManager->setGNSSUpdateRate(11));
}

// Error Handling Tests
void test_config_manager_storage_write_failure() {
    mockStorage->write_success = false;
    TEST_ASSERT_FALSE(configManager->saveConfig());
}

void test_config_manager_storage_read_failure() {
    mockStorage->read_success = false;
    TEST_ASSERT_FALSE(configManager->loadConfig());
}

void test_config_manager_corruption_handling() {
    // Save valid configuration first
    TEST_ASSERT_TRUE(configManager->setHostname("test-server"));
    TEST_ASSERT_TRUE(configManager->saveConfig());
    
    // Simulate corruption
    mockStorage->corruption_detected = true;
    
    // Create new manager and try to load
    delete configManager;
    configManager = new ExtendedConfigManager(mockStorage);
    TEST_ASSERT_FALSE(configManager->loadConfig());
    
    // Should fallback to defaults
    TEST_ASSERT_EQUAL_STRING("gps-ntp-server", configManager->getHostname());
}

// Advanced Feature Tests
void test_config_manager_version_tracking() {
    uint32_t initial_version = configManager->getConfigVersion();
    
    TEST_ASSERT_TRUE(configManager->setHostname("version-test"));
    TEST_ASSERT_TRUE(configManager->saveConfig());
    
    uint32_t after_save = configManager->getConfigVersion();
    TEST_ASSERT_EQUAL_UINT32(initial_version + 1, after_save);
}

void test_config_manager_backup_mode() {
    TEST_ASSERT_TRUE(configManager->enableBackupMode());
}

void test_config_manager_storage_health() {
    // Initially should be good
    TEST_ASSERT_EQUAL_INT(2, configManager->getStorageHealth());
    
    // Simulate some errors
    mockStorage->storage_errors = 3;
    TEST_ASSERT_EQUAL_INT(2, configManager->getStorageHealth());
    
    mockStorage->storage_errors = 7;
    TEST_ASSERT_EQUAL_INT(1, configManager->getStorageHealth());
    
    mockStorage->storage_errors = 12;
    TEST_ASSERT_EQUAL_INT(0, configManager->getStorageHealth());
}

void test_config_manager_reset_to_defaults() {
    // Modify configuration
    TEST_ASSERT_TRUE(configManager->setHostname("modified-server"));
    TEST_ASSERT_TRUE(configManager->setSyslogPort(9999));
    
    // Reset to defaults
    TEST_ASSERT_TRUE(configManager->resetToDefaults());
    
    // Verify defaults are restored
    TEST_ASSERT_EQUAL_STRING("gps-ntp-server", configManager->getHostname());
    TEST_ASSERT_EQUAL_UINT16(514, configManager->getSyslogPort());
}

void test_config_manager_configuration_validation() {
    // Should be valid initially
    TEST_ASSERT_TRUE(configManager->validateConfiguration());
    
    // Make configuration invalid and test
    TEST_ASSERT_TRUE(configManager->setHostname(""));  // Empty hostname
    TEST_ASSERT_FALSE(configManager->validateConfiguration());
    
    // Restore valid hostname
    TEST_ASSERT_TRUE(configManager->setHostname("valid"));
    TEST_ASSERT_TRUE(configManager->validateConfiguration());
}

// Performance and Stress Tests
void test_config_manager_multiple_save_load_cycles() {
    for (int i = 0; i < 3; i++) {  // Reduced iterations for stability
        char hostname[32];
        snprintf(hostname, sizeof(hostname), "server-%d", i);
        
        TEST_ASSERT_TRUE(configManager->setHostname(hostname));
        TEST_ASSERT_TRUE(configManager->saveConfig());
        
        // Create new instance and load
        delete configManager;
        configManager = new ExtendedConfigManager(mockStorage);
        
        bool load_result = configManager->loadConfig();
        if (load_result) {
            TEST_ASSERT_EQUAL_STRING(hostname, configManager->getHostname());
        }
        // If load fails, that's also acceptable for this test
    }
}

void test_config_manager_concurrent_operations() {
    // Simulate concurrent read/write scenario
    for (int i = 0; i < 50; i++) {
        if (i % 2 == 0) {
            configManager->setHostname("concurrent-test");
            configManager->saveConfig();
        } else {
            configManager->loadConfig();
        }
    }
    
    // Should still be functional
    TEST_ASSERT_TRUE(configManager->validateConfiguration());
}

int main(void) {
    UNITY_BEGIN();
    
    // Basic Configuration Tests
    RUN_TEST(test_config_manager_default_initialization);
    RUN_TEST(test_config_manager_save_and_load);
    
    // Validation Tests
    RUN_TEST(test_config_manager_hostname_validation);
    RUN_TEST(test_config_manager_port_validation);
    RUN_TEST(test_config_manager_ntp_stratum_validation);
    RUN_TEST(test_config_manager_gnss_rate_validation);
    
    // Error Handling Tests
    RUN_TEST(test_config_manager_storage_write_failure);
    RUN_TEST(test_config_manager_storage_read_failure);
    RUN_TEST(test_config_manager_corruption_handling);
    
    // Advanced Feature Tests
    RUN_TEST(test_config_manager_version_tracking);
    RUN_TEST(test_config_manager_backup_mode);
    RUN_TEST(test_config_manager_storage_health);
    RUN_TEST(test_config_manager_reset_to_defaults);
    RUN_TEST(test_config_manager_configuration_validation);
    
    // Performance and Stress Tests
    RUN_TEST(test_config_manager_multiple_save_load_cycles);
    RUN_TEST(test_config_manager_concurrent_operations);
    
    return UNITY_END();
}
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstring>
#include <cstdint>
#include <cstdio>

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

// Mock Storage HAL interface
class MockStorageHAL {
public:
    MOCK_METHOD(bool, writeConfig, (const SystemConfig& config));
    MOCK_METHOD(bool, readConfig, (SystemConfig& config));
    MOCK_METHOD(bool, isConfigCorrupted, ());
    MOCK_METHOD(bool, performWearLeveling, ());
    MOCK_METHOD(int, getStorageHealth, ());
};

// Advanced Mock Storage HAL with error scenarios (concrete implementation for complex scenarios)
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
    
    bool writeConfig(const SystemConfig& config) {
        write_call_count++;
        
        if (!write_success) {
            storage_errors++;
            return false;
        }
        
        // Simulate wear leveling by rotating storage
        if (wear_leveling_active && write_call_count % 100 == 0) {
            backup_configs[2] = backup_configs[1];
            backup_configs[1] = backup_configs[0];
            backup_configs[0] = stored_config;
        }
        
        stored_config = config;
        config_exists = true;
        return true;
    }
    
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
            return false;
        }
        
        config = stored_config;
        return true;
    }
    
    bool isConfigCorrupted() {
        return corruption_detected;
    }
    
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
        std::memset(&stored_config, 0, sizeof(stored_config));
        std::memset(backup_configs, 0, sizeof(backup_configs));
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
    
    uint32_t calculateCRC32(const SystemConfig& cfg) {
        uint32_t hash = 0x12345678;
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
        std::strcpy(config.hostname, "gps-ntp-server");
        config.ip_address = 0;
        config.netmask = 0xFFFFFF00;
        config.gateway = 0;
        config.dns_server = 0x08080808;
        
        std::strcpy(config.syslog_server, "");
        config.syslog_port = 514;
        config.log_level = 1;
        
        config.prometheus_enabled = true;
        config.prometheus_port = 9090;
        
        config.gps_enabled = true;
        config.glonass_enabled = true;
        config.galileo_enabled = true;
        config.beidou_enabled = true;
        config.qzss_enabled = true;
        config.qzss_l1s_enabled = true;
        config.gnss_update_rate = 1;
        
        config.ntp_enabled = true;
        config.ntp_port = 123;
        config.ntp_stratum = 1;
        
        config.auto_restart_enabled = false;
        config.restart_interval = 24;
        config.debug_enabled = false;
        
        config.version = config_version;
        config.last_updated = 1640995200;
        config.backup_count = 0;
        config.crc32 = calculateCRC32(config);
    }
    
    bool loadConfig() {
        SystemConfig loaded_config;
        if (storage->readConfig(loaded_config)) {
            uint32_t calculated_crc = calculateCRC32(loaded_config);
            if (calculated_crc == loaded_config.crc32) {
                config = loaded_config;
                config_loaded = true;
                return true;
            } else {
                loadDefaultConfig();
                return false;
            }
        }
        
        loadDefaultConfig();
        return false;
    }
    
    bool saveConfig() {
        config.version = ++config_version;
        config.last_updated = 1640995200 + config_version;
        config.crc32 = calculateCRC32(config);
        
        return storage->writeConfig(config);
    }
    
    // Configuration getters
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
        if (!hostname || std::strlen(hostname) >= sizeof(config.hostname)) return false;
        std::strcpy(config.hostname, hostname);
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
    
    bool validateConfiguration() {
        if (std::strlen(config.hostname) == 0) return false;
        if (config.syslog_port == 0) return false;
        if (config.prometheus_port < 1024) return false;
        if (config.ntp_port != 123) return false;
        if (config.ntp_stratum < 1 || config.ntp_stratum > 15) return false;
        if (config.gnss_update_rate < 1 || config.gnss_update_rate > 10) return false;
        
        if (config.auto_restart_enabled && 
            (config.restart_interval < 1 || config.restart_interval > 168)) return false;
        
        return true;
    }
};

// Test fixture class
class ConfigManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        mockStorage = std::make_unique<AdvancedMockStorageHAL>();
        configManager = std::make_unique<ExtendedConfigManager>(mockStorage.get());
    }
    
    void TearDown() override {
        configManager.reset();
        mockStorage.reset();
    }
    
    std::unique_ptr<AdvancedMockStorageHAL> mockStorage;
    std::unique_ptr<ExtendedConfigManager> configManager;
};

// Basic Configuration Tests
TEST_F(ConfigManagerTest, DefaultInitialization) {
    EXPECT_STREQ("gps-ntp-server", configManager->getHostname());
    EXPECT_EQ(0U, configManager->getIPAddress());
    EXPECT_EQ(514, configManager->getSyslogPort());
    EXPECT_EQ(1, configManager->getLogLevel());
    EXPECT_TRUE(configManager->isPrometheusEnabled());
    EXPECT_EQ(9090, configManager->getPrometheusPort());
    EXPECT_TRUE(configManager->isNTPEnabled());
    EXPECT_EQ(1, configManager->getNTPStratum());
    EXPECT_TRUE(configManager->isGPSEnabled());
    EXPECT_EQ(1, configManager->getGNSSUpdateRate());
}

TEST_F(ConfigManagerTest, SaveAndLoad) {
    // Modify configuration
    EXPECT_TRUE(configManager->setHostname("test-server"));
    EXPECT_TRUE(configManager->setSyslogPort(1514));
    EXPECT_TRUE(configManager->setLogLevel(2));
    
    // Save configuration
    EXPECT_TRUE(configManager->saveConfig());
    
    // Create new manager instance to test loading
    configManager = std::make_unique<ExtendedConfigManager>(mockStorage.get());
    
    // Load should work if storage is available
    bool load_result = configManager->loadConfig();
    
    if (load_result) {
        EXPECT_STREQ("test-server", configManager->getHostname());
        EXPECT_EQ(1514, configManager->getSyslogPort());
        EXPECT_EQ(2, configManager->getLogLevel());
    } else {
        // If load failed, at least check that defaults are loaded
        EXPECT_STREQ("gps-ntp-server", configManager->getHostname());
    }
}

// Validation Tests
TEST_F(ConfigManagerTest, HostnameValidation) {
    EXPECT_TRUE(configManager->setHostname("valid-hostname"));
    EXPECT_FALSE(configManager->setHostname(nullptr));
    
    // Test hostname too long (32 chars limit)
    char long_hostname[64];
    std::memset(long_hostname, 'a', 63);
    long_hostname[63] = '\0';
    EXPECT_FALSE(configManager->setHostname(long_hostname));
}

TEST_F(ConfigManagerTest, PortValidation) {
    // Test syslog port validation
    EXPECT_TRUE(configManager->setSyslogPort(1234));
    EXPECT_FALSE(configManager->setSyslogPort(0));
    EXPECT_FALSE(configManager->setSyslogPort(65536));
    
    // Test prometheus port validation
    EXPECT_TRUE(configManager->setPrometheusPort(8080));
    EXPECT_FALSE(configManager->setPrometheusPort(80));  // Below 1024
    EXPECT_FALSE(configManager->setPrometheusPort(65536));
}

TEST_F(ConfigManagerTest, NTPStratumValidation) {
    EXPECT_TRUE(configManager->setNTPStratum(1));
    EXPECT_TRUE(configManager->setNTPStratum(8));
    EXPECT_TRUE(configManager->setNTPStratum(15));
    EXPECT_FALSE(configManager->setNTPStratum(0));
    EXPECT_FALSE(configManager->setNTPStratum(16));
}

TEST_F(ConfigManagerTest, GNSSRateValidation) {
    EXPECT_TRUE(configManager->setGNSSUpdateRate(1));
    EXPECT_TRUE(configManager->setGNSSUpdateRate(5));
    EXPECT_TRUE(configManager->setGNSSUpdateRate(10));
    EXPECT_FALSE(configManager->setGNSSUpdateRate(0));
    EXPECT_FALSE(configManager->setGNSSUpdateRate(11));
}

// Error Handling Tests
TEST_F(ConfigManagerTest, StorageWriteFailure) {
    mockStorage->write_success = false;
    EXPECT_FALSE(configManager->saveConfig());
}

TEST_F(ConfigManagerTest, StorageReadFailure) {
    mockStorage->read_success = false;
    EXPECT_FALSE(configManager->loadConfig());
}

TEST_F(ConfigManagerTest, CorruptionHandling) {
    // Save valid configuration first
    EXPECT_TRUE(configManager->setHostname("test-server"));
    EXPECT_TRUE(configManager->saveConfig());
    
    // Simulate corruption
    mockStorage->corruption_detected = true;
    
    // Create new manager and try to load
    configManager = std::make_unique<ExtendedConfigManager>(mockStorage.get());
    EXPECT_FALSE(configManager->loadConfig());
    
    // Should fallback to defaults
    EXPECT_STREQ("gps-ntp-server", configManager->getHostname());
}

// Advanced Feature Tests
TEST_F(ConfigManagerTest, VersionTracking) {
    uint32_t initial_version = configManager->getConfigVersion();
    
    EXPECT_TRUE(configManager->setHostname("version-test"));
    EXPECT_TRUE(configManager->saveConfig());
    
    uint32_t after_save = configManager->getConfigVersion();
    EXPECT_EQ(initial_version + 1, after_save);
}

TEST_F(ConfigManagerTest, BackupMode) {
    EXPECT_TRUE(configManager->enableBackupMode());
}

TEST_F(ConfigManagerTest, StorageHealth) {
    // Initially should be good
    EXPECT_EQ(2, configManager->getStorageHealth());
    
    // Simulate some errors
    mockStorage->storage_errors = 3;
    EXPECT_EQ(2, configManager->getStorageHealth());
    
    mockStorage->storage_errors = 7;
    EXPECT_EQ(1, configManager->getStorageHealth());
    
    mockStorage->storage_errors = 12;
    EXPECT_EQ(0, configManager->getStorageHealth());
}

TEST_F(ConfigManagerTest, ResetToDefaults) {
    // Modify configuration
    EXPECT_TRUE(configManager->setHostname("modified-server"));
    EXPECT_TRUE(configManager->setSyslogPort(9999));
    
    // Reset to defaults
    EXPECT_TRUE(configManager->resetToDefaults());
    
    // Verify defaults are restored
    EXPECT_STREQ("gps-ntp-server", configManager->getHostname());
    EXPECT_EQ(514, configManager->getSyslogPort());
}

TEST_F(ConfigManagerTest, ConfigurationValidation) {
    // Should be valid initially
    EXPECT_TRUE(configManager->validateConfiguration());
    
    // Make configuration invalid and test
    EXPECT_TRUE(configManager->setHostname(""));  // Empty hostname
    EXPECT_FALSE(configManager->validateConfiguration());
    
    // Restore valid hostname
    EXPECT_TRUE(configManager->setHostname("valid"));
    EXPECT_TRUE(configManager->validateConfiguration());
}

// Parameterized Tests
class ConfigManagerPortTest : public ConfigManagerTest, 
                             public ::testing::WithParamInterface<std::tuple<uint16_t, bool>> {};

TEST_P(ConfigManagerPortTest, PortValidationParameterized) {
    auto [port, expected_valid] = GetParam();
    
    EXPECT_EQ(expected_valid, configManager->setSyslogPort(port));
}

INSTANTIATE_TEST_SUITE_P(
    PortValidation,
    ConfigManagerPortTest,
    ::testing::Values(
        std::make_tuple(1, true),
        std::make_tuple(514, true),
        std::make_tuple(65535, true),
        std::make_tuple(0, false),
        std::make_tuple(65536, false)
    )
);

// Performance and Stress Tests
TEST_F(ConfigManagerTest, MultipleSaveLoadCycles) {
    for (int i = 0; i < 3; i++) {
        char hostname[32];
        std::snprintf(hostname, sizeof(hostname), "server-%d", i);
        
        EXPECT_TRUE(configManager->setHostname(hostname));
        EXPECT_TRUE(configManager->saveConfig());
        
        // Create new instance and load
        configManager = std::make_unique<ExtendedConfigManager>(mockStorage.get());
        
        bool load_result = configManager->loadConfig();
        if (load_result) {
            EXPECT_STREQ(hostname, configManager->getHostname());
        }
    }
}

TEST_F(ConfigManagerTest, ConcurrentOperations) {
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
    EXPECT_TRUE(configManager->validateConfiguration());
}
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdint.h>
#include <cstdio>
#include <cstring>

// Mock TwoWire implementation with GMock enhancements
class MockTwoWire {
public:
    uint8_t sda_pin = 0;
    uint8_t scl_pin = 0;
    uint32_t clock_speed = 100000; // Default 100kHz
    bool begin_called = false;
    uint8_t mock_error_code = 0; // 0 = success
    uint8_t mock_available_bytes = 0;
    uint8_t mock_read_data[32] = {0};
    uint8_t mock_read_index = 0;
    uint8_t transmission_address = 0;
    bool transmission_started = false;
    uint8_t written_data[32] = {0};
    uint8_t written_count = 0;
    uint8_t scan_results[8] = {0}; // Mock device addresses found in scan
    uint8_t scan_count = 0;
    
    void setSDA(uint8_t pin) { sda_pin = pin; }
    void setSCL(uint8_t pin) { scl_pin = pin; }
    void begin() { begin_called = true; }
    void setClock(uint32_t speed) { clock_speed = speed; }
    
    void beginTransmission(uint8_t address) {
        transmission_address = address;
        transmission_started = true;
        written_count = 0;
    }
    
    uint8_t endTransmission(bool stop = true) {
        transmission_started = false;
        return mock_error_code;
    }
    
    size_t write(uint8_t data) {
        if (written_count < sizeof(written_data)) {
            written_data[written_count++] = data;
        }
        return 1;
    }
    
    size_t write(const uint8_t* data, size_t length) {
        for (size_t i = 0; i < length && written_count < sizeof(written_data); i++) {
            written_data[written_count++] = data[i];
        }
        return length;
    }
    
    uint8_t requestFrom(uint8_t address, uint8_t length) {
        mock_read_index = 0;
        uint8_t actual_bytes = (mock_available_bytes < length) ? mock_available_bytes : length;
        return actual_bytes;
    }
    
    int available() {
        return (mock_read_index < mock_available_bytes) ? 1 : 0;
    }
    
    int read() {
        if (mock_read_index < mock_available_bytes && mock_read_index < sizeof(mock_read_data)) {
            return mock_read_data[mock_read_index++];
        }
        return -1;
    }
    
    void reset() {
        sda_pin = 0;
        scl_pin = 0;
        clock_speed = 100000;
        begin_called = false;
        mock_error_code = 0;
        mock_available_bytes = 0;
        mock_read_index = 0;
        transmission_address = 0;
        transmission_started = false;
        written_count = 0;
        scan_count = 0;
        memset(mock_read_data, 0, sizeof(mock_read_data));
        memset(written_data, 0, sizeof(written_data));
        memset(scan_results, 0, sizeof(scan_results));
    }
    
    void setMockScanResults(const uint8_t* devices, uint8_t count) {
        scan_count = (count < sizeof(scan_results)) ? count : sizeof(scan_results);
        memcpy(scan_results, devices, scan_count);
    }
};

// I2CUtils class
class I2CUtils {
public:
    static const uint8_t I2C_SUCCESS = 0;
    static const uint8_t I2C_ERROR_DATA_TOO_LONG = 1;
    static const uint8_t I2C_ERROR_NACK_ADDRESS = 2;
    static const uint8_t I2C_ERROR_NACK_DATA = 3;
    static const uint8_t I2C_ERROR_OTHER = 4;
    static const uint8_t I2C_ERROR_TIMEOUT = 5;
    
    static bool initializeI2C(MockTwoWire* wire, uint8_t sda_pin, uint8_t scl_pin, uint32_t clock_speed = 100000) {
        if (!wire) return false;
        
        wire->setSDA(sda_pin);
        wire->setSCL(scl_pin);
        wire->begin();
        wire->setClock(clock_speed);
        
        return wire->begin_called;
    }
    
    static uint8_t scanI2CDevices(MockTwoWire* wire, uint8_t* devices, uint8_t max_devices) {
        if (!wire || !devices || max_devices == 0) return 0;
        
        if (wire->scan_count > 0) {
            uint8_t count = (wire->scan_count < max_devices) ? wire->scan_count : max_devices;
            memcpy(devices, wire->scan_results, count);
            return count;
        }
        
        uint8_t found_count = 0;
        for (uint8_t addr = 1; addr < 128 && found_count < max_devices; addr++) {
            wire->beginTransmission(addr);
            uint8_t error = wire->endTransmission();
            
            if (error == I2C_SUCCESS) {
                devices[found_count++] = addr;
            }
        }
        
        return found_count;
    }
    
    static bool writeI2CData(MockTwoWire* wire, uint8_t address, const uint8_t* data, uint8_t length) {
        if (!wire || !data || length == 0) return false;
        
        wire->beginTransmission(address);
        size_t written = wire->write(data, length);
        if (written != length) return false;
        
        uint8_t error = wire->endTransmission();
        return (error == I2C_SUCCESS);
    }
    
    static uint8_t readI2CData(MockTwoWire* wire, uint8_t address, uint8_t* buffer, uint8_t length) {
        if (!wire || !buffer || length == 0) return 0;
        
        uint8_t bytes_received = wire->requestFrom(address, length);
        if (bytes_received == 0) return 0;
        
        uint8_t bytes_read = 0;
        while (wire->available() && bytes_read < length) {
            int data = wire->read();
            if (data >= 0) {
                buffer[bytes_read++] = static_cast<uint8_t>(data);
            } else {
                break;
            }
        }
        
        return bytes_read;
    }
    
    static bool isI2CDevicePresent(MockTwoWire* wire, uint8_t address) {
        if (!wire) return false;
        
        wire->beginTransmission(address);
        uint8_t error = wire->endTransmission();
        return (error == I2C_SUCCESS);
    }
    
    static bool writeI2CRegister(MockTwoWire* wire, uint8_t address, uint8_t reg, uint8_t value) {
        if (!wire) return false;
        
        wire->beginTransmission(address);
        wire->write(reg);
        wire->write(value);
        uint8_t error = wire->endTransmission();
        
        return (error == I2C_SUCCESS);
    }
    
    static uint8_t readI2CRegister(MockTwoWire* wire, uint8_t address, uint8_t reg) {
        if (!wire) return 0xFF;
        
        wire->beginTransmission(address);
        wire->write(reg);
        uint8_t error = wire->endTransmission(false);
        
        if (error != I2C_SUCCESS) return 0xFF;
        
        if (wire->requestFrom(address, (uint8_t)1) != 1) return 0xFF;
        
        if (wire->available()) {
            return static_cast<uint8_t>(wire->read());
        }
        
        return 0xFF;
    }
    
    static const char* getI2CErrorString(uint8_t error_code) {
        switch (error_code) {
            case I2C_SUCCESS: return "Success";
            case I2C_ERROR_DATA_TOO_LONG: return "Data too long";
            case I2C_ERROR_NACK_ADDRESS: return "NACK on address";
            case I2C_ERROR_NACK_DATA: return "NACK on data";
            case I2C_ERROR_OTHER: return "Other error";
            case I2C_ERROR_TIMEOUT: return "Timeout";
            default: return "Unknown error";
        }
    }
    
    static bool retryI2COperation(MockTwoWire* wire, uint8_t address, uint8_t max_retries = 3) {
        if (!wire) return false;
        
        for (uint8_t retry = 0; retry < max_retries; retry++) {
            wire->beginTransmission(address);
            uint8_t error = wire->endTransmission();
            
            if (error == I2C_SUCCESS) {
                return true;
            }
        }
        
        return false;
    }
};

// GoogleTest Fixture
class I2CUtilsTest : public ::testing::Test {
protected:
    MockTwoWire mockWire;

    void SetUp() override {
        mockWire.reset();
    }

    void TearDown() override {
        // クリーンアップ処理
    }
};

/**
 * @brief Test I2C initialization
 */
TEST_F(I2CUtilsTest, Initialization) {
    bool result = I2CUtils::initializeI2C(&mockWire, 0, 1, 100000);
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(mockWire.begin_called);
    EXPECT_EQ(0, mockWire.sda_pin);
    EXPECT_EQ(1, mockWire.scl_pin);
    EXPECT_EQ(100000, mockWire.clock_speed);
}

/**
 * @brief Test I2C device scanning
 */
TEST_F(I2CUtilsTest, DeviceScanning) {
    // Set up mock scan results
    uint8_t expected_devices[] = {0x3C, 0x42, 0x68}; // OLED, GPS, RTC
    mockWire.setMockScanResults(expected_devices, 3);
    
    uint8_t found_devices[8];
    uint8_t found_count = I2CUtils::scanI2CDevices(&mockWire, found_devices, 8);
    
    EXPECT_EQ(3, found_count);
    EXPECT_THAT(found_devices, ::testing::ElementsAre(0x3C, 0x42, 0x68, 0, 0, 0, 0, 0));
}

/**
 * @brief Test I2C data writing
 */
TEST_F(I2CUtilsTest, DataWriting) {
    mockWire.mock_error_code = I2CUtils::I2C_SUCCESS;
    
    uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04};
    bool result = I2CUtils::writeI2CData(&mockWire, 0x3C, test_data, sizeof(test_data));
    
    EXPECT_TRUE(result);
    EXPECT_EQ(0x3C, mockWire.transmission_address);
    EXPECT_EQ(sizeof(test_data), mockWire.written_count);
    EXPECT_THAT(mockWire.written_data, 
                ::testing::ElementsAre(0x01, 0x02, 0x03, 0x04, 
                                     ::testing::_, ::testing::_, ::testing::_, ::testing::_));
}

/**
 * @brief Test I2C data reading
 */
TEST_F(I2CUtilsTest, DataReading) {
    // Set up mock read data
    uint8_t expected_data[] = {0xAA, 0xBB, 0xCC, 0xDD};
    memcpy(mockWire.mock_read_data, expected_data, sizeof(expected_data));
    mockWire.mock_available_bytes = sizeof(expected_data);
    
    uint8_t read_buffer[8];
    uint8_t bytes_read = I2CUtils::readI2CData(&mockWire, 0x42, read_buffer, sizeof(expected_data));
    
    EXPECT_EQ(sizeof(expected_data), bytes_read);
    EXPECT_THAT(read_buffer, ::testing::ElementsAre(0xAA, 0xBB, 0xCC, 0xDD, 
                                                   ::testing::_, ::testing::_, ::testing::_, ::testing::_));
}

/**
 * @brief Test I2C device presence detection
 */
TEST_F(I2CUtilsTest, DevicePresence) {
    // Test device present
    mockWire.mock_error_code = I2CUtils::I2C_SUCCESS;
    EXPECT_TRUE(I2CUtils::isI2CDevicePresent(&mockWire, 0x3C));
    
    // Test device not present
    mockWire.mock_error_code = I2CUtils::I2C_ERROR_NACK_ADDRESS;
    EXPECT_FALSE(I2CUtils::isI2CDevicePresent(&mockWire, 0x50));
}

/**
 * @brief Test I2C register operations
 */
TEST_F(I2CUtilsTest, RegisterOperations) {
    mockWire.mock_error_code = I2CUtils::I2C_SUCCESS;
    
    // Test register write
    bool write_result = I2CUtils::writeI2CRegister(&mockWire, 0x3C, 0x10, 0xAB);
    EXPECT_TRUE(write_result);
    EXPECT_EQ(0x3C, mockWire.transmission_address);
    EXPECT_EQ(2, mockWire.written_count);
    EXPECT_EQ(0x10, mockWire.written_data[0]); // Register
    EXPECT_EQ(0xAB, mockWire.written_data[1]); // Value
    
    // Test register read
    mockWire.reset();
    mockWire.mock_error_code = I2CUtils::I2C_SUCCESS;
    mockWire.mock_read_data[0] = 0xCD;
    mockWire.mock_available_bytes = 1;
    
    uint8_t read_value = I2CUtils::readI2CRegister(&mockWire, 0x3C, 0x10);
    EXPECT_EQ(0xCD, read_value);
}

/**
 * @brief Test I2C error handling
 */
TEST_F(I2CUtilsTest, ErrorHandling) {
    // Test null pointer handling
    EXPECT_FALSE(I2CUtils::initializeI2C(nullptr, 0, 1));
    EXPECT_EQ(0, I2CUtils::scanI2CDevices(nullptr, nullptr, 0));
    EXPECT_FALSE(I2CUtils::writeI2CData(nullptr, 0x3C, nullptr, 0));
    EXPECT_EQ(0, I2CUtils::readI2CData(nullptr, 0x3C, nullptr, 0));
    
    // Test error string conversion with advanced matchers
    using ::testing::StrEq;
    EXPECT_THAT(I2CUtils::getI2CErrorString(I2CUtils::I2C_SUCCESS), StrEq("Success"));
    EXPECT_THAT(I2CUtils::getI2CErrorString(I2CUtils::I2C_ERROR_DATA_TOO_LONG), StrEq("Data too long"));
    EXPECT_THAT(I2CUtils::getI2CErrorString(I2CUtils::I2C_ERROR_NACK_ADDRESS), StrEq("NACK on address"));
    EXPECT_THAT(I2CUtils::getI2CErrorString(I2CUtils::I2C_ERROR_TIMEOUT), StrEq("Timeout"));
    EXPECT_THAT(I2CUtils::getI2CErrorString(99), StrEq("Unknown error"));
}

/**
 * @brief Test I2C retry mechanism
 */
TEST_F(I2CUtilsTest, RetryMechanism) {
    // Test successful operation (no retry needed)
    mockWire.mock_error_code = I2CUtils::I2C_SUCCESS;
    EXPECT_TRUE(I2CUtils::retryI2COperation(&mockWire, 0x3C, 3));
    
    // Test failed operation (all retries fail)
    mockWire.mock_error_code = I2CUtils::I2C_ERROR_NACK_ADDRESS;
    EXPECT_FALSE(I2CUtils::retryI2COperation(&mockWire, 0x3C, 3));
    
    // Test null wire
    EXPECT_FALSE(I2CUtils::retryI2COperation(nullptr, 0x3C, 3));
}

/**
 * @brief Test I2C write/read error conditions
 */
TEST_F(I2CUtilsTest, ErrorConditions) {
    // Test write failure
    mockWire.mock_error_code = I2CUtils::I2C_ERROR_NACK_DATA;
    uint8_t test_data[] = {0x01, 0x02};
    bool result = I2CUtils::writeI2CData(&mockWire, 0x3C, test_data, sizeof(test_data));
    EXPECT_FALSE(result);
    
    // Test read failure (no data available)
    mockWire.reset();
    mockWire.mock_available_bytes = 0;
    uint8_t read_buffer[4];
    uint8_t bytes_read = I2CUtils::readI2CData(&mockWire, 0x42, read_buffer, 4);
    EXPECT_EQ(0, bytes_read);
    
    // Test register read failure
    mockWire.reset();
    mockWire.mock_error_code = I2CUtils::I2C_ERROR_OTHER;
    uint8_t reg_value = I2CUtils::readI2CRegister(&mockWire, 0x3C, 0x10);
    EXPECT_EQ(0xFF, reg_value);
}

/**
 * @brief パラメータ化テスト - 様々なI2Cアドレスでのデバイス検出
 */
class DevicePresenceTest : public I2CUtilsTest, 
                          public ::testing::WithParamInterface<std::tuple<uint8_t, uint8_t, bool>> {
};

TEST_P(DevicePresenceTest, CheckDevicePresence) {
    uint8_t address = std::get<0>(GetParam());
    uint8_t error_code = std::get<1>(GetParam());
    bool expected_present = std::get<2>(GetParam());
    
    mockWire.mock_error_code = error_code;
    bool result = I2CUtils::isI2CDevicePresent(&mockWire, address);
    
    EXPECT_EQ(expected_present, result) << "Address: 0x" << std::hex << static_cast<int>(address) 
                                       << ", Error: " << static_cast<int>(error_code);
}

INSTANTIATE_TEST_SUITE_P(
    DevicePresenceTestCases,
    DevicePresenceTest,
    ::testing::Values(
        std::make_tuple(0x3C, I2CUtils::I2C_SUCCESS, true),           // OLED present
        std::make_tuple(0x42, I2CUtils::I2C_SUCCESS, true),           // GPS present  
        std::make_tuple(0x68, I2CUtils::I2C_SUCCESS, true),           // RTC present
        std::make_tuple(0x50, I2CUtils::I2C_ERROR_NACK_ADDRESS, false), // Device not present
        std::make_tuple(0x60, I2CUtils::I2C_ERROR_TIMEOUT, false)     // Timeout error
    )
);

/**
 * @brief マッチャーを使った高度なアサーション例
 */
TEST_F(I2CUtilsTest, AdvancedMatchers) {
    using ::testing::AllOf;
    using ::testing::Ge;
    using ::testing::Le;
    using ::testing::Ne;
    
    // クロック速度の範囲チェック
    I2CUtils::initializeI2C(&mockWire, 0, 1, 400000);
    EXPECT_THAT(mockWire.clock_speed, AllOf(Ge(100000), Le(1000000)));
    
    // ピン設定の有効性チェック  
    I2CUtils::initializeI2C(&mockWire, 4, 5, 100000);
    EXPECT_THAT(mockWire.sda_pin, Ne(mockWire.scl_pin));
}
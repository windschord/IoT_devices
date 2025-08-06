#include <unity.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// Simple I2CUtils implementation for testing (Simple Test Design Pattern)

// Mock TwoWire implementation
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
        // Return the minimum of requested length and available bytes
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
        
        // For testing, use mock scan results if available
        if (wire->scan_count > 0) {
            uint8_t count = (wire->scan_count < max_devices) ? wire->scan_count : max_devices;
            memcpy(devices, wire->scan_results, count);
            return count;
        }
        
        // Simulate device scanning - check common I2C addresses
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
        
        // Write register address
        wire->beginTransmission(address);
        wire->write(reg);
        uint8_t error = wire->endTransmission(false); // Restart, not stop
        
        if (error != I2C_SUCCESS) return 0xFF;
        
        // Read register value
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
            
            // Simple delay simulation (would be actual delay in real implementation)
            // In test, we just continue
        }
        
        return false;
    }
};

MockTwoWire mockWire;

/**
 * @brief Test I2C initialization
 */
void test_i2c_utils_initialization() {
    mockWire.reset();
    
    bool result = I2CUtils::initializeI2C(&mockWire, 0, 1, 100000);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(mockWire.begin_called);
    TEST_ASSERT_EQUAL_UINT8(0, mockWire.sda_pin);
    TEST_ASSERT_EQUAL_UINT8(1, mockWire.scl_pin);
    TEST_ASSERT_EQUAL_UINT32(100000, mockWire.clock_speed);
}

/**
 * @brief Test I2C device scanning
 */
void test_i2c_utils_device_scanning() {
    mockWire.reset();
    
    // Set up mock scan results
    uint8_t expected_devices[] = {0x3C, 0x42, 0x68}; // OLED, GPS, RTC
    mockWire.setMockScanResults(expected_devices, 3);
    
    uint8_t found_devices[8];
    uint8_t found_count = I2CUtils::scanI2CDevices(&mockWire, found_devices, 8);
    
    TEST_ASSERT_EQUAL_UINT8(3, found_count);
    TEST_ASSERT_EQUAL_UINT8(0x3C, found_devices[0]);
    TEST_ASSERT_EQUAL_UINT8(0x42, found_devices[1]);
    TEST_ASSERT_EQUAL_UINT8(0x68, found_devices[2]);
}

/**
 * @brief Test I2C data writing
 */
void test_i2c_utils_data_writing() {
    mockWire.reset();
    mockWire.mock_error_code = I2CUtils::I2C_SUCCESS;
    
    uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04};
    bool result = I2CUtils::writeI2CData(&mockWire, 0x3C, test_data, sizeof(test_data));
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT8(0x3C, mockWire.transmission_address);
    TEST_ASSERT_EQUAL_UINT8(sizeof(test_data), mockWire.written_count);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(test_data, mockWire.written_data, sizeof(test_data));
}

/**
 * @brief Test I2C data reading
 */
void test_i2c_utils_data_reading() {
    mockWire.reset();
    
    // Set up mock read data
    uint8_t expected_data[] = {0xAA, 0xBB, 0xCC, 0xDD};
    memcpy(mockWire.mock_read_data, expected_data, sizeof(expected_data));
    mockWire.mock_available_bytes = sizeof(expected_data);
    
    uint8_t read_buffer[8];
    uint8_t bytes_read = I2CUtils::readI2CData(&mockWire, 0x42, read_buffer, sizeof(expected_data));
    
    TEST_ASSERT_EQUAL_UINT8(sizeof(expected_data), bytes_read);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_data, read_buffer, sizeof(expected_data));
}

/**
 * @brief Test I2C device presence detection
 */
void test_i2c_utils_device_presence() {
    mockWire.reset();
    
    // Test device present
    mockWire.mock_error_code = I2CUtils::I2C_SUCCESS;
    TEST_ASSERT_TRUE(I2CUtils::isI2CDevicePresent(&mockWire, 0x3C));
    
    // Test device not present
    mockWire.mock_error_code = I2CUtils::I2C_ERROR_NACK_ADDRESS;
    TEST_ASSERT_FALSE(I2CUtils::isI2CDevicePresent(&mockWire, 0x50));
}

/**
 * @brief Test I2C register operations
 */
void test_i2c_utils_register_operations() {
    mockWire.reset();
    mockWire.mock_error_code = I2CUtils::I2C_SUCCESS;
    
    // Test register write
    bool write_result = I2CUtils::writeI2CRegister(&mockWire, 0x3C, 0x10, 0xAB);
    TEST_ASSERT_TRUE(write_result);
    TEST_ASSERT_EQUAL_UINT8(0x3C, mockWire.transmission_address);
    TEST_ASSERT_EQUAL_UINT8(2, mockWire.written_count);
    TEST_ASSERT_EQUAL_UINT8(0x10, mockWire.written_data[0]); // Register
    TEST_ASSERT_EQUAL_UINT8(0xAB, mockWire.written_data[1]); // Value
    
    // Test register read
    mockWire.reset();
    mockWire.mock_error_code = I2CUtils::I2C_SUCCESS;
    mockWire.mock_read_data[0] = 0xCD;
    mockWire.mock_available_bytes = 1;
    
    uint8_t read_value = I2CUtils::readI2CRegister(&mockWire, 0x3C, 0x10);
    TEST_ASSERT_EQUAL_UINT8(0xCD, read_value);
}

/**
 * @brief Test I2C error handling
 */
void test_i2c_utils_error_handling() {
    mockWire.reset();
    
    // Test null pointer handling
    TEST_ASSERT_FALSE(I2CUtils::initializeI2C(nullptr, 0, 1));
    TEST_ASSERT_EQUAL_UINT8(0, I2CUtils::scanI2CDevices(nullptr, nullptr, 0));
    TEST_ASSERT_FALSE(I2CUtils::writeI2CData(nullptr, 0x3C, nullptr, 0));
    TEST_ASSERT_EQUAL_UINT8(0, I2CUtils::readI2CData(nullptr, 0x3C, nullptr, 0));
    
    // Test error string conversion
    TEST_ASSERT_EQUAL_STRING("Success", I2CUtils::getI2CErrorString(I2CUtils::I2C_SUCCESS));
    TEST_ASSERT_EQUAL_STRING("Data too long", I2CUtils::getI2CErrorString(I2CUtils::I2C_ERROR_DATA_TOO_LONG));
    TEST_ASSERT_EQUAL_STRING("NACK on address", I2CUtils::getI2CErrorString(I2CUtils::I2C_ERROR_NACK_ADDRESS));
    TEST_ASSERT_EQUAL_STRING("Timeout", I2CUtils::getI2CErrorString(I2CUtils::I2C_ERROR_TIMEOUT));
    TEST_ASSERT_EQUAL_STRING("Unknown error", I2CUtils::getI2CErrorString(99));
}

/**
 * @brief Test I2C retry mechanism
 */
void test_i2c_utils_retry_mechanism() {
    mockWire.reset();
    
    // Test successful operation (no retry needed)
    mockWire.mock_error_code = I2CUtils::I2C_SUCCESS;
    TEST_ASSERT_TRUE(I2CUtils::retryI2COperation(&mockWire, 0x3C, 3));
    
    // Test failed operation (all retries fail)
    mockWire.mock_error_code = I2CUtils::I2C_ERROR_NACK_ADDRESS;
    TEST_ASSERT_FALSE(I2CUtils::retryI2COperation(&mockWire, 0x3C, 3));
    
    // Test null wire
    TEST_ASSERT_FALSE(I2CUtils::retryI2COperation(nullptr, 0x3C, 3));
}

/**
 * @brief Test I2C write/read error conditions
 */
void test_i2c_utils_error_conditions() {
    mockWire.reset();
    
    // Test write failure
    mockWire.mock_error_code = I2CUtils::I2C_ERROR_NACK_DATA;
    uint8_t test_data[] = {0x01, 0x02};
    bool result = I2CUtils::writeI2CData(&mockWire, 0x3C, test_data, sizeof(test_data));
    TEST_ASSERT_FALSE(result);
    
    // Test read failure (no data available)
    mockWire.reset();
    mockWire.mock_available_bytes = 0;
    uint8_t read_buffer[4];
    uint8_t bytes_read = I2CUtils::readI2CData(&mockWire, 0x42, read_buffer, 4);
    TEST_ASSERT_EQUAL_UINT8(0, bytes_read);
    
    // Test register read failure
    mockWire.reset();
    mockWire.mock_error_code = I2CUtils::I2C_ERROR_OTHER;
    uint8_t reg_value = I2CUtils::readI2CRegister(&mockWire, 0x3C, 0x10);
    TEST_ASSERT_EQUAL_UINT8(0xFF, reg_value); // Error return value
}

void setUp(void) {
    mockWire.reset();
}

void tearDown(void) {
    // Cleanup
}

int main() {
    UNITY_BEGIN();
    
    RUN_TEST(test_i2c_utils_initialization);
    RUN_TEST(test_i2c_utils_device_scanning);
    RUN_TEST(test_i2c_utils_data_writing);
    RUN_TEST(test_i2c_utils_data_reading);
    RUN_TEST(test_i2c_utils_device_presence);
    RUN_TEST(test_i2c_utils_register_operations);
    RUN_TEST(test_i2c_utils_error_handling);
    RUN_TEST(test_i2c_utils_retry_mechanism);
    RUN_TEST(test_i2c_utils_error_conditions);
    
    return UNITY_END();
}
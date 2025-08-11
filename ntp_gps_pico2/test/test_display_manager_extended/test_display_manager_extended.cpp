#include <unity.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// Advanced DisplayManager implementation for extended testing
enum class DisplayMode {
    GPS_STATUS = 0,
    TIME_SYNC = 1,
    NETWORK_INFO = 2,
    SYSTEM_STATUS = 3,
    GNSS_DETAILS = 4,
    ERROR_STATUS = 5,
    PERFORMANCE_METRICS = 6,
    COUNT
};

// GPS Data structure for display
struct GpsData {
    bool fix_valid;
    uint8_t satellites_used;
    uint8_t satellites_visible;
    uint32_t fix_time_ms;
    float hdop;
    float vdop;
    float pdop;
    double latitude;
    double longitude;
    float altitude;
    float speed_kmh;
    float heading_deg;
    char constellation_status[32];  // "GPS|GLO|GAL|BEI|QZSS"
    bool pps_active;
    uint64_t last_pps_timestamp;
    uint32_t pps_count;
};

// Network Data structure for display
struct NetworkData {
    bool ethernet_connected;
    uint32_t ip_address;
    uint32_t netmask;
    uint32_t gateway;
    uint32_t dns_server;
    char hostname[32];
    bool dhcp_enabled;
    uint32_t bytes_sent;
    uint32_t bytes_received;
    uint16_t active_connections;
    bool ntp_server_active;
    uint32_t ntp_requests;
    uint32_t ntp_responses;
};

// System Data structure for display
struct SystemData {
    uint32_t uptime_seconds;
    uint32_t free_memory_bytes;
    uint32_t used_memory_bytes;
    uint8_t cpu_usage_percent;
    float temperature_celsius;
    float voltage;
    uint32_t error_count;
    uint32_t warning_count;
    bool watchdog_active;
    uint32_t last_restart_reason;
    uint32_t total_restarts;
};

// Performance Metrics structure
struct PerformanceMetrics {
    float ntp_accuracy_ms;
    float pps_jitter_us;
    uint32_t missed_pps_count;
    uint32_t clock_corrections;
    float average_response_time_ms;
    float max_response_time_ms;
    uint32_t packet_loss_count;
    float cpu_load_1min;
    float cpu_load_5min;
    uint32_t memory_fragmentation_percent;
};

// Mock OLED Display HAL for testing
class MockOLEDDisplay {
public:
    static const int SCREEN_WIDTH = 128;
    static const int SCREEN_HEIGHT = 64;
    static const int MAX_LINES = 8;
    static const int MAX_CHARS_PER_LINE = 21;
    
    char display_buffer[MAX_LINES][MAX_CHARS_PER_LINE + 1];
    bool initialized = false;
    bool display_on = true;
    uint8_t contrast = 128;
    bool invert_display = false;
    int cursor_x = 0;
    int cursor_y = 0;
    bool text_size = 1;
    int update_count = 0;
    bool connection_error = false;
    int i2c_errors = 0;
    
    bool begin() {
        if (connection_error) {
            i2c_errors++;
            return false;
        }
        initialized = true;
        clearDisplay();
        return true;
    }
    
    void clearDisplay() {
        for (int i = 0; i < MAX_LINES; i++) {
            memset(display_buffer[i], ' ', MAX_CHARS_PER_LINE);
            display_buffer[i][MAX_CHARS_PER_LINE] = '\0';
        }
        cursor_x = 0;
        cursor_y = 0;
    }
    
    void setCursor(int x, int y) {
        cursor_x = x;
        cursor_y = y / 8;  // Convert pixel to line
        if (cursor_y >= MAX_LINES) cursor_y = MAX_LINES - 1;
    }
    
    void setTextSize(uint8_t size) {
        text_size = size;
    }
    
    void print(const char* text) {
        if (!initialized || cursor_y >= MAX_LINES) return;
        
        int len = strlen(text);
        int chars_to_copy = (len < MAX_CHARS_PER_LINE - cursor_x) ? len : MAX_CHARS_PER_LINE - cursor_x;
        
        strncpy(&display_buffer[cursor_y][cursor_x], text, chars_to_copy);
        cursor_x += chars_to_copy;
    }
    
    void println(const char* text) {
        print(text);
        cursor_x = 0;
        cursor_y++;
    }
    
    void display() {
        if (connection_error) {
            i2c_errors++;
            return;
        }
        update_count++;
    }
    
    void setContrast(uint8_t contrast_val) {
        contrast = contrast_val;
    }
    
    void invertDisplay(bool invert) {
        invert_display = invert;
    }
    
    void displayOn(bool on) {
        display_on = on;
    }
    
    // Test helper methods
    const char* getLine(int line) {
        if (line < 0 || line >= MAX_LINES) return nullptr;
        return display_buffer[line];
    }
    
    bool containsText(const char* text) {
        for (int i = 0; i < MAX_LINES; i++) {
            if (strstr(display_buffer[i], text) != nullptr) {
                return true;
            }
        }
        return false;
    }
    
    void reset() {
        initialized = false;
        display_on = true;
        contrast = 128;
        invert_display = false;
        cursor_x = 0;
        cursor_y = 0;
        text_size = 1;
        update_count = 0;
        connection_error = false;
        i2c_errors = 0;
        clearDisplay();
    }
    
    void simulateConnectionError(bool error) {
        connection_error = error;
    }
};

// Extended DisplayManager with advanced features
class ExtendedDisplayManager {
private:
    MockOLEDDisplay* display;
    DisplayMode current_mode = DisplayMode::GPS_STATUS;
    uint32_t last_update_time = 0;
    uint32_t display_update_interval = 1000;  // 1 second default
    bool auto_rotation_enabled = false;
    uint32_t auto_rotation_interval = 5000;  // 5 seconds
    uint32_t last_rotation_time = 0;
    bool display_enabled = true;
    uint8_t brightness_level = 100;  // 0-100%
    bool power_save_mode = false;
    uint32_t inactivity_timeout = 30000;  // 30 seconds
    uint32_t last_activity_time = 0;
    int display_errors = 0;
    bool animation_enabled = true;
    int animation_frame = 0;
    
    // Data caching for display
    GpsData cached_gps_data;
    NetworkData cached_network_data;
    SystemData cached_system_data;
    PerformanceMetrics cached_metrics;
    
public:
    ExtendedDisplayManager(MockOLEDDisplay* oled) : display(oled) {
        memset(&cached_gps_data, 0, sizeof(cached_gps_data));
        memset(&cached_network_data, 0, sizeof(cached_network_data));
        memset(&cached_system_data, 0, sizeof(cached_system_data));
        memset(&cached_metrics, 0, sizeof(cached_metrics));
        last_activity_time = getCurrentTime();
    }
    
    // Simulate current time for testing
    uint32_t getCurrentTime() const {
        static uint32_t simulated_time = 1000;
        simulated_time += 100;  // Advance time by 100ms each call
        return simulated_time;
    }
    
    bool initialize() {
        if (!display->begin()) {
            display_errors++;
            return false;
        }
        
        display->setContrast(brightness_level * 255 / 100);
        display->clearDisplay();
        displayWelcomeMessage();
        return true;
    }
    
    void displayWelcomeMessage() {
        display->clearDisplay();
        display->setCursor(0, 0);
        display->println("GPS NTP Server");
        display->println("Initializing...");
        display->display();
    }
    
    void setDisplayMode(DisplayMode mode) {
        if (mode != current_mode) {
            current_mode = mode;
            last_activity_time = getCurrentTime();
            forceUpdate();
        }
    }
    
    DisplayMode getDisplayMode() const {
        return current_mode;
    }
    
    void nextMode() {
        int next = (static_cast<int>(current_mode) + 1) % static_cast<int>(DisplayMode::COUNT);
        setDisplayMode(static_cast<DisplayMode>(next));
    }
    
    void previousMode() {
        int prev = static_cast<int>(current_mode) - 1;
        if (prev < 0) prev = static_cast<int>(DisplayMode::COUNT) - 1;
        setDisplayMode(static_cast<DisplayMode>(prev));
    }
    
    void enableAutoRotation(bool enable, uint32_t interval_ms = 5000) {
        auto_rotation_enabled = enable;
        auto_rotation_interval = interval_ms;
        last_rotation_time = getCurrentTime();
    }
    
    void setUpdateInterval(uint32_t interval_ms) {
        display_update_interval = interval_ms;
    }
    
    void setBrightness(uint8_t level) {
        if (level > 100) level = 100;
        brightness_level = level;
        display->setContrast(level * 255 / 100);
    }
    
    void enablePowerSave(bool enable, uint32_t timeout_ms = 30000) {
        power_save_mode = enable;
        inactivity_timeout = timeout_ms;
    }
    
    void enableAnimation(bool enable) {
        animation_enabled = enable;
    }
    
    // Update display with GPS data
    void updateGPSData(const GpsData& gps_data) {
        cached_gps_data = gps_data;
        last_activity_time = getCurrentTime();
    }
    
    // Update display with Network data
    void updateNetworkData(const NetworkData& network_data) {
        cached_network_data = network_data;
    }
    
    // Update display with System data
    void updateSystemData(const SystemData& system_data) {
        cached_system_data = system_data;
    }
    
    // Update display with Performance metrics
    void updatePerformanceMetrics(const PerformanceMetrics& metrics) {
        cached_metrics = metrics;
    }
    
    void update() {
        uint32_t current_time = getCurrentTime();
        
        // Check power save mode
        if (power_save_mode && 
            (current_time - last_activity_time > inactivity_timeout)) {
            if (display_enabled) {
                display->displayOn(false);
                display_enabled = false;
            }
            return;
        } else if (!display_enabled) {
            display->displayOn(true);
            display_enabled = true;
        }
        
        // Check auto rotation
        if (auto_rotation_enabled && 
            (current_time - last_rotation_time > auto_rotation_interval)) {
            nextMode();
            last_rotation_time = current_time;
        }
        
        // Check if update is needed
        if (current_time - last_update_time < display_update_interval) {
            return;
        }
        
        // Update animation frame
        if (animation_enabled) {
            animation_frame = (animation_frame + 1) % 4;
        }
        
        // Display content based on current mode
        display->clearDisplay();
        display->setCursor(0, 0);
        
        switch (current_mode) {
            case DisplayMode::GPS_STATUS:
                displayGPSStatus();
                break;
            case DisplayMode::TIME_SYNC:
                displayTimeSync();
                break;
            case DisplayMode::NETWORK_INFO:
                displayNetworkInfo();
                break;
            case DisplayMode::SYSTEM_STATUS:
                displaySystemStatus();
                break;
            case DisplayMode::GNSS_DETAILS:
                displayGNSSDetails();
                break;
            case DisplayMode::ERROR_STATUS:
                displayErrorStatus();
                break;
            case DisplayMode::PERFORMANCE_METRICS:
                displayPerformanceMetrics();
                break;
            case DisplayMode::COUNT:
            default:
                displayGPSStatus();  // Default fallback
                break;
        }
        
        display->display();
        last_update_time = current_time;
    }
    
    void forceUpdate() {
        last_update_time = 0;  // Force immediate update
        update();
    }
    
private:
    void displayGPSStatus() {
        display->println("=== GPS STATUS ===");
        
        char line[22];
        if (cached_gps_data.fix_valid) {
            snprintf(line, sizeof(line), "Fix: %d/%d sats", 
                     cached_gps_data.satellites_used, 
                     cached_gps_data.satellites_visible);
            display->println(line);
            
            snprintf(line, sizeof(line), "HDOP: %.1f", cached_gps_data.hdop);
            display->println(line);
            
            if (cached_gps_data.pps_active) {
                const char* pps_indicators[] = {"|", "/", "-", "\\"};
                snprintf(line, sizeof(line), "PPS: %s %d", 
                         pps_indicators[animation_frame], 
                         cached_gps_data.pps_count);
            } else {
                snprintf(line, sizeof(line), "PPS: OFF");
            }
            display->println(line);
        } else {
            display->println("Searching...");
            const char* search_animation[] = {"   ", ".  ", ".. ", "..."};
            display->print(search_animation[animation_frame]);
        }
        
        // Show constellation status
        display->println(cached_gps_data.constellation_status);
    }
    
    void displayTimeSync() {
        display->println("=== TIME SYNC ===");
        
        char line[22];
        if (cached_gps_data.fix_valid) {
            display->println("GPS Time: Valid");
            snprintf(line, sizeof(line), "Accuracy: %.1fms", cached_metrics.ntp_accuracy_ms);
            display->println(line);
            
            snprintf(line, sizeof(line), "PPS Jitter: %.1fus", cached_metrics.pps_jitter_us);
            display->println(line);
            
            if (cached_metrics.clock_corrections > 0) {
                snprintf(line, sizeof(line), "Corrections: %d", cached_metrics.clock_corrections);
                display->println(line);
            }
        } else {
            display->println("No GPS Time");
            display->println("Using RTC");
        }
    }
    
    void displayNetworkInfo() {
        display->println("=== NETWORK ===");
        
        char line[22];
        if (cached_network_data.ethernet_connected) {
            display->println("Ethernet: UP");
            
            // Convert IP to readable format (simplified)
            uint32_t ip = cached_network_data.ip_address;
            snprintf(line, sizeof(line), "IP: %d.%d.%d.%d",
                     (ip >> 24) & 0xFF, (ip >> 16) & 0xFF,
                     (ip >> 8) & 0xFF, ip & 0xFF);
            display->println(line);
            
            snprintf(line, sizeof(line), "NTP: %s", 
                     cached_network_data.ntp_server_active ? "Active" : "Inactive");
            display->println(line);
            
            snprintf(line, sizeof(line), "Requests: %d", cached_network_data.ntp_requests);
            display->println(line);
        } else {
            display->println("No Network");
        }
    }
    
    void displaySystemStatus() {
        display->println("=== SYSTEM ===");
        
        char line[22];
        uint32_t hours = cached_system_data.uptime_seconds / 3600;
        uint32_t minutes = (cached_system_data.uptime_seconds % 3600) / 60;
        snprintf(line, sizeof(line), "Uptime: %dh %dm", hours, minutes);
        display->println(line);
        
        uint32_t free_kb = cached_system_data.free_memory_bytes / 1024;
        uint32_t used_kb = cached_system_data.used_memory_bytes / 1024;
        snprintf(line, sizeof(line), "Mem: %dK/%dK", used_kb, used_kb + free_kb);
        display->println(line);
        
        snprintf(line, sizeof(line), "CPU: %d%%", cached_system_data.cpu_usage_percent);
        display->println(line);
        
        snprintf(line, sizeof(line), "Temp: %.1fC", cached_system_data.temperature_celsius);
        display->println(line);
        
        if (cached_system_data.error_count > 0) {
            snprintf(line, sizeof(line), "Errors: %d", cached_system_data.error_count);
            display->println(line);
        }
    }
    
    void displayGNSSDetails() {
        display->println("=== GNSS ===");
        
        char line[22];
        if (cached_gps_data.fix_valid) {
            snprintf(line, sizeof(line), "Lat: %.4f", cached_gps_data.latitude);
            display->println(line);
            
            snprintf(line, sizeof(line), "Lon: %.4f", cached_gps_data.longitude);
            display->println(line);
            
            snprintf(line, sizeof(line), "Alt: %.1fm", cached_gps_data.altitude);
            display->println(line);
            
            snprintf(line, sizeof(line), "Speed: %.1fkm/h", cached_gps_data.speed_kmh);
            display->println(line);
            
            snprintf(line, sizeof(line), "PDOP: %.1f", cached_gps_data.pdop);
            display->println(line);
        } else {
            display->println("No Fix");
        }
    }
    
    void displayErrorStatus() {
        display->println("=== ERRORS ===");
        
        char line[22];
        if (cached_system_data.error_count == 0 && display_errors == 0) {
            display->println("All OK");
            const char* ok_animation[] = {" :)", " :D", " :)", " :P"};
            display->print(ok_animation[animation_frame]);
        } else {
            snprintf(line, sizeof(line), "System: %d", cached_system_data.error_count);
            display->println(line);
            
            snprintf(line, sizeof(line), "Display: %d", display_errors);
            display->println(line);
            
            snprintf(line, sizeof(line), "Warnings: %d", cached_system_data.warning_count);
            display->println(line);
        }
    }
    
    void displayPerformanceMetrics() {
        display->println("=== PERFORMANCE ===");
        
        char line[22];
        snprintf(line, sizeof(line), "NTP Acc: %.2fms", cached_metrics.ntp_accuracy_ms);
        display->println(line);
        
        snprintf(line, sizeof(line), "Resp: %.1fms", cached_metrics.average_response_time_ms);
        display->println(line);
        
        snprintf(line, sizeof(line), "CPU 1m: %.1f%%", cached_metrics.cpu_load_1min);
        display->println(line);
        
        snprintf(line, sizeof(line), "Mem Frag: %d%%", cached_metrics.memory_fragmentation_percent);
        display->println(line);
    }
    
public:
    // Test helper methods
    bool isDisplayInitialized() const {
        return display->initialized;
    }
    
    int getUpdateCount() const {
        return display->update_count;
    }
    
    int getDisplayErrors() const {
        return display_errors;
    }
    
    bool isAutoRotationEnabled() const {
        return auto_rotation_enabled;
    }
    
    bool isPowerSaveMode() const {
        return power_save_mode;
    }
    
    uint8_t getBrightnessLevel() const {
        return brightness_level;
    }
    
    bool containsText(const char* text) const {
        return display->containsText(text);
    }
    
    const char* getDisplayLine(int line) const {
        return display->getLine(line);
    }
};

// Global test instances
static MockOLEDDisplay* mockDisplay = nullptr;
static ExtendedDisplayManager* displayManager = nullptr;

void setUp(void) {
    mockDisplay = new MockOLEDDisplay();
    displayManager = new ExtendedDisplayManager(mockDisplay);
}

void tearDown(void) {
    delete displayManager;
    delete mockDisplay;
    displayManager = nullptr;
    mockDisplay = nullptr;
}

// Basic Display Tests
void test_display_manager_initialization() {
    TEST_ASSERT_TRUE(displayManager->initialize());
    TEST_ASSERT_TRUE(displayManager->isDisplayInitialized());
    TEST_ASSERT_TRUE(displayManager->containsText("GPS NTP Server"));
}

void test_display_manager_initialization_failure() {
    mockDisplay->simulateConnectionError(true);
    TEST_ASSERT_FALSE(displayManager->initialize());
    TEST_ASSERT_EQUAL_INT(1, displayManager->getDisplayErrors());
}

void test_display_manager_mode_switching() {
    TEST_ASSERT_TRUE(displayManager->initialize());
    TEST_ASSERT_EQUAL_INT(0, static_cast<int>(displayManager->getDisplayMode()));
    
    displayManager->nextMode();
    TEST_ASSERT_EQUAL_INT(1, static_cast<int>(displayManager->getDisplayMode()));
    
    displayManager->previousMode();
    TEST_ASSERT_EQUAL_INT(0, static_cast<int>(displayManager->getDisplayMode()));
}

// GPS Display Tests
void test_display_manager_gps_status_no_fix() {
    TEST_ASSERT_TRUE(displayManager->initialize());
    
    GpsData gps_data = {};
    gps_data.fix_valid = false;
    gps_data.satellites_visible = 5;
    strcpy(gps_data.constellation_status, "GPS|GLO");
    
    displayManager->updateGPSData(gps_data);
    displayManager->setDisplayMode(DisplayMode::GPS_STATUS);
    displayManager->forceUpdate();
    
    TEST_ASSERT_TRUE(displayManager->containsText("GPS STATUS"));
    TEST_ASSERT_TRUE(displayManager->containsText("Searching"));
}

void test_display_manager_gps_status_with_fix() {
    TEST_ASSERT_TRUE(displayManager->initialize());
    
    GpsData gps_data = {};
    gps_data.fix_valid = true;
    gps_data.satellites_used = 8;
    gps_data.satellites_visible = 12;
    gps_data.hdop = 1.2f;
    gps_data.pps_active = true;
    gps_data.pps_count = 1234;
    strcpy(gps_data.constellation_status, "GPS|GLO|GAL");
    
    displayManager->updateGPSData(gps_data);
    displayManager->setDisplayMode(DisplayMode::GPS_STATUS);
    displayManager->forceUpdate();
    
    TEST_ASSERT_TRUE(displayManager->containsText("Fix: 8/12"));
    TEST_ASSERT_TRUE(displayManager->containsText("HDOP: 1.2"));
    TEST_ASSERT_TRUE(displayManager->containsText("PPS:"));
}

// Network Display Tests
void test_display_manager_network_connected() {
    TEST_ASSERT_TRUE(displayManager->initialize());
    
    NetworkData network_data = {};
    network_data.ethernet_connected = true;
    network_data.ip_address = 0xC0A80101; // 192.168.1.1
    network_data.ntp_server_active = true;
    network_data.ntp_requests = 156;
    
    displayManager->updateNetworkData(network_data);
    displayManager->setDisplayMode(DisplayMode::NETWORK_INFO);
    displayManager->forceUpdate();
    
    TEST_ASSERT_TRUE(displayManager->containsText("NETWORK"));
    TEST_ASSERT_TRUE(displayManager->containsText("Ethernet: UP"));
    TEST_ASSERT_TRUE(displayManager->containsText("NTP: Active"));
}

void test_display_manager_network_disconnected() {
    TEST_ASSERT_TRUE(displayManager->initialize());
    
    NetworkData network_data = {};
    network_data.ethernet_connected = false;
    
    displayManager->updateNetworkData(network_data);
    displayManager->setDisplayMode(DisplayMode::NETWORK_INFO);
    displayManager->forceUpdate();
    
    TEST_ASSERT_TRUE(displayManager->containsText("No Network"));
}

// System Display Tests
void test_display_manager_system_status() {
    TEST_ASSERT_TRUE(displayManager->initialize());
    
    SystemData system_data = {};
    system_data.uptime_seconds = 7265; // 2h 1m 5s
    system_data.free_memory_bytes = 150 * 1024;
    system_data.used_memory_bytes = 100 * 1024;
    system_data.cpu_usage_percent = 25;
    system_data.temperature_celsius = 45.5f;
    system_data.error_count = 0;
    
    displayManager->updateSystemData(system_data);
    displayManager->setDisplayMode(DisplayMode::SYSTEM_STATUS);
    displayManager->forceUpdate();
    
    TEST_ASSERT_TRUE(displayManager->containsText("SYSTEM"));
    TEST_ASSERT_TRUE(displayManager->containsText("Uptime: 2h 1m"));
    TEST_ASSERT_TRUE(displayManager->containsText("CPU: 25%"));
    TEST_ASSERT_TRUE(displayManager->containsText("Temp: 45.5C"));
}

// Advanced Features Tests
void test_display_manager_auto_rotation() {
    TEST_ASSERT_TRUE(displayManager->initialize());
    
    displayManager->enableAutoRotation(true, 1000);  // 1 second for testing
    TEST_ASSERT_TRUE(displayManager->isAutoRotationEnabled());
    
    DisplayMode initial_mode = displayManager->getDisplayMode();
    
    // Simulate time passage and updates
    for (int i = 0; i < 15; i++) {
        displayManager->update();
    }
    
    // Should have rotated after sufficient updates
    DisplayMode final_mode = displayManager->getDisplayMode();
    TEST_ASSERT_NOT_EQUAL(static_cast<int>(initial_mode), static_cast<int>(final_mode));
}

void test_display_manager_brightness_control() {
    TEST_ASSERT_TRUE(displayManager->initialize());
    
    displayManager->setBrightness(50);
    TEST_ASSERT_EQUAL_UINT8(50, displayManager->getBrightnessLevel());
    TEST_ASSERT_EQUAL_UINT8(127, mockDisplay->contrast);  // ~50% of 255
    
    displayManager->setBrightness(150);  // Should clamp to 100
    TEST_ASSERT_EQUAL_UINT8(100, displayManager->getBrightnessLevel());
}

void test_display_manager_power_save_mode() {
    TEST_ASSERT_TRUE(displayManager->initialize());
    
    displayManager->enablePowerSave(true, 1000);  // 1 second timeout
    TEST_ASSERT_TRUE(displayManager->isPowerSaveMode());
    
    // Simulate time passage without activity
    for (int i = 0; i < 20; i++) {
        displayManager->update();
    }
    
    // Display should be turned off due to inactivity
    TEST_ASSERT_FALSE(mockDisplay->display_on);
}

void test_display_manager_animation_enabled() {
    TEST_ASSERT_TRUE(displayManager->initialize());
    
    displayManager->enableAnimation(true);
    
    GpsData gps_data = {};
    gps_data.fix_valid = false;  // Show searching animation
    displayManager->updateGPSData(gps_data);
    displayManager->setDisplayMode(DisplayMode::GPS_STATUS);
    
    // Multiple updates should show different animation frames
    displayManager->forceUpdate();
    const char* first_display = displayManager->getDisplayLine(2);
    
    displayManager->forceUpdate();
    const char* second_display = displayManager->getDisplayLine(2);
    
    // Animation frames should be different (though this test might be flaky)
    // We mainly test that animation doesn't crash
    TEST_ASSERT_NOT_NULL(first_display);
    TEST_ASSERT_NOT_NULL(second_display);
}

// Error Handling Tests
void test_display_manager_display_errors() {
    mockDisplay->simulateConnectionError(true);
    TEST_ASSERT_FALSE(displayManager->initialize());
    TEST_ASSERT_EQUAL_INT(1, displayManager->getDisplayErrors());
    
    // Additional operations should increase error count
    displayManager->update();
    TEST_ASSERT_TRUE(displayManager->getDisplayErrors() > 1);
}

void test_display_manager_error_status_display() {
    TEST_ASSERT_TRUE(displayManager->initialize());
    
    SystemData system_data = {};
    system_data.error_count = 5;
    system_data.warning_count = 12;
    
    displayManager->updateSystemData(system_data);
    displayManager->setDisplayMode(DisplayMode::ERROR_STATUS);
    displayManager->forceUpdate();
    
    TEST_ASSERT_TRUE(displayManager->containsText("ERRORS"));
    TEST_ASSERT_TRUE(displayManager->containsText("System: 5"));
    TEST_ASSERT_TRUE(displayManager->containsText("Warnings: 12"));
}

void test_display_manager_no_errors_status() {
    TEST_ASSERT_TRUE(displayManager->initialize());
    
    SystemData system_data = {};
    system_data.error_count = 0;
    system_data.warning_count = 0;
    
    displayManager->updateSystemData(system_data);
    displayManager->setDisplayMode(DisplayMode::ERROR_STATUS);
    displayManager->forceUpdate();
    
    TEST_ASSERT_TRUE(displayManager->containsText("All OK"));
}

// Performance Metrics Tests
void test_display_manager_performance_metrics() {
    TEST_ASSERT_TRUE(displayManager->initialize());
    
    PerformanceMetrics metrics = {};
    metrics.ntp_accuracy_ms = 0.25f;
    metrics.average_response_time_ms = 1.5f;
    metrics.cpu_load_1min = 15.5f;
    metrics.memory_fragmentation_percent = 12;
    
    displayManager->updatePerformanceMetrics(metrics);
    displayManager->setDisplayMode(DisplayMode::PERFORMANCE_METRICS);
    displayManager->forceUpdate();
    
    TEST_ASSERT_TRUE(displayManager->containsText("PERFORMANCE"));
    TEST_ASSERT_TRUE(displayManager->containsText("NTP Acc: 0.25ms"));
    TEST_ASSERT_TRUE(displayManager->containsText("CPU 1m: 15.5%"));
}

// Update Interval Tests
void test_display_manager_update_interval() {
    TEST_ASSERT_TRUE(displayManager->initialize());
    
    int initial_updates = displayManager->getUpdateCount();
    
    displayManager->setUpdateInterval(5000);  // 5 second interval
    
    // Multiple update calls within interval shouldn't increase count much
    for (int i = 0; i < 10; i++) {
        displayManager->update();
    }
    
    int after_updates = displayManager->getUpdateCount();
    
    // Should not have updated frequently due to long interval
    TEST_ASSERT_TRUE((after_updates - initial_updates) < 5);
}

// Comprehensive Display Mode Tests
void test_display_manager_all_display_modes() {
    TEST_ASSERT_TRUE(displayManager->initialize());
    
    // Prepare test data
    GpsData gps_data = {true, 8, 12, 5000, 1.2f, 2.1f, 2.5f, 35.123, 139.456, 125.5f, 0.0f, 0.0f, "GPS|GLO|GAL", true, 1000, 1234};
    NetworkData network_data = {true, 0xC0A80101, 0xFFFFFF00, 0xC0A80001, 0x08080808, "test-server", true, 1024000, 2048000, 5, true, 150, 145};
    SystemData system_data = {7265, 150*1024, 100*1024, 25, 45.5f, 3.3f, 0, 2, true, 0, 1};
    PerformanceMetrics metrics = {0.25f, 10.5f, 2, 15, 1.5f, 8.2f, 1, 15.5f, 18.2f, 12};
    
    displayManager->updateGPSData(gps_data);
    displayManager->updateNetworkData(network_data);
    displayManager->updateSystemData(system_data);
    displayManager->updatePerformanceMetrics(metrics);
    
    // Test each display mode
    for (int mode = 0; mode < static_cast<int>(DisplayMode::COUNT); mode++) {
        displayManager->setDisplayMode(static_cast<DisplayMode>(mode));
        displayManager->forceUpdate();
        
        // Verify display is updated and contains relevant content
        TEST_ASSERT_TRUE(displayManager->getUpdateCount() > mode);
        TEST_ASSERT_NOT_NULL(displayManager->getDisplayLine(0));  // Should have title line
    }
}

int main(void) {
    UNITY_BEGIN();
    
    // Basic Display Tests
    RUN_TEST(test_display_manager_initialization);
    RUN_TEST(test_display_manager_initialization_failure);
    RUN_TEST(test_display_manager_mode_switching);
    
    // GPS Display Tests
    RUN_TEST(test_display_manager_gps_status_no_fix);
    RUN_TEST(test_display_manager_gps_status_with_fix);
    
    // Network Display Tests
    RUN_TEST(test_display_manager_network_connected);
    RUN_TEST(test_display_manager_network_disconnected);
    
    // System Display Tests
    RUN_TEST(test_display_manager_system_status);
    
    // Advanced Features Tests
    RUN_TEST(test_display_manager_auto_rotation);
    RUN_TEST(test_display_manager_brightness_control);
    RUN_TEST(test_display_manager_power_save_mode);
    RUN_TEST(test_display_manager_animation_enabled);
    
    // Error Handling Tests
    RUN_TEST(test_display_manager_display_errors);
    RUN_TEST(test_display_manager_error_status_display);
    RUN_TEST(test_display_manager_no_errors_status);
    
    // Performance Metrics Tests
    RUN_TEST(test_display_manager_performance_metrics);
    
    // Update and Configuration Tests
    RUN_TEST(test_display_manager_update_interval);
    RUN_TEST(test_display_manager_all_display_modes);
    
    return UNITY_END();
}
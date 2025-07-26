#include "DisplayManager.h"
#include "HardwareConfig.h"

DisplayManager::DisplayManager() 
    : display(nullptr), i2cAddress(0), initialized(false), displayCount(0), 
      lastDisplay(0), currentMode(DISPLAY_GPS_TIME), modeChangeTime(0), 
      errorState(false), errorMessage(""), buttonLastPressed(0) {
}

bool DisplayManager::testI2CAddress(uint8_t address) {
    Serial.printf("Testing I2C address 0x%02X...\n", address);
    
    Wire.beginTransmission(address);
    Wire.write(0x00); // Command mode
    Wire.write(0xAE); // Display OFF command
    int result = Wire.endTransmission();
    
    if (result == 0) {
        Serial.printf("✅ Device responds at address 0x%02X\n", address);
        return true;
    } else {
        Serial.printf("❌ No response at address 0x%02X (error: %d)\n", address, result);
        return false;
    }
}

bool DisplayManager::initialize() {
    Serial.println("=== DisplayManager Initialization ===");
    
    // Try common OLED I2C addresses
    uint8_t testAddresses[] = {0x3C, 0x3D};
    bool found = false;
    
    for (int i = 0; i < 2; i++) {
        if (testI2CAddress(testAddresses[i])) {
            i2cAddress = testAddresses[i];
            found = true;
            break;
        }
    }
    
    if (!found) {
        Serial.println("❌ No OLED display found at common I2C addresses");
        return false;
    }
    
    // Clean up any existing display instance
    if (display) {
        delete display;
        display = nullptr;
    }
    
    Serial.printf("Creating OLED instance at address 0x%02X...\n", i2cAddress);
    
    // Create OLED instance: OLED(SDA, SCL, RESET, WIDTH, HEIGHT, CONTROLLER, ADDRESS)
    display = new OLED(0, 1, 255, OLED::W_128, OLED::H_64, OLED::CTRL_SH1106, i2cAddress);
    
    if (!display) {
        Serial.println("❌ Failed to create OLED instance");
        return false;
    }
    
    Serial.println("Initializing OLED display...");
    display->begin();
    
    // Enable SH1106 offset for 132x64 -> 128x64 conversion
    display->useOffset(true);
    
    Serial.println("Testing display with initial message...");
    display->clear();
    display->draw_string(0, 0, "GPS NTP Server");
    display->draw_string(0, 10, "Initializing...");
    char addrStr[20];
    sprintf(addrStr, "I2C: 0x%02X", i2cAddress);
    display->draw_string(0, 20, addrStr);
    display->display();
    
    initialized = true;
    Serial.println("✅ DisplayManager initialized successfully");
    
    return true;
}

void DisplayManager::init() {
    Serial.println("=== OLED Display Initialization ===");
    Serial.printf("Display address: 0x%02X\n", SCREEN_ADDRESS);
    
    // Use the new initialize method
    if (!initialize()) {
        Serial.println("❌ DisplayManager initialization failed");
        return;
    }
    
    displayCount = 0;
    lastDisplay = 0;
    currentMode = DISPLAY_GPS_TIME;
    modeChangeTime = millis();
    errorState = false;
    buttonLastPressed = 0;
    
    Serial.println("✅ OLED Display initialization completed");
}

void DisplayManager::checkDisplayButton() {
    unsigned long now = millis();
    if (digitalRead(BTN_DISPLAY_PIN) == LOW && (now - buttonLastPressed > 500)) {
        Serial.println("Display button pressed - switching mode");
        nextDisplayMode();
        displayCount = 1;
        buttonLastPressed = now;
    }
}

void DisplayManager::update() {
    // Non-blocking display management
    if (displayCount > 0) {
        unsigned long now = micros();
        if (now - lastDisplay > 1000) { // 1ms minimum interval
            if (displayCount < 10) {
                // Display will be updated by main loop with GPS data
                displayCount++;
                lastDisplay = now;
            } else {
                displayCount = 0;
                clearDisplay();
            }
        }
    }
}

void DisplayManager::displayInfo(const GpsSummaryData& gpsSummaryData) {
    if (!initialized || !display) return;
    
    if (errorState) {
        displayErrorScreen();
        return;
    }
    
    switch (currentMode) {
        case DISPLAY_GPS_TIME:
            displayGpsTimeScreen(gpsSummaryData);
            break;
        case DISPLAY_GPS_SATS:
            displayGpsSatsScreen(gpsSummaryData);
            break;
        default:
            displayGpsTimeScreen(gpsSummaryData);
            break;
    }
}

void DisplayManager::clearDisplay() {
    if (initialized && display) {
        display->clear();
        display->display();
    }
}

void DisplayManager::formatDateTime(const GpsSummaryData& gpsData, char* buffer, size_t bufferSize) {
    snprintf(buffer, bufferSize, "%04d/%02d/%02d %02d:%02d:%02d",
             gpsData.year, gpsData.month, gpsData.day,
             gpsData.hour, gpsData.min, gpsData.sec);
}

void DisplayManager::formatPosition(const GpsSummaryData& gpsData, char* buffer, size_t bufferSize) {
    snprintf(buffer, bufferSize, "Lat: %7.4f Long: %7.4f Height: %6.2fm",
             gpsData.latitude / 10000000.0, 
             gpsData.longitude / 10000000.0, 
             gpsData.altitude / 1000.0);
}

void DisplayManager::displayNtpStats(const NtpStatistics& ntpStats) {
    if (currentMode == DISPLAY_NTP_STATS && !errorState && initialized && display) {
        displayNtpStatsScreen(ntpStats);
    }
}

void DisplayManager::displaySystemStatus(bool gpsConnected, bool networkConnected, uint32_t uptimeSeconds) {
    if (currentMode == DISPLAY_SYSTEM_STATUS && !errorState && initialized && display) {
        displaySystemStatusScreen(gpsConnected, networkConnected, uptimeSeconds);
    }
}

void DisplayManager::displayError(const String& message) {
    setErrorState(message);
    if (initialized && display) {
        displayErrorScreen();
    }
}

void DisplayManager::nextDisplayMode() {
    currentMode = static_cast<DisplayMode>((currentMode + 1) % DISPLAY_MODE_COUNT);
    modeChangeTime = millis();
    Serial.print("Display mode changed to: ");
    Serial.println(currentMode);
}

void DisplayManager::setErrorState(const String& message) {
    errorState = true;
    errorMessage = message;
    currentMode = DISPLAY_ERROR;
}

void DisplayManager::clearErrorState() {
    errorState = false;
    errorMessage = "";
    currentMode = DISPLAY_GPS_TIME;
}

void DisplayManager::displayStartupScreen() {
    if (!initialized || !display) return;
    
    display->clear();
    display->draw_string(15, 10, "GPS NTP");
    display->draw_string(25, 25, "Server v1.0");
    display->draw_string(10, 40, "Initializing...");
    display->display();
    delay(2000);
}

void DisplayManager::displayGpsTimeScreen(const GpsSummaryData& gpsData) {
    if (!initialized || !display) return;
    
    display->clear();
    
    // Title
    display->draw_string(0, 0, "GPS Time & Position");
    display->draw_line(0, 9, 128, 9, OLED::WHITE);
    
    // Date/Time
    char dateTimeChr[32];
    formatDateTime(gpsData, dateTimeChr, sizeof(dateTimeChr));
    display->draw_string(0, 12, "Time:");
    display->draw_string(0, 22, dateTimeChr);
    
    // Position
    char latStr[20], lonStr[20], altStr[20];
    sprintf(latStr, "Lat: %7.4f", gpsData.latitude / 10000000.0);
    sprintf(lonStr, "Lon: %7.4f", gpsData.longitude / 10000000.0);
    sprintf(altStr, "Alt: %6.2fm", gpsData.altitude / 1000.0);
    
    display->draw_string(0, 32, latStr);
    display->draw_string(0, 42, lonStr);
    display->draw_string(0, 52, altStr);
    
    display->display();
}

void DisplayManager::displayGpsSatsScreen(const GpsSummaryData& gpsData) {
    if (!initialized || !display) return;
    
    display->clear();
    
    // Title
    display->draw_string(0, 0, "GPS Satellites");
    display->draw_line(0, 9, 128, 9, OLED::WHITE);
    
    // Satellite info
    char sivStr[20], fixStr[20];
    sprintf(sivStr, "SIV:    %2d", gpsData.SIV);
    sprintf(fixStr, "Fix:    %2d", gpsData.fixType);
    
    display->draw_string(0, 12, sivStr);
    display->draw_string(0, 22, fixStr);
    
    // Signal quality
    display->draw_string(70, 12, "Quality:");
    if (gpsData.fixType >= 3) {
        display->draw_string(70, 22, "Good");
    } else if (gpsData.fixType >= 2) {
        display->draw_string(70, 22, "Fair");
    } else {
        display->draw_string(70, 22, "Poor");
    }
    
    display->display();
}

void DisplayManager::displayNtpStatsScreen(const NtpStatistics& stats) {
    if (!initialized || !display) return;
    
    display->clear();
    
    // Title
    display->draw_string(0, 0, "NTP Server Stats");
    display->draw_line(0, 9, 128, 9, OLED::WHITE);
    
    // Statistics
    char reqStr[20], validStr[20], invalidStr[20], avgStr[25];
    sprintf(reqStr, "Requests: %d", stats.requests_total);
    sprintf(validStr, "Valid:    %d", stats.requests_valid);
    sprintf(invalidStr, "Invalid:  %d", stats.requests_invalid);
    sprintf(avgStr, "Avg time: %.1fms", stats.avg_processing_time);
    
    display->draw_string(0, 12, reqStr);
    display->draw_string(0, 22, validStr);
    display->draw_string(0, 32, invalidStr);
    display->draw_string(0, 42, avgStr);
    
    // Success rate
    if (stats.requests_total > 0) {
        int successRate = (stats.requests_valid * 100) / stats.requests_total;
        char successStr[20];
        sprintf(successStr, "Success:  %d%%", successRate);
        display->draw_string(0, 52, successStr);
    }
    
    display->display();
}

void DisplayManager::displaySystemStatusScreen(bool gpsConnected, bool networkConnected, uint32_t uptimeSeconds) {
    if (!initialized || !display) return;
    
    display->clear();
    
    // Title
    display->draw_string(0, 0, "System Status");
    display->draw_line(0, 9, 128, 9, OLED::WHITE);
    
    // Status indicators
    display->draw_string(0, 12, "GPS:");
    display->draw_string(50, 12, gpsConnected ? "CONNECTED" : "DISCONNECTED");
    
    display->draw_string(0, 22, "Network:");
    display->draw_string(50, 22, networkConnected ? "CONNECTED" : "DISCONNECTED");
    
    // Uptime
    uint32_t hours = uptimeSeconds / 3600;
    uint32_t minutes = (uptimeSeconds % 3600) / 60;
    uint32_t seconds = uptimeSeconds % 60;
    
    char uptimeStr[25];
    sprintf(uptimeStr, "Uptime: %02d:%02d:%02d", hours, minutes, seconds);
    display->draw_string(0, 32, uptimeStr);
    
    // Memory status (approximate)
    char memStr[20];
    sprintf(memStr, "Free RAM: %d KB", (524288 - 16880) / 1024);
    display->draw_string(0, 42, memStr);
    
    char buildStr[20];
    sprintf(buildStr, "Build: %s", __DATE__);
    display->draw_string(0, 52, buildStr);
    
    display->display();
}

void DisplayManager::displayErrorScreen() {
    if (!initialized || !display) return;
    
    display->clear();
    
    // Title
    display->draw_string(0, 0, "ERROR");
    display->draw_line(0, 9, 128, 9, OLED::WHITE);
    
    // Error message
    display->draw_string(0, 15, "System Error:");
    display->draw_string(0, 25, errorMessage.c_str());
    
    display->draw_string(0, 55, "Press BTN to continue");
    
    display->display();
}

void DisplayManager::drawProgressBar(int x, int y, int width, int height, int value, int maxValue) {
    if (!initialized || !display) return;
    
    // Draw border
    display->draw_rectangle(x, y, x + width - 1, y + height - 1, OLED::HOLLOW, OLED::WHITE);
    
    // Draw fill
    int fillWidth = (value * (width - 2)) / maxValue;
    if (fillWidth > 0) {
        display->draw_rectangle(x + 1, y + 1, x + fillWidth, y + height - 2, OLED::SOLID, OLED::WHITE);
    }
}

void DisplayManager::drawSignalBars(int x, int y, int signalStrength) {
    if (!initialized || !display) return;
    
    int barWidth = 3;
    int barSpacing = 4;
    int maxBars = 5;
    int activeBars = (signalStrength * maxBars) / 100;
    
    for (int i = 0; i < maxBars; i++) {
        int barHeight = 3 + (i * 2);
        int barX = x + (i * barSpacing);
        int barY = y + (10 - barHeight);
        
        if (i < activeBars) {
            display->draw_rectangle(barX, barY, barX + barWidth - 1, barY + barHeight - 1, OLED::SOLID, OLED::WHITE);
        } else {
            display->draw_rectangle(barX, barY, barX + barWidth - 1, barY + barHeight - 1, OLED::HOLLOW, OLED::WHITE);
        }
    }
}

const char* DisplayManager::getGnssName(int gnssId) {
    switch (gnssId) {
        case 0: return "GPS";
        case 1: return "SBAS";
        case 2: return "Galileo";
        case 3: return "BeiDou";
        case 4: return "IMES";
        case 5: return "QZSS";
        case 6: return "GLONASS";
        default: return "Unknown";
    }
}
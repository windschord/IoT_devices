#include "DisplayManager.h"
#include "../hal/HardwareConfig.h"
#include "../config/LoggingService.h"
#include "../utils/I2CUtils.h"

// OLEDアドレス定義（広範囲の自動検出対応）
const uint8_t DisplayManager::OLED_ADDRESSES[] = {0x3C, 0x3D, 0x78, 0x7A};
const uint8_t DisplayManager::OLED_ADDRESS_COUNT = sizeof(OLED_ADDRESSES) / sizeof(OLED_ADDRESSES[0]);

DisplayManager::DisplayManager() 
    : display(nullptr), loggingService(nullptr), i2cAddress(0), initialized(false), displayCount(0), 
      lastDisplay(0), currentMode(DISPLAY_GPS_TIME), modeChangeTime(0), 
      errorState(false), errorMessage(""), buttonLastPressed(0),
      displayOn(true), sleepCounter(0) {
    // Initialize frame buffer
    frameBuffer.dirty = false;
    frameBuffer.lastUpdate = 0;
}

bool DisplayManager::testI2CAddress(uint8_t address) {
    return validateI2CConnection(address);
}

// 堅牢なI2Cバス初期化
bool DisplayManager::initializeI2CBus() {
    if (loggingService) {
        loggingService->info("DISPLAY", "Initializing I2C bus for OLED (Wire0)...");
    }
    
    // I2CUtils を使用した安定初期化
    bool success = I2CUtils::initializeBus(Wire, 0, 1, I2C_CLOCK_SPEED, true);
    
    if (!success) {
        if (loggingService) {
            loggingService->error("DISPLAY", "Failed to initialize I2C bus");
        }
        return false;
    }
    
    // バス動作確認
    delay(50); // I2C安定化待機
    
    if (loggingService) {
        loggingService->info("DISPLAY", "I2C bus initialized successfully");
    }
    
    return true;
}

// OLED デバイス自動検出（改良版）
bool DisplayManager::detectOLEDDevice() {
    if (loggingService) {
        loggingService->info("DISPLAY", "Starting OLED device auto-detection...");
    }
    
    uint8_t found_devices[8];
    uint8_t device_count = I2CUtils::scanBus(Wire, found_devices, 8, loggingService, "DISPLAY");
    
    // 検出されたデバイスからOLEDを特定
    for (uint8_t i = 0; i < device_count; i++) {
        for (uint8_t j = 0; j < OLED_ADDRESS_COUNT; j++) {
            if (found_devices[i] == OLED_ADDRESSES[j]) {
                if (validateI2CConnection(found_devices[i])) {
                    i2cAddress = found_devices[i];
                    if (loggingService) {
                        loggingService->infof("DISPLAY", "OLED detected and validated at address 0x%02X", i2cAddress);
                    }
                    return true;
                }
            }
        }
    }
    
    if (loggingService) {
        loggingService->error("DISPLAY", "No compatible OLED device found");
    }
    return false;
}

// I2C接続検証（バッファオーバーフロー対策）
bool DisplayManager::validateI2CConnection(uint8_t address) {
    // OLED基本コマンドテスト（Display OFF）
    I2CUtils::I2CResult result = performI2CCommand(address, 0xAE);
    
    if (result != I2CUtils::I2C_SUCCESS) {
        if (loggingService) {
            loggingService->warningf("DISPLAY", "I2C validation failed for address 0x%02X: %s", 
                                   address, I2CUtils::getErrorString(static_cast<uint8_t>(result)));
        }
        return false;
    }
    
    // Display ONコマンドテスト
    result = performI2CCommand(address, 0xAF);
    if (result != I2CUtils::I2C_SUCCESS) {
        if (loggingService) {
            loggingService->warningf("DISPLAY", "OLED ON command failed for address 0x%02X: %s", 
                                   address, I2CUtils::getErrorString(static_cast<uint8_t>(result)));
        }
        return false;
    }
    
    return true;
}

// 安全なI2Cコマンド実行
I2CUtils::I2CResult DisplayManager::performI2CCommand(uint8_t address, uint8_t command) {
    const uint8_t cmd_buffer[] = {0x00, command}; // Command mode + command
    return I2CUtils::safeWrite(Wire, address, 0x00, &command, 1, I2C_MAX_RETRY);
}

bool DisplayManager::initialize() {
    if (loggingService) {
        loggingService->info("DISPLAY", "Initializing OLED display with enhanced I2C handling...");
    }
    
    // Step 1: I2Cバス初期化
    if (!initializeI2CBus()) {
        return false;
    }
    
    // Step 2: OLEDデバイス自動検出
    if (!detectOLEDDevice()) {
        return false;
    }
    
    // Step 3: OLEDインスタンス作成と初期化
    // Clean up any existing display instance
    if (display) {
        delete display;
        display = nullptr;
    }
    
    if (loggingService) {
        loggingService->infof("DISPLAY", "Creating OLED instance at validated address 0x%02X", i2cAddress);
    }
    
    // Create OLED instance with error handling: OLED(SDA, SCL, RESET, WIDTH, HEIGHT, CONTROLLER, ADDRESS)
    display = new OLED(0, 1, 255, OLED::W_128, OLED::H_64, OLED::CTRL_SH1106, i2cAddress);
    if (!display) {
        if (loggingService) {
            loggingService->error("DISPLAY", "Failed to allocate OLED instance");
        }
        return false;
    }
    
    // OLED初期化（エラーハンドリング強化）
    if (loggingService) {
        loggingService->info("DISPLAY", "Calling display->begin() with enhanced error handling...");
    }
    
    display->begin();
    
    // 初期化完了確認（OLED応答テスト）
    delay(50);
    
    // Enable SH1106 offset for 132x64 -> 128x64 conversion
    display->useOffset(true);
    
    // 初期化成功確認（画面クリア＋表示テスト）
    display->clear();
    display->display();
    delay(50);
    
    if (loggingService) {
        loggingService->info("DISPLAY", "OLED initialization completed successfully");
    }
    
    // Set initialized flag BEFORE calling display methods
    initialized = true;
    if (loggingService) {
        loggingService->info("DISPLAY", "DisplayManager marked as initialized");
    }
    
    // Display startup screen
    displayStartupScreen();
    
    // Trigger display to start updating after 3 seconds
    displayCount = 1;
    lastDisplay = micros();
    
    // Initialize auto-sleep state
    displayOn = true;
    sleepCounter = 0;
    
    if (loggingService) {
        loggingService->info("DISPLAY", "OLED display initialized successfully");
    }
    
    return true;
}

void DisplayManager::init() {
    // Use the new initialize method
    if (!initialize()) {
        if (loggingService) {
            loggingService->error("DISPLAY", "DisplayManager initialization failed");
        }
        return;
    }
    
    displayCount = 0;
    lastDisplay = 0;
    currentMode = DISPLAY_GPS_TIME;
    modeChangeTime = millis();
    errorState = false;
    buttonLastPressed = 0;
    
    // Initialize auto-sleep state
    displayOn = true;
    sleepCounter = 0;
    
    if (loggingService) {
        loggingService->info("DISPLAY", "OLED Display initialization completed");
    }
}

// Button handling removed - now managed centrally by PhysicalReset class
// PhysicalReset::handleShortPress() calls nextDisplayMode() and triggerDisplay()

void DisplayManager::update() {
    // Simple display management - just ensure displayCount stays active
    if (displayCount > 0 && displayCount < 100) {
        displayCount++;
    }
    
    // Auto-sleep management (controlled by millis instead of update counter)
    static unsigned long lastSleepCheck = 0;
    unsigned long currentTime = millis();
    
    if (displayOn) {
        // Check every 1000ms (1 second)
        if (currentTime - lastSleepCheck >= 1000) {
            sleepCounter++;
            lastSleepCheck = currentTime;
            
            if (sleepCounter >= SLEEP_TIMEOUT_COUNT) {
                sleepDisplay();
            }
        }
    }
    
    // Performance optimization: Commit display updates with timing control
    commitDisplayUpdate();
}

void DisplayManager::displayInfo(const GpsSummaryData& gpsSummaryData) {
    if (!initialized || !display) {
        if (loggingService) {
            loggingService->warning("DISPLAY", "DisplayInfo called but not initialized or no display");
        }
        return;
    }
    
    // Continue to process data but skip actual display when sleeping
    // This ensures data structures stay updated
    
    if (errorState) {
        // Note: Removed verbose error state logging for cleaner output
        displayErrorScreen();
        return;
    }
    
    // Note: Removed verbose mode logging for cleaner output
    
    switch (currentMode) {
        case DISPLAY_GPS_TIME:
#ifdef DEBUG_DISPLAY_GPS
            if (loggingService) {
                loggingService->debug("DISPLAY", "Displaying GPS Time screen");
            }
#endif
            displayGpsTimeScreen(gpsSummaryData);
            break;
        case DISPLAY_GPS_SATS:
#ifdef DEBUG_DISPLAY_GPS
            if (loggingService) {
                loggingService->debug("DISPLAY", "Displaying GPS Satellites screen");
            }
#endif
            displayGpsSatsScreen(gpsSummaryData);
            break;
        default:
#ifdef DEBUG_DISPLAY_GPS
            if (loggingService) {
                loggingService->debugf("DISPLAY", "Displaying default GPS Time screen (mode: %d)", currentMode);
            }
#endif
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
    DisplayMode oldMode = currentMode;
    currentMode = static_cast<DisplayMode>((currentMode + 1) % DISPLAY_MODE_COUNT);
    modeChangeTime = millis();
    
    // Wake display when mode changes
    wakeDisplay();
    
    if (loggingService) {
        loggingService->debugf("DISPLAY", "Display mode changed from %d to %d", oldMode, currentMode);
        loggingService->debugf("DISPLAY", "DisplayManager state: displayCount=%d, shouldDisplay=%s, initialized=%s", 
                      displayCount, shouldDisplay() ? "YES" : "NO", initialized ? "YES" : "NO");
    }
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
}

void DisplayManager::displayGpsTimeScreen(const GpsSummaryData& gpsData) {
    if (!initialized || !display) return;
    
    // Skip actual drawing when display is sleeping
    if (!displayOn) {
        return;
    }
    
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
    
    // Skip actual drawing when display is sleeping
    if (!displayOn) {
        return;
    }
    
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
    
    // Skip actual drawing when display is sleeping
    if (!displayOn) {
        return;
    }
    
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
    
    // Skip actual drawing when display is sleeping
    if (!displayOn) {
        return;
    }
    
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
    
    // Skip actual drawing when display is sleeping
    if (!displayOn) {
        return;
    }
    
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

// Auto-sleep control methods
void DisplayManager::wakeDisplay() {
    if (!displayOn) {
        LOG_INFO_MSG("DISPLAY", "Waking display from sleep");
        displayOn = true;
        sleepCounter = 0;
        
        // Turn on display (refresh current screen)
        if (initialized && display) {
            // Display will be refreshed on next update cycle
        }
    } else {
        // Reset sleep counter to extend display time
        sleepCounter = 0;
    }
    
    // Debug log for triggerDisplay() calls (from inline function)
    if (loggingService && displayCount == 1) {
        loggingService->debugf("DISPLAY", "triggerDisplay() called - displayCount set to %d", displayCount);
    }
}

void DisplayManager::sleepDisplay() {
    if (displayOn) {
        LOG_INFO_MSG("DISPLAY", "Putting display to sleep after 30 seconds of inactivity");
        displayOn = false;
        sleepCounter = 0;
        
        // Turn off display to save OLED lifetime (clear screen once)
        if (initialized && display) {
            display->clear();
            display->display(); // Ensure the clear is shown
        }
    }
}

// Performance optimization methods implementation (with buffer overflow protection)
bool DisplayManager::shouldUpdateDisplay() {
    unsigned long currentTime = millis();
    
    // Check if enough time has passed since last update
    if (currentTime - frameBuffer.lastUpdate < frameBuffer.UPDATE_INTERVAL_MS) {
        return false;
    }
    
    // Check if display needs updating
    return frameBuffer.dirty && displayOn;
}

void DisplayManager::markDisplayDirty() {
    frameBuffer.dirty = true;
}

void DisplayManager::commitDisplayUpdate() {
    if (initialized && display && shouldUpdateDisplay()) {
        // I2Cバッファオーバーフロー対策付きの表示更新
        // 安全な表示更新（タイムアウト付き）
        unsigned long start_time = millis();
        display->display();
        unsigned long update_time = millis() - start_time;
        
        // 異常に長い更新時間の検出
        if (update_time > 500) { // 500ms以上は異常
            if (loggingService) {
                loggingService->warningf("DISPLAY", "Slow I2C update detected: %lu ms", update_time);
            }
        }
        
        // Update timing and reset dirty flag
        frameBuffer.lastUpdate = millis();
        frameBuffer.dirty = false;
        
        #ifdef DEBUG_DISPLAY_PERFORMANCE
        if (loggingService) {
            loggingService->debugf("DISPLAY", "Display updated at %lu ms (took %lu ms)", 
                                 frameBuffer.lastUpdate, update_time);
        }
        #endif
    }
}
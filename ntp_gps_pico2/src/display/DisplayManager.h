#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <oled.h>
#include "../gps/GpsModel.h"
#include "../system/SystemTypes.h"
#include "../network/NtpTypes.h"
#include "../utils/I2CUtils.h"

// Forward declaration for LoggingService
class LoggingService;

// Display modes for page switching
enum DisplayMode {
    DISPLAY_GPS_TIME,
    DISPLAY_GPS_SATS,
    DISPLAY_NTP_STATS,
    DISPLAY_SYSTEM_STATUS,
    DISPLAY_ERROR,
    DISPLAY_MODE_COUNT
};

class DisplayManager {
private:
    OLED* display;
    LoggingService* loggingService;
    uint8_t i2cAddress;
    bool initialized;
    
    // I2C通信設定
    static const uint32_t I2C_CLOCK_SPEED = 100000; // 100kHz安定動作
    static const uint8_t I2C_MAX_RETRY = 3;
    static const uint8_t I2C_BUFFER_SIZE = 32; // バッファオーバーフロー防止
    
    // 自動検出用アドレスリスト
    static const uint8_t OLED_ADDRESSES[];
    static const uint8_t OLED_ADDRESS_COUNT;
    int displayCount;
    unsigned long lastDisplay;
    DisplayMode currentMode;
    unsigned long modeChangeTime;
    bool errorState;
    String errorMessage;
    unsigned long buttonLastPressed;
    
    // Auto-sleep functionality (simple counter-based)
    bool displayOn;
    int sleepCounter;
    static const int SLEEP_TIMEOUT_COUNT = 30; // 30 update cycles (approximately 30 seconds)
    
    // Performance optimization: Frame buffering
    struct DisplayBuffer {
        bool dirty;
        unsigned long lastUpdate;
        static const int UPDATE_INTERVAL_MS = 100; // Minimum 100ms between I2C updates
    } frameBuffer;

public:
    DisplayManager();
    void setLoggingService(LoggingService* loggingServiceInstance) { loggingService = loggingServiceInstance; }
    bool initialize();
    
    void init();
    void update();
    
    // I2C address auto-detection and connection test
    bool testI2CAddress(uint8_t address);
    bool isInitialized() const { return initialized; }
    
    // 強化されたI2C管理
    bool initializeI2CBus();
    bool detectOLEDDevice();
    bool validateI2CConnection(uint8_t address);
    I2CUtils::I2CResult performI2CCommand(uint8_t address, uint8_t command);
    void displayInfo(const GpsSummaryData& gpsSummaryData);
    void displayNtpStats(const NtpStatistics& ntpStats);
    void displaySystemStatus(bool gpsConnected, bool networkConnected, uint32_t uptimeSeconds);
    void displayError(const String& message);
    // Button handling removed - managed by PhysicalReset class
    void clearDisplay();
    void nextDisplayMode();
    
    bool shouldDisplay() const { return displayCount > 0; }
    void triggerDisplay() { 
        displayCount = 1; 
        lastDisplay = 0; 
        wakeDisplay(); // Wake display when triggered
        // Debug log moved to non-inline function to avoid header include complexity
    }
    
    // Auto-sleep control methods
    void wakeDisplay();
    void sleepDisplay();
    bool isDisplayOn() const { return displayOn; }
    void setErrorState(const String& message);
    void clearErrorState();
    DisplayMode getCurrentMode() const { return currentMode; }

private:
    void formatDateTime(const GpsSummaryData& gpsData, char* buffer, size_t bufferSize);
    void formatPosition(const GpsSummaryData& gpsData, char* buffer, size_t bufferSize);
    void displayStartupScreen();
    void displayGpsTimeScreen(const GpsSummaryData& gpsData);
    void displayGpsSatsScreen(const GpsSummaryData& gpsData);
    void displayNtpStatsScreen(const NtpStatistics& stats);
    void displaySystemStatusScreen(bool gpsConnected, bool networkConnected, uint32_t uptimeSeconds);
    void displayErrorScreen();
    void drawProgressBar(int x, int y, int width, int height, int value, int maxValue);
    void drawSignalBars(int x, int y, int signalStrength);
    const char* getGnssName(int gnssId);
    
    // Performance optimization methods
    bool shouldUpdateDisplay();
    void markDisplayDirty();
    void commitDisplayUpdate();
};

#endif // DISPLAY_MANAGER_H
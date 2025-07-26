#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <oled.h>
#include "Gps_model.h"
#include "SystemTypes.h"
#include "NtpTypes.h"

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
    uint8_t i2cAddress;
    bool initialized;
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

public:
    DisplayManager();
    bool initialize();
    
    void init();
    void update();
    
    // I2C address auto-detection and connection test
    bool testI2CAddress(uint8_t address);
    bool isInitialized() const { return initialized; }
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
        Serial.printf("triggerDisplay() called - displayCount set to %d\n", displayCount);
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
};

#endif // DISPLAY_MANAGER_H
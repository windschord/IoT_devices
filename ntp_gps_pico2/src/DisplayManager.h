#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>
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
    Adafruit_SH1106* display;
    int displayCount;
    unsigned long lastDisplay;
    DisplayMode currentMode;
    unsigned long modeChangeTime;
    bool errorState;
    String errorMessage;
    unsigned long buttonLastPressed;

public:
    DisplayManager(Adafruit_SH1106* displayInstance);
    
    void init();
    void update();
    void displayInfo(const GpsSummaryData& gpsSummaryData);
    void displayNtpStats(const NtpStatistics& ntpStats);
    void displaySystemStatus(bool gpsConnected, bool networkConnected, uint32_t uptimeSeconds);
    void displayError(const String& message);
    void checkDisplayButton();
    void clearDisplay();
    void nextDisplayMode();
    
    bool shouldDisplay() const { return displayCount > 0; }
    void triggerDisplay() { displayCount = 1; }
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
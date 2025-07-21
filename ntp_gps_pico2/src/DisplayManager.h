#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>
#include "Gps_model.h"
#include "SystemTypes.h"

class DisplayManager {
private:
    Adafruit_SH1106* display;
    int displayCount;
    unsigned long lastDisplay;

public:
    DisplayManager(Adafruit_SH1106* displayInstance);
    
    void init();
    void update();
    void displayInfo(const GpsSummaryData& gpsSummaryData);
    void checkDisplayButton();
    void clearDisplay();
    
    bool shouldDisplay() const { return displayCount > 0; }
    void triggerDisplay() { displayCount = 1; }

private:
    void formatDateTime(const GpsSummaryData& gpsData, char* buffer, size_t bufferSize);
    void formatPosition(const GpsSummaryData& gpsData, char* buffer, size_t bufferSize);
};

#endif // DISPLAY_MANAGER_H
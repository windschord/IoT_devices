#include "DisplayManager.h"
#include "HardwareConfig.h"

DisplayManager::DisplayManager(Adafruit_SH1106* displayInstance)
    : display(displayInstance), displayCount(0), lastDisplay(0) {
}

void DisplayManager::init() {
    display->begin(SH1106_SWITCHCAPVCC, SCREEN_ADDRESS, false);
    display->clearDisplay();
    display->display();
    
    // Initialize button pin
    pinMode(BTN_DISPLAY_PIN, INPUT_PULLUP);
    
    displayCount = 0;
    lastDisplay = 0;
}

void DisplayManager::checkDisplayButton() {
    if (digitalRead(BTN_DISPLAY_PIN) == LOW) {
        Serial.println("Button Display");
        displayCount = 1;
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
    char dateTimeChr[32];
    char posChr[128];
    
    formatDateTime(gpsSummaryData, dateTimeChr, sizeof(dateTimeChr));
    formatPosition(gpsSummaryData, posChr, sizeof(posChr));
    
    display->clearDisplay();
    
    display->setTextSize(1);
    display->setTextColor(WHITE);
    display->setCursor(0, 0);
    display->println("DateTime:");
    display->setCursor(0, 10);
    display->println(dateTimeChr);
    display->setCursor(0, 20);
    display->println("Position:");
    display->setCursor(0, 30);
    display->println(posChr);
    display->display();
    
    delay(1000);
}

void DisplayManager::clearDisplay() {
    display->clearDisplay();
    display->display();
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
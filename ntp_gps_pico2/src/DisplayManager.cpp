#include "DisplayManager.h"
#include "HardwareConfig.h"

DisplayManager::DisplayManager(Adafruit_SH1106* displayInstance)
    : display(displayInstance), displayCount(0), lastDisplay(0), 
      currentMode(DISPLAY_GPS_TIME), modeChangeTime(0), errorState(false), 
      errorMessage(""), buttonLastPressed(0) {
}

void DisplayManager::init() {
    display->begin(SH1106_SWITCHCAPVCC, SCREEN_ADDRESS, false);
    display->clearDisplay();
    display->display();
    
    // Initialize button pin
    pinMode(BTN_DISPLAY_PIN, INPUT_PULLUP);
    
    displayCount = 0;
    lastDisplay = 0;
    currentMode = DISPLAY_GPS_TIME;
    modeChangeTime = millis();
    errorState = false;
    buttonLastPressed = 0;
    
    // Show startup screen
    displayStartupScreen();
    Serial.println("OLED Display initialized successfully");
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

void DisplayManager::displayNtpStats(const NtpStatistics& ntpStats) {
    if (currentMode == DISPLAY_NTP_STATS && !errorState) {
        displayNtpStatsScreen(ntpStats);
    }
}

void DisplayManager::displaySystemStatus(bool gpsConnected, bool networkConnected, uint32_t uptimeSeconds) {
    if (currentMode == DISPLAY_SYSTEM_STATUS && !errorState) {
        displaySystemStatusScreen(gpsConnected, networkConnected, uptimeSeconds);
    }
}

void DisplayManager::displayError(const String& message) {
    setErrorState(message);
    displayErrorScreen();
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
    display->clearDisplay();
    display->setTextSize(2);
    display->setTextColor(WHITE);
    display->setCursor(15, 10);
    display->println("GPS NTP");
    display->setTextSize(1);
    display->setCursor(25, 35);
    display->println("Server v1.0");
    display->setCursor(10, 50);
    display->println("Initializing...");
    display->display();
    delay(2000);
}

void DisplayManager::displayGpsTimeScreen(const GpsSummaryData& gpsData) {
    display->clearDisplay();
    display->setTextSize(1);
    display->setTextColor(WHITE);
    
    // Title
    display->setCursor(0, 0);
    display->println("GPS Time & Position");
    display->drawLine(0, 9, 128, 9, WHITE);
    
    // Date/Time
    char dateTimeChr[32];
    formatDateTime(gpsData, dateTimeChr, sizeof(dateTimeChr));
    display->setCursor(0, 12);
    display->print("Time: ");
    display->println(dateTimeChr);
    
    // Position
    display->setCursor(0, 22);
    display->printf("Lat: %7.4f", gpsData.latitude / 10000000.0);
    display->setCursor(0, 32);
    display->printf("Lon: %7.4f", gpsData.longitude / 10000000.0);
    display->setCursor(0, 42);
    display->printf("Alt: %6.2fm", gpsData.altitude / 1000.0);
    
    // Status indicators
    display->setCursor(0, 54);
    display->print("Time: ");
    display->print(gpsData.timeValid ? "OK" : "ERR");
    display->setCursor(50, 54);
    display->print("Date: ");
    display->print(gpsData.dateValid ? "OK" : "ERR");
    
    display->display();
}

void DisplayManager::displayGpsSatsScreen(const GpsSummaryData& gpsData) {
    display->clearDisplay();
    display->setTextSize(1);
    display->setTextColor(WHITE);
    
    // Title
    display->setCursor(0, 0);
    display->println("GPS Satellites");
    display->drawLine(0, 9, 128, 9, WHITE);
    
    // Satellite counts by constellation
    display->setCursor(0, 12);
    display->printf("SIV:    %2d", gpsData.SIV);
    display->setCursor(0, 22);
    display->printf("Fix:    %2d", gpsData.fixType);
    display->setCursor(0, 32);
    display->printf("GAL:    --"); // TODO: Add individual constellation counts from UBX-NAV-SAT
    display->setCursor(0, 42);
    display->printf("BDS:    --"); // TODO: Add BeiDou count
    
    // Signal quality indicator
    display->setCursor(70, 12);
    display->print("Quality:");
    display->setCursor(70, 22);
    if (gpsData.fixType >= 3) {
      display->print("Good");
    } else if (gpsData.fixType >= 2) {
      display->print("Fair");
    } else {
      display->print("Poor");
    }
    
    // Signal strength bars based on satellites in view
    drawSignalBars(70, 35, min(gpsData.SIV * 10, 100));
    
    display->display();
}

void DisplayManager::displayNtpStatsScreen(const NtpStatistics& stats) {
    display->clearDisplay();
    display->setTextSize(1);
    display->setTextColor(WHITE);
    
    // Title
    display->setCursor(0, 0);
    display->println("NTP Server Stats");
    display->drawLine(0, 9, 128, 9, WHITE);
    
    // Statistics
    display->setCursor(0, 12);
    display->printf("Requests: %d", stats.requests_total);
    display->setCursor(0, 22);
    display->printf("Valid:    %d", stats.requests_valid);
    display->setCursor(0, 32);
    display->printf("Invalid:  %d", stats.requests_invalid);
    display->setCursor(0, 42);
    display->printf("Avg time: %.1fms", stats.avg_processing_time);
    
    // Response rate
    if (stats.requests_total > 0) {
        int successRate = (stats.requests_valid * 100) / stats.requests_total;
        display->setCursor(0, 52);
        display->printf("Success:  %d%%", successRate);
        
        // Success rate bar
        drawProgressBar(70, 52, 50, 8, successRate, 100);
    }
    
    display->display();
}

void DisplayManager::displaySystemStatusScreen(bool gpsConnected, bool networkConnected, uint32_t uptimeSeconds) {
    display->clearDisplay();
    display->setTextSize(1);
    display->setTextColor(WHITE);
    
    // Title
    display->setCursor(0, 0);
    display->println("System Status");
    display->drawLine(0, 9, 128, 9, WHITE);
    
    // Status indicators
    display->setCursor(0, 12);
    display->print("GPS:     ");
    display->println(gpsConnected ? "CONNECTED" : "DISCONNECTED");
    
    display->setCursor(0, 22);
    display->print("Network: ");
    display->println(networkConnected ? "CONNECTED" : "DISCONNECTED");
    
    // Uptime
    uint32_t hours = uptimeSeconds / 3600;
    uint32_t minutes = (uptimeSeconds % 3600) / 60;
    uint32_t seconds = uptimeSeconds % 60;
    
    display->setCursor(0, 32);
    display->printf("Uptime: %02d:%02d:%02d", hours, minutes, seconds);
    
    // Memory status (approximate)
    display->setCursor(0, 42);
    display->printf("Free RAM: %d KB", (524288 - 16880) / 1024);
    
    display->setCursor(0, 52);
    display->printf("Build: %s", __DATE__);
    
    display->display();
}

void DisplayManager::displayErrorScreen() {
    display->clearDisplay();
    display->setTextSize(1);
    display->setTextColor(WHITE);
    
    // Title
    display->setCursor(0, 0);
    display->println("ERROR");
    display->drawLine(0, 9, 128, 9, WHITE);
    
    // Error message
    display->setCursor(0, 15);
    display->println("System Error:");
    display->setCursor(0, 25);
    
    // Word wrap for error message
    String msg = errorMessage;
    int lineLength = 21; // Characters per line on 128px wide screen
    int yPos = 25;
    
    while (msg.length() > 0 && yPos < 55) {
        String line = msg.substring(0, min(lineLength, (int)msg.length()));
        display->setCursor(0, yPos);
        display->println(line);
        msg = msg.substring(line.length());
        yPos += 10;
    }
    
    display->setCursor(0, 55);
    display->println("Press BTN to continue");
    
    display->display();
}

void DisplayManager::drawProgressBar(int x, int y, int width, int height, int value, int maxValue) {
    // Draw border
    display->drawRect(x, y, width, height, WHITE);
    
    // Draw fill
    int fillWidth = (value * (width - 2)) / maxValue;
    display->fillRect(x + 1, y + 1, fillWidth, height - 2, WHITE);
}

void DisplayManager::drawSignalBars(int x, int y, int signalStrength) {
    int barWidth = 3;
    int barSpacing = 4;
    int maxBars = 5;
    int activeBars = (signalStrength * maxBars) / 100;
    
    for (int i = 0; i < maxBars; i++) {
        int barHeight = 3 + (i * 2);
        int barX = x + (i * barSpacing);
        int barY = y + (10 - barHeight);
        
        if (i < activeBars) {
            display->fillRect(barX, barY, barWidth, barHeight, WHITE);
        } else {
            display->drawRect(barX, barY, barWidth, barHeight, WHITE);
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
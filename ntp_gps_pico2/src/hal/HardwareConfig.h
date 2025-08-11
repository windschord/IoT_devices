#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

// GPIO Pin Assignments
#define GPS_PPS_PIN 8
#define GPS_SDA_PIN 6  // I2C1 bus shared with RTC
#define GPS_SCL_PIN 7  // I2C1 bus shared with RTC
#define BTN_DISPLAY_PIN 11
#define LED_GNSS_FIX_PIN 4   // GNSS Fix Status LED (Green)
#define LED_NETWORK_PIN 5    // Network Status LED (Blue)
#define LED_ERROR_PIN 14     // Error Status LED (Red)
#define LED_PPS_PIN 15       // PPS Status LED (Yellow)
#define LED_ONBOARD_PIN 25   // Onboard LED

// W5500 SPI Pins
#define W5500_RST_PIN 20
#define W5500_INT_PIN 21
#define W5500_CS_PIN 17

// OLED Display Configuration (definitions moved to Constants.h to avoid conflicts)

// NTP Configuration (definitions moved to Constants.h to avoid conflicts)

// Serial Communication
#define SERIAL_BAUD_RATE 9600

// Default MAC Address
// Generated from: https://www.hellion.org.uk/cgi-bin/randmac.pl?scope=local&type=unicast
#define DEFAULT_MAC_ADDRESS {0x6e, 0xc9, 0x4c, 0x32, 0x3a, 0xf6}

// RTC Configuration (using RTClib DS3231 class directly)

#endif // HARDWARE_CONFIG_H
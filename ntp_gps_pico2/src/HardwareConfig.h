#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

// GPIO Pin Assignments
#define GPS_PPS_PIN 8
#define GPS_SDA_PIN 6  // I2C1 bus shared with RTC
#define GPS_SCL_PIN 7  // I2C1 bus shared with RTC
#define BTN_DISPLAY_PIN 11
#define LED_ERROR_PIN 14
#define LED_PPS_PIN 15
#define LED_ONBOARD_PIN 25

// W5500 SPI Pins
#define W5500_RST_PIN 20
#define W5500_INT_PIN 21
#define W5500_CS_PIN 17

// OLED Display Configuration
#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

// NTP Configuration
#define NTP_PORT 123           // NTP standard port
#define NTP_PACKET_SIZE 48     // NTP packet size

// Serial Communication
#define SERIAL_BAUD_RATE 9600

// Default MAC Address
// Generated from: https://www.hellion.org.uk/cgi-bin/randmac.pl?scope=local&type=unicast
#define DEFAULT_MAC_ADDRESS {0x6e, 0xc9, 0x4c, 0x32, 0x3a, 0xf6}

// RTC Configuration (using RTClib DS3231 class directly)

#endif // HARDWARE_CONFIG_H
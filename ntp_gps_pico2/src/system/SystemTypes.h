#ifndef SYSTEM_TYPES_H
#define SYSTEM_TYPES_H

#include <stdint.h>

// Network monitoring and error handling structure
struct NetworkMonitor {
  bool isConnected;              // Ethernet connection status
  bool dhcpActive;               // DHCP acquisition status
  unsigned long lastLinkCheck;   // Last link check time
  unsigned long linkCheckInterval; // Link check interval (5 seconds)
  int reconnectAttempts;         // Reconnection attempt count
  int maxReconnectAttempts;      // Maximum reconnection attempts
  unsigned long lastReconnectTime; // Last reconnection attempt time
  unsigned long reconnectInterval; // Reconnection interval (30 seconds)
  uint32_t localIP;              // Current local IP
  uint32_t gateway;              // Gateway IP
  uint32_t dnsServer;            // DNS server IP
  bool ntpServerActive;          // NTP server status
};

// UDP socket management structure
struct UdpSocketManager {
  bool ntpSocketOpen;            // NTP UDP socket status
  unsigned long lastSocketCheck; // Last socket check
  unsigned long socketCheckInterval; // Socket check interval (10 seconds)
  int socketErrors;              // Socket error count
};

// GPS signal monitoring and fallback management
struct GpsMonitor {
  unsigned long lastValidTime;     // Last valid GPS time acquisition time
  unsigned long lastPpsTime;       // Last PPS signal reception time
  unsigned long ppsTimeoutMs;      // PPS timeout time (30 seconds)
  unsigned long gpsTimeoutMs;      // GPS time timeout time (60 seconds)
  bool ppsActive;                  // PPS signal status
  bool gpsTimeValid;               // GPS time validity
  int signalQuality;               // Signal quality (0-10)
  int satelliteCount;              // Number of received satellites
  bool inFallbackMode;             // Fallback mode status
};

// High-precision time synchronization structure
struct TimeSync {
  unsigned long gpsTime;      // GPS time (Unix timestamp)
  unsigned long ppsTime;      // Microseconds when PPS was received
  unsigned long rtcTime;      // RTC time
  unsigned long lastGpsUpdate; // Last GPS time update (millis())
  bool synchronized;          // Synchronization status
  float accuracy;            // Accuracy (seconds)
};

#endif // SYSTEM_TYPES_H
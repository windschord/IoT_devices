
#ifndef GPS_MODEL_H
#define GPS_MODEL_H

#include <Arduino.h>

struct GpsSummaryData
{
  long latitude;
  long longitude;
  long altitude;
  byte SIV;
  byte fixType;
  uint16_t year; // Year (UTC)
  uint8_t month; // Month, range 1..12 (UTC)
  uint8_t day;   // Day of month, range 1..31 (UTC)
  uint8_t hour;  // Hour of day, range 0..23 (UTC)
  uint8_t min;   // Minute of hour, range 0..59 (UTC)
  uint8_t sec;   // Seconds of minute, range 0..60 (UTC)
  unsigned long msec;
  bool timeValid;
  bool dateValid;
};

// Web GPS表示用のデータ構造
#define MAX_SATELLITES 32

// 個別衛星情報構造体
struct satellite_info_t {
    uint8_t  prn;             // Pseudo Random Number (satellite identifier)
    uint8_t  constellation;   // Constellation type (0=GPS, 1=SBAS, 2=Galileo, 3=BeiDou, 4=GLONASS, 5=QZSS)
    float    azimuth;         // Azimuth angle (0-359 degrees)
    float    elevation;       // Elevation angle (0-90 degrees)
    uint8_t  signal_strength; // Signal strength (C/N0 in dBHz)
    bool     used_in_nav;     // Used in navigation solution
    bool     tracked;         // Currently being tracked
};

// Web GPS表示用包括的データ構造体
struct web_gps_data_t {
    // Position and Time Information
    double   latitude;        // Latitude in degrees
    double   longitude;       // Longitude in degrees
    float    altitude;        // Altitude in meters
    float    speed;           // Speed in m/s
    float    course;          // Course over ground in degrees
    uint32_t utc_time;        // UTC time (Unix timestamp)
    uint32_t ttff;            // Time to first fix in seconds
    
    // Fix Information  
    uint8_t  fix_type;        // Fix type (0=no fix, 2=2D, 3=3D, 4=RTK)
    float    pdop;            // Position dilution of precision
    float    hdop;            // Horizontal dilution of precision
    float    vdop;            // Vertical dilution of precision
    float    accuracy_3d;     // 3D accuracy estimate (meters)
    float    accuracy_2d;     // 2D accuracy estimate (meters)
    
    // Constellation Statistics
    uint8_t  satellites_total;     // Total satellites visible
    uint8_t  satellites_used;      // Satellites used in navigation
    uint8_t  satellites_gps_total; // Total GPS satellites
    uint8_t  satellites_gps_used;  // GPS satellites used
    uint8_t  satellites_glonass_total; // Total GLONASS satellites
    uint8_t  satellites_glonass_used;  // GLONASS satellites used
    uint8_t  satellites_galileo_total; // Total Galileo satellites
    uint8_t  satellites_galileo_used;  // Galileo satellites used
    uint8_t  satellites_beidou_total;  // Total BeiDou satellites
    uint8_t  satellites_beidou_used;   // BeiDou satellites used
    uint8_t  satellites_sbas_total;    // Total SBAS satellites
    uint8_t  satellites_sbas_used;     // SBAS satellites used
    uint8_t  satellites_qzss_total;    // Total QZSS satellites
    uint8_t  satellites_qzss_used;     // QZSS satellites used
    
    // Individual Satellite Information
    uint8_t  satellite_count;  // Number of satellites in array
    satellite_info_t satellites[MAX_SATELLITES]; // Individual satellite data
    
    // Constellation Enable Status
    bool     gps_enabled;      // GPS constellation enabled
    bool     glonass_enabled;  // GLONASS constellation enabled
    bool     galileo_enabled;  // Galileo constellation enabled
    bool     beidou_enabled;   // BeiDou constellation enabled
    bool     sbas_enabled;     // SBAS constellation enabled
    bool     qzss_enabled;     // QZSS constellation enabled
    
    // System Status
    bool     data_valid;       // Data validity flag
    uint32_t last_update;      // Last update timestamp (millis())
};

#endif // GPS_MODEL_H
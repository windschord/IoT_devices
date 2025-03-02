
#ifndef GPS_MODEL_H
#define GPS_MODEL_H

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

#endif // GPS_MODEL_H
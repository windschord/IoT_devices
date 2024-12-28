#include "Gps_Client.h"

void GpsClient::newNAVSAT(UBX_NAV_SAT_data_t *ubxDataStruct)
{
  #if defined(DEBUG_CONSOLE_GPS)
    stream_.println("callback from gps");
  #endif
  ubxNavSatData_t = ubxDataStruct;
}

void GpsClient::getPVTdata(UBX_NAV_PVT_data_t *ubxDataStruct)
{
  gpsSummaryData.latitude = ubxDataStruct->lat;
  gpsSummaryData.longitude = ubxDataStruct->lon;
  gpsSummaryData.altitude = ubxDataStruct->hMSL;
  gpsSummaryData.SIV = ubxDataStruct->numSV;
  gpsSummaryData.timeValid = ubxDataStruct->valid.bits.validTime;
  gpsSummaryData.dateValid = ubxDataStruct->valid.bits.validDate;
  gpsSummaryData.year = ubxDataStruct->year;
  gpsSummaryData.month = ubxDataStruct->month;
  gpsSummaryData.day = ubxDataStruct->day;
  gpsSummaryData.hour = ubxDataStruct->hour;
  gpsSummaryData.min = ubxDataStruct->min;
  gpsSummaryData.sec = ubxDataStruct->sec;
  gpsSummaryData.msec = ubxDataStruct->iTOW % 1000;
  gpsSummaryData.fixType = ubxDataStruct->fixType;
}



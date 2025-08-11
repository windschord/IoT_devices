
#ifndef GPS_CLIENT_H
#define GPS_CLIENT_H

#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
#include "GpsModel.h"
#include <QZQSM.h>
#include <QZSSDCX.h>

class GpsClient
{
public:
    GpsClient(Stream &stream) : stream(stream) {};
    void getPvtData(UBX_NAV_PVT_data_t *ubxDataStruct);
    void newSfrBx(UBX_RXM_SFRBX_data_t *data);
    void newNavSat(UBX_NAV_SAT_data_t *data);

    // Web GPS display methods
    WebGpsData getWebGpsData();
    void updateWebGpsData(UBX_NAV_PVT_data_t *pvtData, UBX_NAV_SAT_data_t *satData);
    
    // Existing methods
    GpsSummaryData getGpsSummaryData() { return gpsSummaryData; }
    UBX_NAV_SAT_data_t *getUbxNavSatData() { return ubxNavSatData; }

private:
    Stream &stream;
    UBX_NAV_SAT_data_t *ubxNavSatData;
    GpsSummaryData gpsSummaryData;
    WebGpsData webGpsData;
    
    // Private methods for Web GPS display
    void processNavSatData(UBX_NAV_SAT_data_t *satData);
    uint8_t mapGnssIdToConstellation(uint8_t gnssId);
    void resetConstellationStats();
    void calculateConstellationStats();
    
    const char *dwordToString(uint32_t value);
};

#endif // GPS_CLIENT_H

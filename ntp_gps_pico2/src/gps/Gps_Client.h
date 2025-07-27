
#ifndef GPS_CLIENT_H
#define GPS_CLIENT_H

#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
#include "Gps_model.h"
#include <QZQSM.h>
#include <QZSSDCX.h>

class GpsClient
{
public:
    GpsClient(Stream &stream) : stream_(stream) {};
    void getPVTdata(UBX_NAV_PVT_data_t *ubxDataStruct);
    void newSFRBX(UBX_RXM_SFRBX_data_t *data);
    void newNAVSAT(UBX_NAV_SAT_data_t *data);

    // Web GPS表示用メソッド
    web_gps_data_t getWebGpsData();
    void updateWebGpsData(UBX_NAV_PVT_data_t *pvtData, UBX_NAV_SAT_data_t *satData);
    
    // 既存メソッド
    GpsSummaryData getGpsSummaryData() { return gpsSummaryData; }
    UBX_NAV_SAT_data_t *getUbxNavSatData_t() { return ubxNavSatData_t; }

private:
    Stream &stream_;
    UBX_NAV_SAT_data_t *ubxNavSatData_t;
    GpsSummaryData gpsSummaryData;
    web_gps_data_t webGpsData;
    
    // Web GPS表示用プライベートメソッド
    void processNavSatData(UBX_NAV_SAT_data_t *satData);
    uint8_t mapGnssIdToConstellation(uint8_t gnssId);
    void resetConstellationStats();
    void calculateConstellationStats();
    
    const char *dwrd_to_str(uint32_t value);
};

#endif // GPS_CLIENT_H

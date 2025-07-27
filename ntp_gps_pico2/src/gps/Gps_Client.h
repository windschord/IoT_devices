
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

    GpsSummaryData getGpsSummaryData() { return gpsSummaryData; }
    UBX_NAV_SAT_data_t *getUbxNavSatData_t() { return ubxNavSatData_t; }

private:
    Stream &stream_;
    UBX_NAV_SAT_data_t *ubxNavSatData_t;
    GpsSummaryData gpsSummaryData;
    const char *dwrd_to_str(uint32_t value);
};

// SparkFun u-blox GNSS Library mock for native testing
#include "arduino_mock.h"
#include <stdint.h>

// Mock GNSS data structures
struct UBX_NAV_PVT_data_t {
    uint32_t iTOW;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t valid;
    uint32_t tAcc;
    int32_t nano;
    uint8_t fixType;
    uint8_t flags;
    uint8_t flags2;
    uint8_t numSV;
    int32_t lon;
    int32_t lat;
    int32_t height;
    int32_t hMSL;
    uint32_t hAcc;
    uint32_t vAcc;
    int32_t velN;
    int32_t velE;
    int32_t velD;
    int32_t gSpeed;
    int32_t headMot;
    uint32_t sAcc;
    uint32_t headAcc;
    uint16_t pDOP;
    uint16_t flags3;
    uint8_t reserved1[4];
    int32_t headVeh;
    int16_t magDec;
    uint16_t magAcc;
};

struct UBX_NAV_SAT_data_t {
    uint32_t iTOW;
    uint8_t version;
    uint8_t numSvs;
    uint8_t reserved1[2];
};

// Mock GNSS class
class SFE_UBLOX_GNSS {
public:
    bool begin() { return true; }
    void checkUblox() {}
    void checkCallbacks() {}
    bool setI2CAddress(uint8_t) { return true; }
    bool isConnected() { return true; }
    
    // Mock data access
    UBX_NAV_PVT_data_t* getPVT() {
        static UBX_NAV_PVT_data_t mockData = {};
        return &mockData;
    }
    
    UBX_NAV_SAT_data_t* getNAVSAT() {
        static UBX_NAV_SAT_data_t mockData = {};
        return &mockData;
    }
    
    bool getPVT(uint16_t = 0) { return true; }
    bool getNAVSAT(uint16_t = 0) { return true; }
};
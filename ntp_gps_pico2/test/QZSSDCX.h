// QZSSDCX.h mock for native testing
#include "arduino_mock.h"

// Mock QZSS DCX structures
struct DCXMessage {
    uint8_t category;
    uint8_t subcategory;
    char message[256];
    bool valid;
};

class QZSSDCX {
public:
    QZSSDCX() {}
    
    bool begin() { return true; }
    bool process(const uint8_t* data, size_t length) { return true; }
    bool hasDisasterMessage() { return false; }
    DCXMessage getDisasterMessage() { return DCXMessage(); }
};
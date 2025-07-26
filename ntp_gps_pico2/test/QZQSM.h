// QZQSM.h mock for native testing
#include "arduino_mock.h"

// Mock QZSS message structures
struct QZSSMessage {
    uint8_t data[256];
    size_t length;
    bool valid;
};

class QZQSM {
public:
    QZQSM() {}
    
    bool begin() { return true; }
    bool process(const uint8_t* data, size_t length) { return true; }
    bool hasMessage() { return false; }
    QZSSMessage getMessage() { return QZSSMessage(); }
};
// SPI.h mock for native testing
#include "arduino_mock.h"

class MockSPI {
public:
    void begin() {}
    void end() {}
    void beginTransaction(uint32_t) {}
    void endTransaction() {}
    uint8_t transfer(uint8_t data) { return data; }
};

extern MockSPI SPI;
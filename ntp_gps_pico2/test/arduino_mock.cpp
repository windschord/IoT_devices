#include "arduino_mock.h"
#include "SPI.h"
#include "Ethernet.h"
#include "EEPROM.h"

// Global mock instances
MockSerial Serial;
MockWire Wire;
MockWire Wire1;
MockSPI SPI;
MockEthernet Ethernet;
MockEEPROM EEPROM;
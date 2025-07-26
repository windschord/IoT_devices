// Ethernet.h mock for native testing
#include "arduino_mock.h"
#include <stdint.h>

enum EthernetHardwareStatus {
    EthernetNoHardware,
    EthernetW5100,
    EthernetW5200,
    EthernetW5500
};

enum EthernetLinkStatus {
    Unknown,
    LinkON,
    LinkOFF
};

class IPAddress {
public:
    IPAddress() {}
    IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {}
    operator uint32_t() const { return 0; }
};

class MockEthernet {
public:
    int begin() { return 1; }
    int begin(uint8_t*) { return 1; }
    void maintain() {}
    EthernetHardwareStatus hardwareStatus() { return EthernetW5500; }
    EthernetLinkStatus linkStatus() { return LinkON; }
    IPAddress localIP() { return IPAddress(192, 168, 1, 100); }
    IPAddress subnetMask() { return IPAddress(255, 255, 255, 0); }
    IPAddress gatewayIP() { return IPAddress(192, 168, 1, 1); }
    IPAddress dnsServerIP() { return IPAddress(8, 8, 8, 8); }
};

extern MockEthernet Ethernet;

class EthernetServer {
public:
    EthernetServer(uint16_t) {}
    void begin() {}
};

class EthernetUDP {
public:
    uint8_t begin(uint16_t) { return 1; }
    void stop() {}
    int beginPacket(IPAddress, uint16_t) { return 1; }
    int endPacket() { return 1; }
    size_t write(const uint8_t*, size_t) { return 0; }
    int parsePacket() { return 0; }
    int available() { return 0; }
    int read() { return -1; }
    int read(unsigned char*, size_t) { return 0; }
    IPAddress remoteIP() { return IPAddress(); }
    uint16_t remotePort() { return 0; }
};
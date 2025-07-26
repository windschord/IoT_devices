// WebServer.h mock for native testing
#include "arduino_mock.h"

class WebServer {
public:
    WebServer() {}
    WebServer(int port) {}
    
    void begin() {}
    void handleClient() {}
    void on(const String&, void(*)()) {}
    void send(int, const String&, const String&) {}
    void sendHeader(const String&, const String&) {}
    
    String arg(const String&) { return String(""); }
    bool hasArg(const String&) { return false; }
    int args() { return 0; }
    String argName(int) { return String(""); }
};
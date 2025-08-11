#include "webserver.h"
#include "GpsWebServer.h"

// 静的インスタンス（従来の互換性維持のため）
static ModernGpsWebServer* modernGpsWebServerInstance = nullptr;

void GpsWebServer::handleClient(Stream &stream, EthernetServer &server, UBX_NAV_SAT_data_t *ubxNavSatData_t, GpsSummaryData gpsSummaryData) {
    // 新しい実装への転送
    if (!modernGpsWebServerInstance) {
        modernGpsWebServerInstance = new ModernGpsWebServer();
        
        // 依存性を転送
        modernGpsWebServerInstance->setNtpServer(ntpServer);
        modernGpsWebServerInstance->setConfigManager(configManager);
        modernGpsWebServerInstance->setPrometheusMetrics(prometheusMetrics);
        modernGpsWebServerInstance->setLoggingService(loggingService);
        modernGpsWebServerInstance->setGpsClient(gpsClient);
    }
    
    modernGpsWebServerInstance->handleClient(stream, server, ubxNavSatData_t, gpsSummaryData);
}

void GpsWebServer::printHeader(EthernetClient &client, String contentType) {
    // 新しい実装では HttpResponseBuilder を使用するため、この関数は廃止予定
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: " + contentType);
    client.println("Connection: close");
    client.println("X-Content-Type-Options: nosniff");
    client.println("X-Frame-Options: DENY");
    client.println("X-XSS-Protection: 1; mode=block");
    client.println("Cache-Control: no-cache, no-store, must-revalidate");
    client.println("Pragma: no-cache");
    client.println("Expires: 0");
    client.println();
}

void GpsWebServer::send404(EthernetClient &client) {
    // 新しい実装では HttpResponseBuilder::send404 を使用
    client.println("HTTP/1.1 404 Not Found");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.println("<!DOCTYPE HTML>");
    client.println("<html><body><h1>404 Not Found</h1><p>The requested resource could not be found on this server.</p></body></html>");
}

// 残りのメソッドは新しい実装で置き換えられるため、スタブとして残す
void GpsWebServer::mainPage(EthernetClient &client, GpsSummaryData gpsSummaryData) {
    // スタブ - 新しい実装では FileRouter が処理
}

void GpsWebServer::configApiGet(EthernetClient &client) {
    // スタブ - 新しい実装では ApiRouter が処理
}

void GpsWebServer::configApiPost(EthernetClient &client, String postData) {
    // スタブ - 新しい実装では ApiRouter が処理
}

void GpsWebServer::configApiReset(EthernetClient &client) {
    // スタブ - 新しい実装では ApiRouter が処理
}

void GpsWebServer::gpsApiGet(EthernetClient &client) {
    // スタブ - 新しい実装では ApiRouter が処理
}

// 以下、その他のメソッドもすべてスタブ化
void GpsWebServer::configNetworkApiGet(EthernetClient &client) {}
void GpsWebServer::configNetworkApiPost(EthernetClient &client, String postData) {}
void GpsWebServer::configGnssApiGet(EthernetClient &client) {}
void GpsWebServer::configGnssApiPost(EthernetClient &client, String postData) {}
void GpsWebServer::configNtpApiGet(EthernetClient &client) {}
void GpsWebServer::configNtpApiPost(EthernetClient &client, String postData) {}
void GpsWebServer::configSystemApiGet(EthernetClient &client) {}
void GpsWebServer::configSystemApiPost(EthernetClient &client, String postData) {}
void GpsWebServer::configLogApiGet(EthernetClient &client) {}
void GpsWebServer::configLogApiPost(EthernetClient &client, String postData) {}
void GpsWebServer::statusApiGet(EthernetClient &client) {}
void GpsWebServer::systemRebootApiPost(EthernetClient &client) {}
void GpsWebServer::systemMetricsApiGet(EthernetClient &client) {}
void GpsWebServer::systemLogsApiGet(EthernetClient &client) {}
void GpsWebServer::metricsPage(EthernetClient &client) {}
void GpsWebServer::handleFileRequest(EthernetClient &client, const String &filepath, const String &contentType) {}
void GpsWebServer::sendJsonResponse(EthernetClient &client, const String &jsonResponse, int statusCode) {}
bool GpsWebServer::checkRequestRate(const String &clientIP) { return true; }
bool GpsWebServer::isValidJsonInput(const String &input) { return true; }
String GpsWebServer::sanitizeInput(const String &input) { return input; }
void GpsWebServer::debugFilesApiGet(EthernetClient &client) {}
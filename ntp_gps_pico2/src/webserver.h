#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <Ethernet.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
#include <Gps_model.h>


class NtpServer; // Forward declaration
class ConfigManager; // Forward declaration
class PrometheusMetrics; // Forward declaration

class WebServer
{
public:
    void handleClient(Stream &stream, EthernetServer &server, UBX_NAV_SAT_data_t *ubxNavSatData_t, GpsSummaryData gpsSummaryData);
    void setNtpServer(NtpServer* ntpServerInstance) { ntpServer = ntpServerInstance; }
    void setConfigManager(ConfigManager* configManagerInstance) { configManager = configManagerInstance; }
    void setPrometheusMetrics(PrometheusMetrics* prometheusMetricsInstance) { prometheusMetrics = prometheusMetricsInstance; }

private:
    NtpServer* ntpServer = nullptr;
    ConfigManager* configManager = nullptr;
    PrometheusMetrics* prometheusMetrics = nullptr;
    
    void rootPage(EthernetClient &client, GpsSummaryData gpsSummaryData);
    void gpsPage(EthernetClient &client, UBX_NAV_SAT_data_t *ubxNavSatData_t);
    void metricsPage(EthernetClient &client);
    void configPage(EthernetClient &client);
    void configApiGet(EthernetClient &client);
    void configApiPost(EthernetClient &client, String postData);
    void configApiReset(EthernetClient &client);
    void configFormPage(EthernetClient &client);
    void printHeader(EthernetClient &client, String contentType);
    bool parsePostData(const String& data, String& key, String& value);
    void sendJsonResponse(EthernetClient &client, const String& json, int statusCode = 200);

};
#endif
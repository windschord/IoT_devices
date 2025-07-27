#ifndef GPS_WEBSERVER_H
#define GPS_WEBSERVER_H

#include <Ethernet.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
#include "../gps/Gps_model.h"


class NtpServer; // Forward declaration
class ConfigManager; // Forward declaration
class PrometheusMetrics; // Forward declaration
class LoggingService; // Forward declaration
class GpsClient; // Forward declaration

class GpsWebServer
{
public:
    void handleClient(Stream &stream, EthernetServer &server, UBX_NAV_SAT_data_t *ubxNavSatData_t, GpsSummaryData gpsSummaryData);
    void setNtpServer(NtpServer* ntpServerInstance) { ntpServer = ntpServerInstance; }
    void setConfigManager(ConfigManager* configManagerInstance) { configManager = configManagerInstance; }
    void setPrometheusMetrics(PrometheusMetrics* prometheusMetricsInstance) { prometheusMetrics = prometheusMetricsInstance; }
    void setLoggingService(LoggingService* loggingServiceInstance) { loggingService = loggingServiceInstance; }
    void setGpsClient(GpsClient* gpsClientInstance) { gpsClient = gpsClientInstance; }

private:
    NtpServer* ntpServer = nullptr;
    ConfigManager* configManager = nullptr;
    PrometheusMetrics* prometheusMetrics = nullptr;
    LoggingService* loggingService = nullptr;
    GpsClient* gpsClient = nullptr;
    
    void rootPage(EthernetClient &client, GpsSummaryData gpsSummaryData);
    void gpsPage(EthernetClient &client, UBX_NAV_SAT_data_t *ubxNavSatData_t);
    void metricsPage(EthernetClient &client);
    void configPage(EthernetClient &client);
    void configApiGet(EthernetClient &client);
    void configApiPost(EthernetClient &client, String postData);
    void configApiReset(EthernetClient &client);
    void configFormPage(EthernetClient &client);
    void gpsApiGet(EthernetClient &client); // New GPS API endpoint
    void printHeader(EthernetClient &client, String contentType);
    bool parsePostData(const String& data, String& key, String& value);
    void sendJsonResponse(EthernetClient &client, const String& json, int statusCode = 200);

};
#endif
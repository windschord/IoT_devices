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
    
    // Performance monitoring and optimization methods
    void invalidateGpsCache() { gpsDataCacheValid = false; }
    unsigned long getRequestCount() const { return requestCount; }
    unsigned long getAverageResponseTime() const { 
        return requestCount > 0 ? totalResponseTime / requestCount : 0; 
    }

private:
    NtpServer* ntpServer = nullptr;
    ConfigManager* configManager = nullptr;
    PrometheusMetrics* prometheusMetrics = nullptr;
    LoggingService* loggingService = nullptr;
    GpsClient* gpsClient = nullptr;
    
    // Performance optimization: Cache mechanism
    unsigned long lastGpsDataUpdate = 0;
    static const unsigned long GPS_DATA_CACHE_INTERVAL = 2000; // 2 seconds cache
    String cachedGpsJson = "";
    bool gpsDataCacheValid = false;
    
    // Performance monitoring
    unsigned long requestCount = 0;
    unsigned long totalResponseTime = 0;
    
    void rootPage(EthernetClient &client, GpsSummaryData gpsSummaryData);
    // DEPRECATED: GPS display now served from LittleFS files
    // void gpsPage(EthernetClient &client, UBX_NAV_SAT_data_t *ubxNavSatData_t);
    void metricsPage(EthernetClient &client);
    void configPage(EthernetClient &client);
    void configApiGet(EthernetClient &client);
    void configApiPost(EthernetClient &client, String postData);
    void configApiReset(EthernetClient &client);
    void configFormPage(EthernetClient &client);
    void gpsApiGet(EthernetClient &client); // New GPS API endpoint
    
    // Basic configuration category API endpoints (家庭利用向け)
    void configNetworkApiGet(EthernetClient &client);
    void configNetworkApiPost(EthernetClient &client, String postData);
    void configGnssApiGet(EthernetClient &client);
    void configGnssApiPost(EthernetClient &client, String postData);
    void configNtpApiGet(EthernetClient &client);
    void configNtpApiPost(EthernetClient &client, String postData);
    void configSystemApiGet(EthernetClient &client);
    void configSystemApiPost(EthernetClient &client, String postData);
    void configLogApiGet(EthernetClient &client);
    void configLogApiPost(EthernetClient &client, String postData);
    
    // System status API for real-time monitoring
    void statusApiGet(EthernetClient &client);
    
    // Maintenance and diagnostics APIs
    void systemRebootApiPost(EthernetClient &client);
    void systemMetricsApiGet(EthernetClient &client);
    void systemLogsApiGet(EthernetClient &client);
    void printHeader(EthernetClient &client, String contentType);
    bool parsePostData(const String& data, String& key, String& value);
    void sendJsonResponse(EthernetClient &client, const String& json, int statusCode = 200);
    void send404(EthernetClient &client); // Add 404 handler
    void handleFileRequest(EthernetClient &client, const String& path, const String& contentType); // File system support

};
#endif
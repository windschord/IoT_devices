#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <Ethernet.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
#include <Gps_model.h>


class WebServer
{
public:
    void handleClient(Stream &stream, EthernetServer &server, UBX_NAV_SAT_data_t *ubxNavSatData_t, GpsSummaryData gpsSummaryData);

private:
    void rootPage(EthernetClient &client, GpsSummaryData gpsSummaryData);
    void gpsPage(EthernetClient &client, UBX_NAV_SAT_data_t *ubxNavSatData_t);
    void metricsPage(EthernetClient &client);
    void printHeader(EthernetClient &client, String contentType);

};
#endif
#include <webserver.h>
#include "NtpServer.h"
#include "ConfigManager.h"

void WebServer::handleClient(Stream &stream, EthernetServer &server, UBX_NAV_SAT_data_t *ubxNavSatData_t, GpsSummaryData gpsSummaryData)
{
  EthernetClient client = server.available();
  if (client)
  {
    stream.println("new client");
    String s;
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected())
    {
      if (client.available())
      {
        char c = client.read();
        s += c;
        if (c == '\n' && currentLineIsBlank)
        {
          break;
        }
        if (c == '\n')
        {
          // you're starting a new line
          currentLineIsBlank = true;
        }
        else if (c != '\r')
        {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }

    stream.print(s);

    if (s.indexOf("GET /gps ") >= 0)
    {
      stream.println("GPS");
      gpsPage(client, ubxNavSatData_t);
    }
    else if (s.indexOf("GET /metrics ") >= 0)
    {
      stream.println("METRICS");
      metricsPage(client);
    }
    else if (s.indexOf("GET /config ") >= 0)
    {
      stream.println("CONFIG");
      configPage(client);
    }
    else if (s.indexOf("GET /api/config ") >= 0)
    {
      stream.println("CONFIG_API_GET");
      configApiGet(client);
    }
    else if (s.indexOf("POST /api/config ") >= 0)
    {
      stream.println("CONFIG_API_POST");
      // Extract POST data from request
      String postData = "";
      // POST data parsing would be implemented here
      configApiPost(client, postData);
    }
    else
    {
      stream.println("ROOT");
      rootPage(client, gpsSummaryData);
    }

    stream.println("sending response");
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    stream.println("client disonnected");
  }
}

void WebServer::printHeader(EthernetClient &client, String contentType)
{
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: " + contentType);
  client.println("Connnection: close");
  client.println();
}

void WebServer::rootPage(EthernetClient &client, GpsSummaryData gpsSummaryData)
{
  printHeader(client, "text/html");

  char dateTimechr[20];
  sprintf(dateTimechr, "%04d/%02d/%02d %02d:%02d:%02d",
          gpsSummaryData.year, gpsSummaryData.month, gpsSummaryData.day,
          gpsSummaryData.hour, gpsSummaryData.min, gpsSummaryData.sec);

  char poschr[100];
  sprintf(poschr, "Lat: %7.4f Long:  %7.4f Height above MSL:  %6.2f m",
          gpsSummaryData.latitude / 10000000.0, gpsSummaryData.longitude / 10000000.0, gpsSummaryData.altitude / 1000.0);

  client.println("<!DOCTYPE HTML>");
  client.println("<html><body>");

  client.println("<h1>GPS NTP Server</h1>");
  client.println("<a href=\"/gps\">GPS Details</a> | ");
  client.println("<a href=\"/config\">Configuration</a> | ");

  client.println("<div>Date/Time: ");
  client.println(dateTimechr);
  client.println("</div>");

  if (gpsSummaryData.timeValid)
  {
    client.println("<div>Time is valid</div>");
  }
  else
  {
    client.println("<div>Time is invalid</div>");
  }

  if (gpsSummaryData.dateValid)
  {
    client.println("<div>Date is valid</div>");
  }
  else
  {
    client.println("<div>Date is invalid</div>");
  }

  client.println("<div>Position: ");
  client.println(poschr);
  client.println("</div>");

  client.println("<a href=\"/metrics\">Metrics</a>");
  client.println("</body></html>");
}

void WebServer::gpsPage(EthernetClient &client, UBX_NAV_SAT_data_t *ubxNavSatData_t)
{
  printHeader(client, "text/html");

  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.print("New NAV SAT data received. It contains data for SVs: ");
  client.print(ubxNavSatData_t->header.numSvs);
  client.println("<br>");

  // Just for giggles, print the signal strength for each SV as a barchart
  for (uint16_t block = 0; block < ubxNavSatData_t->header.numSvs; block++) // For each SV
  {
    switch (ubxNavSatData_t->blocks[block].gnssId) // Print the GNSS ID
    {
    case 0:
      client.print(F("GPS     "));
      break;
    case 1:
      client.print(F("SBAS    "));
      break;
    case 2:
      client.print(F("Galileo "));
      break;
    case 3:
      client.print(F("BeiDou  "));
      break;
    case 4:
      client.print(F("IMES    "));
      break;
    case 5:
      client.print(F("QZSS    "));
      break;
    case 6:
      client.print(F("GLONASS "));
      break;
    default:
      client.print(F("UNKNOWN "));
      break;
    }

    client.print(ubxNavSatData_t->blocks[block].svId); // Print the SV ID

    if (ubxNavSatData_t->blocks[block].svId < 10)
    {
      client.print(F("   "));
    }
    else if (ubxNavSatData_t->blocks[block].svId < 100)
    {
      client.print(F("  "));
    }
    else
    {
      client.print(F(" "));
    }

    client.print(ubxNavSatData_t->blocks[block].cno);
    client.print("<br>");
  }
  client.println("</body></html>");
}

void WebServer::metricsPage(EthernetClient &client)
{
  printHeader(client, "text/plain");
  
  // System metrics
  client.println("# HELP system_uptime_seconds System uptime in seconds");
  client.println("# TYPE system_uptime_seconds counter");
  client.print("system_uptime_seconds ");
  client.println(millis() / 1000);
  
  client.println("# HELP memory_free_bytes Free memory in bytes"); 
  client.println("# TYPE memory_free_bytes gauge");
  client.print("memory_free_bytes ");
  client.println(524288 - 16880); // Approximate calculation
  
  client.println("# HELP network_connected Network connection status");
  client.println("# TYPE network_connected gauge");
  client.print("network_connected ");
  client.println("1"); // Active if serving this page
  
  // NTP Server metrics
  if (ntpServer) {
    const auto& stats = ntpServer->getStatistics();
    
    client.println("# HELP ntp_requests_total Total number of NTP requests received");
    client.println("# TYPE ntp_requests_total counter");
    client.print("ntp_requests_total ");
    client.println(stats.requests_total);
    
    client.println("# HELP ntp_requests_valid Valid NTP requests processed");
    client.println("# TYPE ntp_requests_valid counter");
    client.print("ntp_requests_valid ");
    client.println(stats.requests_valid);
    
    client.println("# HELP ntp_requests_invalid Invalid NTP requests rejected");
    client.println("# TYPE ntp_requests_invalid counter");
    client.print("ntp_requests_invalid ");
    client.println(stats.requests_invalid);
    
    client.println("# HELP ntp_responses_sent NTP responses successfully sent");
    client.println("# TYPE ntp_responses_sent counter");
    client.print("ntp_responses_sent ");
    client.println(stats.responses_sent);
    
    client.println("# HELP ntp_processing_time_avg_ms Average NTP request processing time in milliseconds");
    client.println("# TYPE ntp_processing_time_avg_ms gauge");
    client.print("ntp_processing_time_avg_ms ");
    client.println(stats.avg_processing_time, 3);
    
    client.println("# HELP ntp_last_request_time_seconds Time since last NTP request in seconds");
    client.println("# TYPE ntp_last_request_time_seconds gauge");
    if (stats.last_request_time > 0) {
      client.print("ntp_last_request_time_seconds ");
      client.println((millis() - stats.last_request_time) / 1000);
    } else {
      client.println("ntp_last_request_time_seconds 0");
    }
  } else {
    client.println("# NTP Server not initialized");
    client.println("ntp_requests_total 0");
    client.println("ntp_requests_valid 0");
    client.println("ntp_requests_invalid 0");
    client.println("ntp_responses_sent 0");
    client.println("ntp_processing_time_avg_ms 0");
    client.println("ntp_last_request_time_seconds 0");
  }
}

void WebServer::configPage(EthernetClient &client) {
  printHeader(client, "text/html");
  
  client.println("<!DOCTYPE HTML>");
  client.println("<html><head>");
  client.println("<title>GPS NTP Server Configuration</title>");
  client.println("<style>");
  client.println("body { font-family: Arial, sans-serif; margin: 20px; }");
  client.println("table { border-collapse: collapse; width: 100%; }");
  client.println("th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }");
  client.println("th { background-color: #f2f2f2; }");
  client.println(".nav { margin-bottom: 20px; }");
  client.println(".nav a { margin-right: 10px; }");
  client.println("</style>");
  client.println("</head><body>");
  
  // Navigation
  client.println("<div class=\"nav\">");
  client.println("<a href=\"/\">Home</a>");
  client.println("<a href=\"/gps\">GPS</a>");
  client.println("<a href=\"/metrics\">Metrics</a>");
  client.println("<a href=\"/config\">Configuration</a>");
  client.println("</div>");
  
  client.println("<h1>System Configuration</h1>");
  
  if (configManager) {
    const auto& config = configManager->getConfig();
    
    client.println("<h2>Network Settings</h2>");
    client.println("<table>");
    client.println("<tr><th>Parameter</th><th>Value</th></tr>");
    client.printf("<tr><td>Hostname</td><td>%s</td></tr>", config.hostname);
    client.printf("<tr><td>IP Address</td><td>%s</td></tr>", 
                  config.ip_address == 0 ? "DHCP" : "Static");
    client.printf("<tr><td>Syslog Server</td><td>%s:%d</td></tr>", 
                  config.syslog_server, config.syslog_port);
    client.printf("<tr><td>Log Level</td><td>%d</td></tr>", config.log_level);
    client.println("</table>");
    
    client.println("<h2>GNSS Settings</h2>");
    client.println("<table>");
    client.println("<tr><th>Constellation</th><th>Status</th></tr>");
    client.printf("<tr><td>GPS</td><td>%s</td></tr>", config.gps_enabled ? "Enabled" : "Disabled");
    client.printf("<tr><td>GLONASS</td><td>%s</td></tr>", config.glonass_enabled ? "Enabled" : "Disabled");
    client.printf("<tr><td>Galileo</td><td>%s</td></tr>", config.galileo_enabled ? "Enabled" : "Disabled");
    client.printf("<tr><td>BeiDou</td><td>%s</td></tr>", config.beidou_enabled ? "Enabled" : "Disabled");
    client.printf("<tr><td>QZSS</td><td>%s</td></tr>", config.qzss_enabled ? "Enabled" : "Disabled");
    client.printf("<tr><td>QZSS L1S</td><td>%s</td></tr>", config.qzss_l1s_enabled ? "Enabled" : "Disabled");
    client.printf("<tr><td>Update Rate</td><td>%d Hz</td></tr>", config.gnss_update_rate);
    client.println("</table>");
    
    client.println("<h2>NTP Server Settings</h2>");
    client.println("<table>");
    client.printf("<tr><td>NTP Enabled</td><td>%s</td></tr>", config.ntp_enabled ? "Yes" : "No");
    client.printf("<tr><td>NTP Port</td><td>%d</td></tr>", config.ntp_port);
    client.printf("<tr><td>Stratum</td><td>%d</td></tr>", config.ntp_stratum);
    client.println("</table>");
    
    client.println("<h2>Actions</h2>");
    client.println("<p><a href=\"/api/config\">View JSON Configuration</a></p>");
    client.println("<p><strong>Note:</strong> Configuration editing via web interface will be available in future versions.</p>");
  } else {
    client.println("<p>Configuration Manager not available</p>");
  }
  
  client.println("</body></html>");
}

void WebServer::configApiGet(EthernetClient &client) {
  if (configManager) {
    String configJson = configManager->configToJson();
    sendJsonResponse(client, configJson);
  } else {
    sendJsonResponse(client, "{\"error\": \"Configuration Manager not available\"}", 500);
  }
}

void WebServer::configApiPost(EthernetClient &client, String postData) {
  if (!configManager) {
    sendJsonResponse(client, "{\"error\": \"Configuration Manager not available\"}", 500);
    return;
  }
  
  if (postData.length() == 0) {
    sendJsonResponse(client, "{\"error\": \"No POST data received\"}", 400);
    return;
  }
  
  if (configManager->configFromJson(postData)) {
    sendJsonResponse(client, "{\"success\": true, \"message\": \"Configuration updated successfully\"}");
  } else {
    sendJsonResponse(client, "{\"error\": \"Configuration validation failed\"}", 400);
  }
}

void WebServer::sendJsonResponse(EthernetClient &client, const String& json, int statusCode) {
  client.printf("HTTP/1.1 %d %s\r\n", statusCode, statusCode == 200 ? "OK" : "Error");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  client.println(json);
}
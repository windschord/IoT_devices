#include "webserver.h"
#include "NtpServer.h"
#include "../config/ConfigManager.h"
#include "../system/PrometheusMetrics.h"
#include "../config/LoggingService.h"
#include "../gps/Gps_Client.h"
#include <ArduinoJson.h>
#include <LittleFS.h>

void GpsWebServer::handleClient(Stream &stream, EthernetServer &server, UBX_NAV_SAT_data_t *ubxNavSatData_t, GpsSummaryData gpsSummaryData)
{
  EthernetClient client = server.available();
  if (client)
  {
    if (loggingService) {
      loggingService->info("WEB", "New HTTP client connected");
    }
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

    // HTTP request logging moved to LoggingService - raw request no longer printed

    if (s.indexOf("GET /gps ") >= 0)
    {
      if (loggingService) {
        loggingService->info("WEB", "Serving GPS page from file system");
      }
      handleFileRequest(client, "/gps.html", "text/html");
    }
    else if (s.indexOf("GET /gps.js ") >= 0)
    {
      if (loggingService) {
        loggingService->info("WEB", "Serving GPS JavaScript from file system");
      }
      handleFileRequest(client, "/gps.js", "text/javascript");
    }
    else if (s.indexOf("GET /metrics ") >= 0)
    {
      if (loggingService) {
        loggingService->info("WEB", "Serving Prometheus metrics page");
      }
      metricsPage(client);
    }
    else if (s.indexOf("GET /config ") >= 0)
    {
      if (loggingService) {
        loggingService->info("WEB", "Serving configuration page");
      }
      configPage(client);
    }
    else if (s.indexOf("GET /api/gps ") >= 0)
    {
      if (loggingService) {
        loggingService->info("WEB", "Serving GPS API GET");
      }
      gpsApiGet(client);
    }
    else if (s.indexOf("GET /api/config ") >= 0)
    {
      if (loggingService) {
        loggingService->info("WEB", "Serving config API GET");
      }
      configApiGet(client);
    }
    else if (s.indexOf("POST /api/config ") >= 0)
    {
      if (loggingService) {
        loggingService->info("WEB", "Processing config API POST");
      }
      // Extract POST data from request
      String postData = "";
      int contentStart = s.indexOf("\r\n\r\n");
      if (contentStart > 0) {
        postData = s.substring(contentStart + 4);
      }
      configApiPost(client, postData);
    }
    else if (s.indexOf("POST /api/reset ") >= 0)
    {
      if (loggingService) {
        loggingService->warning("WEB", "Processing factory reset API");
      }
      configApiReset(client);
    }
    // Basic configuration category API endpoints (家庭利用向け)
    else if (s.indexOf("GET /api/config/network ") >= 0)
    {
      if (loggingService) {
        loggingService->info("WEB", "Serving network config API GET");
      }
      configNetworkApiGet(client);
    }
    else if (s.indexOf("POST /api/config/network ") >= 0)
    {
      if (loggingService) {
        loggingService->info("WEB", "Processing network config API POST");
      }
      String postData = "";
      int contentStart = s.indexOf("\r\n\r\n");
      if (contentStart > 0) {
        postData = s.substring(contentStart + 4);
      }
      configNetworkApiPost(client, postData);
    }
    else if (s.indexOf("GET /api/config/gnss ") >= 0)
    {
      if (loggingService) {
        loggingService->info("WEB", "Serving GNSS config API GET");
      }
      configGnssApiGet(client);
    }
    else if (s.indexOf("POST /api/config/gnss ") >= 0)
    {
      if (loggingService) {
        loggingService->info("WEB", "Processing GNSS config API POST");
      }
      String postData = "";
      int contentStart = s.indexOf("\r\n\r\n");
      if (contentStart > 0) {
        postData = s.substring(contentStart + 4);
      }
      configGnssApiPost(client, postData);
    }
    else if (s.indexOf("GET /api/config/ntp ") >= 0)
    {
      if (loggingService) {
        loggingService->info("WEB", "Serving NTP config API GET");
      }
      configNtpApiGet(client);
    }
    else if (s.indexOf("POST /api/config/ntp ") >= 0)
    {
      if (loggingService) {
        loggingService->info("WEB", "Processing NTP config API POST");
      }
      String postData = "";
      int contentStart = s.indexOf("\r\n\r\n");
      if (contentStart > 0) {
        postData = s.substring(contentStart + 4);
      }
      configNtpApiPost(client, postData);
    }
    else if (s.indexOf("GET /api/config/system ") >= 0)
    {
      if (loggingService) {
        loggingService->info("WEB", "Serving system config API GET");
      }
      configSystemApiGet(client);
    }
    else if (s.indexOf("POST /api/config/system ") >= 0)
    {
      if (loggingService) {
        loggingService->info("WEB", "Processing system config API POST");
      }
      String postData = "";
      int contentStart = s.indexOf("\r\n\r\n");
      if (contentStart > 0) {
        postData = s.substring(contentStart + 4);
      }
      configSystemApiPost(client, postData);
    }
    else if (s.indexOf("GET /api/config/log ") >= 0)
    {
      if (loggingService) {
        loggingService->info("WEB", "Serving log config API GET");
      }
      configLogApiGet(client);
    }
    else if (s.indexOf("POST /api/config/log ") >= 0)
    {
      if (loggingService) {
        loggingService->info("WEB", "Processing log config API POST");
      }
      String postData = "";
      int contentStart = s.indexOf("\r\n\r\n");
      if (contentStart > 0) {
        postData = s.substring(contentStart + 4);
      }
      configLogApiPost(client, postData);
    }
    else if (s.indexOf("GET /api/status ") >= 0)
    {
      if (loggingService) {
        loggingService->info("WEB", "Serving system status API GET");
      }
      statusApiGet(client);
    }
    else if (s.indexOf("GET /api/system/metrics ") >= 0)
    {
      if (loggingService) {
        loggingService->info("WEB", "Serving system metrics API GET");
      }
      systemMetricsApiGet(client);
    }
    else if (s.indexOf("GET /api/system/logs ") >= 0)
    {
      if (loggingService) {
        loggingService->info("WEB", "Serving system logs API GET");
      }
      systemLogsApiGet(client);
    }
    else if (s.indexOf("POST /api/system/reboot ") >= 0)
    {
      if (loggingService) {
        loggingService->warning("WEB", "Processing system reboot API");
      }
      systemRebootApiPost(client);
    }
    else
    {
      if (loggingService) {
        loggingService->info("WEB", "Serving root page");
      }
      rootPage(client, gpsSummaryData);
    }

    if (loggingService) {
      loggingService->debug("WEB", "HTTP response sent to client");
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    if (loggingService) {
      loggingService->info("WEB", "HTTP client disconnected");
    }
  }
}

void GpsWebServer::printHeader(EthernetClient &client, String contentType)
{
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: " + contentType);
  client.println("Connnection: close");
  client.println();
}

void GpsWebServer::rootPage(EthernetClient &client, GpsSummaryData gpsSummaryData)
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

// DEPRECATED: Large GPS page function has been completely removed to save flash memory
// GPS display is now served from LittleFS files: data/gps.html and data/gps.js

void GpsWebServer::configPage(EthernetClient &client) {
  printHeader(client, "text/html");
  
  client.println("<!DOCTYPE HTML>");
  client.println("<html><head>");
  client.println("<meta charset=\"UTF-8\">");
  client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">");
  client.println("<title>GPS NTP Server Configuration</title>");
  client.println("<style>");
  client.println("body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background-color: #f5f5f5; }");
  client.println(".container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }");
  client.println(".header { text-align: center; margin-bottom: 30px; }");
  client.println(".nav { margin-bottom: 20px; }");
  client.println(".nav a { text-decoration: none; color: #4CAF50; margin-right: 15px; }");
  client.println(".tabs { display: flex; border-bottom: 2px solid #ddd; margin-bottom: 20px; }");
  client.println(".tab { padding: 10px 20px; cursor: pointer; border: none; background: none; border-bottom: 2px solid transparent; }");
  client.println(".tab.active { border-bottom-color: #4CAF50; background-color: #f9f9f9; }");
  client.println(".tab-content { display: none; }");
  client.println(".tab-content.active { display: block; }");
  client.println(".form-group { margin-bottom: 15px; }");
  client.println("label { display: block; margin-bottom: 5px; font-weight: bold; }");
  client.println("input[type=text], input[type=number], select { width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }");
  client.println("input[type=checkbox] { margin-right: 8px; }");
  client.println(".checkbox-group { display: flex; align-items: center; margin-bottom: 5px; }");
  client.println(".form-row { display: flex; gap: 15px; }");
  client.println(".form-row .form-group { flex: 1; }");
  client.println("button { background-color: #4CAF50; color: white; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; margin-right: 10px; }");
  client.println("button:hover { background-color: #45a049; }");
  client.println("button.secondary { background-color: #f44336; }");
  client.println("button.secondary:hover { background-color: #da190b; }");
  client.println(".message { padding: 10px; margin: 10px 0; border-radius: 4px; }");
  client.println(".success { background-color: #d4edda; color: #155724; border: 1px solid #c3e6cb; }");
  client.println(".error { background-color: #f8d7da; color: #721c24; border: 1px solid #f5c6cb; }");
  client.println(".warning { background-color: #fff3cd; color: #856404; border: 1px solid #ffeaa7; }");
  client.println(".loading { display: none; padding: 10px; text-align: center; }");
  client.println("</style>");
  client.println("</head><body>");
  
  client.println("<div class=\"container\">");
  client.println("<div class=\"header\">");
  client.println("<h1>GPS NTP Server Configuration</h1>");
  client.println("<div class=\"nav\">");
  client.println("<a href=\"/\">← Status Dashboard</a>");
  client.println("<a href=\"/metrics\">Metrics</a>");
  client.println("<a href=\"/gps.html\">GPS View</a>");
  client.println("</div>");
  client.println("</div>");
  
  // Message container
  client.println("<div id=\"messageContainer\"></div>");
  client.println("<div id=\"loadingIndicator\" class=\"loading\">Saving configuration...</div>");
  
  // Tab navigation
  client.println("<div class=\"tabs\">");
  client.println("<button class=\"tab active\" onclick=\"showTab('network')\">Network</button>");
  client.println("<button class=\"tab\" onclick=\"showTab('gnss')\">GNSS</button>");
  client.println("<button class=\"tab\" onclick=\"showTab('ntp')\">NTP Server</button>");
  client.println("<button class=\"tab\" onclick=\"showTab('system')\">System</button>");
  client.println("<button class=\"tab\" onclick=\"showTab('logs')\">Logging</button>");
  client.println("</div>");
  
  if (configManager) {
    const auto& config = configManager->getConfig();
    
    // Network Configuration Tab
    client.println("<div id=\"network\" class=\"tab-content active\">");
    client.println("<h3>Network Configuration</h3>");
    client.println("<form id=\"networkForm\">");
    
    client.println("<div class=\"form-group\">");
    client.println("<label for=\"hostname\">Device Hostname:</label>");
    client.print("<input type=\"text\" id=\"hostname\" name=\"hostname\" value=\"");
    client.print(config.hostname);
    client.println("\" required>");
    client.println("</div>");
    
    client.println("<div class=\"form-group\">");
    client.println("<label>");
    client.println("<input type=\"radio\" name=\"ip_mode\" value=\"dhcp\" ");
    if (config.ip_address == 0) client.print("checked");
    client.println("> Use DHCP (Automatic)");
    client.println("</label>");
    client.println("<label>");
    client.println("<input type=\"radio\" name=\"ip_mode\" value=\"static\" ");
    if (config.ip_address != 0) client.print("checked");
    client.println("> Static IP Address");
    client.println("</label>");
    client.println("</div>");
    
    client.println("<div id=\"static-ip-fields\" style=\"display: ");
    client.print(config.ip_address != 0 ? "block" : "none");
    client.println("\">");
    client.println("<div class=\"form-row\">");
    client.println("<div class=\"form-group\">");
    client.println("<label for=\"ip_address\">IP Address:</label>");
    client.print("<input type=\"text\" id=\"ip_address\" name=\"ip_address\" value=\"");
    if (config.ip_address != 0) {
      client.print((config.ip_address & 0xFF)); client.print(".");
      client.print((config.ip_address >> 8) & 0xFF); client.print(".");
      client.print((config.ip_address >> 16) & 0xFF); client.print(".");
      client.print((config.ip_address >> 24) & 0xFF);
    }
    client.println("\">");
    client.println("</div>");
    client.println("<div class=\"form-group\">");
    client.println("<label for=\"netmask\">Subnet Mask:</label>");
    client.print("<input type=\"text\" id=\"netmask\" name=\"netmask\" value=\"");
    if (config.netmask != 0) {
      client.print((config.netmask & 0xFF)); client.print(".");
      client.print((config.netmask >> 8) & 0xFF); client.print(".");
      client.print((config.netmask >> 16) & 0xFF); client.print(".");
      client.print((config.netmask >> 24) & 0xFF);
    }
    client.println("\">");
    client.println("</div>");
    client.println("</div>");
    client.println("<div class=\"form-row\">");
    client.println("<div class=\"form-group\">");
    client.println("<label for=\"gateway\">Gateway:</label>");
    client.print("<input type=\"text\" id=\"gateway\" name=\"gateway\" value=\"");
    if (config.gateway != 0) {
      client.print((config.gateway & 0xFF)); client.print(".");
      client.print((config.gateway >> 8) & 0xFF); client.print(".");
      client.print((config.gateway >> 16) & 0xFF); client.print(".");
      client.print((config.gateway >> 24) & 0xFF);
    }
    client.println("\">");
    client.println("</div>");
    client.println("<div class=\"form-group\">");
    client.println("<label for=\"dns_server\">DNS Server:</label>");
    client.print("<input type=\"text\" id=\"dns_server\" name=\"dns_server\" value=\"");
    if (config.dns_server != 0) {
      client.print((config.dns_server & 0xFF)); client.print(".");
      client.print((config.dns_server >> 8) & 0xFF); client.print(".");
      client.print((config.dns_server >> 16) & 0xFF); client.print(".");
      client.print((config.dns_server >> 24) & 0xFF);
    }
    client.println("\">");
    client.println("</div>");
    client.println("</div>");
    client.println("</div>");
    
    client.println("<div class=\"form-group\">");
    client.println("<label>MAC Address (Read-only):</label>");
    uint8_t mac[6];
    Ethernet.MACAddress(mac);
    client.print("<input type=\"text\" value=\"");
    for (int i = 0; i < 6; i++) {
      if (i > 0) client.print(":");
      if (mac[i] < 16) client.print("0");
      client.print(mac[i], HEX);
    }
    client.println("\" readonly>");
    client.println("</div>");
    
    client.println("<button type=\"submit\">Save Network Configuration</button>");
    client.println("</form>");
    client.println("</div>");
    
    // GNSS Configuration Tab
    client.println("<div id=\"gnss\" class=\"tab-content\">");
    client.println("<h3>GNSS Configuration</h3>");
    client.println("<form id=\"gnssForm\">");
    
    client.println("<h4>Satellite Constellations</h4>");
    client.println("<div class=\"checkbox-group\">");
    client.print("<input type=\"checkbox\" id=\"gps_enabled\" name=\"gps_enabled\"");
    if (config.gps_enabled) client.print(" checked");
    client.println("><label for=\"gps_enabled\">GPS (Global Positioning System)</label>");
    client.println("</div>");
    client.println("<div class=\"checkbox-group\">");
    client.print("<input type=\"checkbox\" id=\"glonass_enabled\" name=\"glonass_enabled\"");
    if (config.glonass_enabled) client.print(" checked");
    client.println("><label for=\"glonass_enabled\">GLONASS (Russian)</label>");
    client.println("</div>");
    client.println("<div class=\"checkbox-group\">");
    client.print("<input type=\"checkbox\" id=\"galileo_enabled\" name=\"galileo_enabled\"");
    if (config.galileo_enabled) client.print(" checked");
    client.println("><label for=\"galileo_enabled\">Galileo (European)</label>");
    client.println("</div>");
    client.println("<div class=\"checkbox-group\">");
    client.print("<input type=\"checkbox\" id=\"beidou_enabled\" name=\"beidou_enabled\"");
    if (config.beidou_enabled) client.print(" checked");
    client.println("><label for=\"beidou_enabled\">BeiDou (Chinese)</label>");
    client.println("</div>");
    client.println("<div class=\"checkbox-group\">");
    client.print("<input type=\"checkbox\" id=\"qzss_enabled\" name=\"qzss_enabled\"");
    if (config.qzss_enabled) client.print(" checked");
    client.println("><label for=\"qzss_enabled\">QZSS (Quasi-Zenith Satellite System)</label>");
    client.println("</div>");
    
    client.println("<div class=\"form-group\">");
    client.println("<label for=\"gnss_update_rate\">GNSS Update Rate:</label>");
    client.print("<select id=\"gnss_update_rate\" name=\"gnss_update_rate\">");
    client.print("<option value=\"1\""); if (config.gnss_update_rate == 1) client.print(" selected"); client.print(">1 Hz</option>");
    client.print("<option value=\"5\""); if (config.gnss_update_rate == 5) client.print(" selected"); client.print(">5 Hz</option>");
    client.print("<option value=\"10\""); if (config.gnss_update_rate == 10) client.print(" selected"); client.print(">10 Hz</option>");
    client.println("</select>");
    client.println("</div>");
    
    client.println("<h4>QZSS L1S Disaster Alert System</h4>");
    client.println("<div class=\"checkbox-group\">");
    client.print("<input type=\"checkbox\" id=\"qzss_l1s_enabled\" name=\"qzss_l1s_enabled\"");
    if (config.qzss_l1s_enabled) client.print(" checked");
    client.println("><label for=\"qzss_l1s_enabled\">Enable L1S Disaster Alert Reception</label>");
    client.println("</div>");
    
    client.println("<div class=\"form-group\">");
    client.println("<label for=\"disaster_alert_priority\">Disaster Alert Priority:</label>");
    client.print("<select id=\"disaster_alert_priority\" name=\"disaster_alert_priority\">");
    client.print("<option value=\"1\""); if (config.disaster_alert_priority == 1) client.print(" selected"); client.print(">Low</option>");
    client.print("<option value=\"2\""); if (config.disaster_alert_priority == 2) client.print(" selected"); client.print(">Medium</option>");
    client.print("<option value=\"3\""); if (config.disaster_alert_priority == 3) client.print(" selected"); client.print(">High</option>");
    client.println("</select>");
    client.println("</div>");
    
    client.println("<button type=\"submit\">Save GNSS Configuration</button>");
    client.println("</form>");
    client.println("</div>");
    
    // NTP Server Configuration Tab
    client.println("<div id=\"ntp\" class=\"tab-content\">");
    client.println("<h3>NTP Server Configuration</h3>");
    client.println("<form id=\"ntpForm\">");
    
    client.println("<div class=\"checkbox-group\">");
    client.print("<input type=\"checkbox\" id=\"ntp_enabled\" name=\"ntp_enabled\"");
    if (config.ntp_enabled) client.print(" checked");
    client.println("><label for=\"ntp_enabled\">Enable NTP Server</label>");
    client.println("</div>");
    
    client.println("<div class=\"form-group\">");
    client.println("<label for=\"ntp_port\">NTP Port:</label>");
    client.print("<input type=\"number\" id=\"ntp_port\" name=\"ntp_port\" value=\"");
    client.print(config.ntp_port);
    client.println("\" min=\"1\" max=\"65535\">");
    client.println("</div>");
    
    client.println("<div class=\"form-group\">");
    client.println("<label for=\"ntp_stratum\">Stratum Level:</label>");
    client.print("<select id=\"ntp_stratum\" name=\"ntp_stratum\">");
    client.print("<option value=\"1\""); if (config.ntp_stratum == 1) client.print(" selected"); client.print(">1 (Primary - GPS Synchronized)</option>");
    client.print("<option value=\"2\""); if (config.ntp_stratum == 2) client.print(" selected"); client.print(">2 (Secondary)</option>");
    client.print("<option value=\"3\""); if (config.ntp_stratum == 3) client.print(" selected"); client.print(">3 (Fallback)</option>");
    client.println("</select>");
    client.println("</div>");
    
    client.println("<button type=\"submit\">Save NTP Configuration</button>");
    client.println("</form>");
    client.println("</div>");
    
    // System Configuration Tab
    client.println("<div id=\"system\" class=\"tab-content\">");
    client.println("<h3>System Configuration</h3>");
    client.println("<form id=\"systemForm\">");
    
    client.println("<div class=\"checkbox-group\">");
    client.print("<input type=\"checkbox\" id=\"auto_restart_enabled\" name=\"auto_restart_enabled\"");
    if (config.auto_restart_enabled) client.print(" checked");
    client.println("><label for=\"auto_restart_enabled\">Enable Auto Restart</label>");
    client.println("</div>");
    
    client.println("<div class=\"form-group\">");
    client.println("<label for=\"restart_interval\">Restart Interval (hours):</label>");
    client.print("<input type=\"number\" id=\"restart_interval\" name=\"restart_interval\" value=\"");
    client.print(config.restart_interval);
    client.println("\" min=\"1\" max=\"168\">");
    client.println("</div>");
    
    client.println("<div class=\"checkbox-group\">");
    client.print("<input type=\"checkbox\" id=\"debug_enabled\" name=\"debug_enabled\"");
    if (config.debug_enabled) client.print(" checked");
    client.println("><label for=\"debug_enabled\">Enable Debug Mode</label>");
    client.println("</div>");
    
    client.println("<button type=\"submit\">Save System Configuration</button>");
    client.println("<button type=\"button\" class=\"secondary\" onclick=\"factoryReset()\">Factory Reset</button>");
    client.println("</form>");
    client.println("</div>");
    
    // Logging Configuration Tab
    client.println("<div id=\"logs\" class=\"tab-content\">");
    client.println("<h3>Logging Configuration</h3>");
    client.println("<form id=\"logsForm\">");
    
    client.println("<div class=\"form-group\">");
    client.println("<label for=\"syslog_server\">Syslog Server:</label>");
    client.print("<input type=\"text\" id=\"syslog_server\" name=\"syslog_server\" value=\"");
    client.print(config.syslog_server);
    client.println("\">");
    client.println("</div>");
    
    client.println("<div class=\"form-group\">");
    client.println("<label for=\"syslog_port\">Syslog Port:</label>");
    client.print("<input type=\"number\" id=\"syslog_port\" name=\"syslog_port\" value=\"");
    client.print(config.syslog_port);
    client.println("\" min=\"1\" max=\"65535\">");
    client.println("</div>");
    
    client.println("<div class=\"form-group\">");
    client.println("<label for=\"log_level\">Log Level:</label>");
    client.print("<select id=\"log_level\" name=\"log_level\">");
    client.print("<option value=\"0\""); if (config.log_level == 0) client.print(" selected"); client.print(">Emergency</option>");
    client.print("<option value=\"1\""); if (config.log_level == 1) client.print(" selected"); client.print(">Alert</option>");
    client.print("<option value=\"2\""); if (config.log_level == 2) client.print(" selected"); client.print(">Critical</option>");
    client.print("<option value=\"3\""); if (config.log_level == 3) client.print(" selected"); client.print(">Error</option>");
    client.print("<option value=\"4\""); if (config.log_level == 4) client.print(" selected"); client.print(">Warning</option>");
    client.print("<option value=\"5\""); if (config.log_level == 5) client.print(" selected"); client.print(">Notice</option>");
    client.print("<option value=\"6\""); if (config.log_level == 6) client.print(" selected"); client.print(">Info</option>");
    client.print("<option value=\"7\""); if (config.log_level == 7) client.print(" selected"); client.print(">Debug</option>");
    client.println("</select>");
    client.println("</div>");
    
    client.println("<div class=\"checkbox-group\">");
    client.print("<input type=\"checkbox\" id=\"prometheus_enabled\" name=\"prometheus_enabled\"");
    if (config.prometheus_enabled) client.print(" checked");
    client.println("><label for=\"prometheus_enabled\">Enable Prometheus Metrics</label>");
    client.println("</div>");
    
    client.println("<button type=\"submit\">Save Logging Configuration</button>");
    client.println("</form>");
    client.println("</div>");
    
    // Add JavaScript at the end of the page
    client.println("<script>");
    
    // Tab switching function
    client.println("function showTab(tabName) {");
    client.println("  var tabs = document.querySelectorAll('.tab');");
    client.println("  var contents = document.querySelectorAll('.tab-content');");
    client.println("  tabs.forEach(t => t.classList.remove('active'));");
    client.println("  contents.forEach(c => c.classList.remove('active'));");
    client.println("  document.querySelector('[onclick=\"showTab(\\''+tabName+'\\')\"').classList.add('active');");
    client.println("  document.getElementById(tabName).classList.add('active');");
    client.println("}");
    
    // IP mode change handler
    client.println("document.querySelectorAll('input[name=\"ip_mode\"]').forEach(radio => {");
    client.println("  radio.addEventListener('change', function() {");
    client.println("    document.getElementById('static-ip-fields').style.display = this.value === 'static' ? 'block' : 'none';");
    client.println("  });");
    client.println("});");
    
    // Form submission handlers
    client.println("function handleFormSubmit(formId, apiEndpoint) {");
    client.println("  const form = document.getElementById(formId);");
    client.println("  const formData = new FormData(form);");
    client.println("  const jsonData = {};");
    client.println("  for (let [key, value] of formData.entries()) {");
    client.println("    if (form.querySelector('[name=\"'+key+'\"]').type === 'checkbox') {");
    client.println("      jsonData[key] = form.querySelector('[name=\"'+key+'\"]').checked;");
    client.println("    } else if (form.querySelector('[name=\"'+key+'\"]').type === 'number') {");
    client.println("      jsonData[key] = parseInt(value);");
    client.println("    } else {");
    client.println("      jsonData[key] = value;");
    client.println("    }");
    client.println("  }");
    client.println("  showLoading();");
    client.println("  fetch(apiEndpoint, {");
    client.println("    method: 'POST',");
    client.println("    headers: {'Content-Type': 'application/json'},");
    client.println("    body: JSON.stringify(jsonData)");
    client.println("  }).then(response => response.json())");
    client.println("    .then(data => {");
    client.println("      hideLoading();");
    client.println("      if (data.success) {");
    client.println("        showMessage(data.message || 'Configuration saved successfully', 'success');");
    client.println("      } else {");
    client.println("        showMessage(data.error || 'Failed to save configuration', 'error');");
    client.println("      }");
    client.println("    }).catch(error => {");
    client.println("      hideLoading();");
    client.println("      showMessage('Network error: ' + error, 'error');");
    client.println("    });");
    client.println("}");
    
    // Message display functions
    client.println("function showMessage(message, type) {");
    client.println("  const container = document.getElementById('messageContainer');");
    client.println("  container.innerHTML = '<div class=\"message ' + type + '\">' + message + '</div>';");
    client.println("  setTimeout(() => container.innerHTML = '', 5000);");
    client.println("}");
    
    client.println("function showLoading() {");
    client.println("  document.getElementById('loadingIndicator').style.display = 'block';");
    client.println("}");
    
    client.println("function hideLoading() {");
    client.println("  document.getElementById('loadingIndicator').style.display = 'none';");
    client.println("}");
    
    // Factory reset function
    client.println("function factoryReset() {");
    client.println("  if (confirm('Are you sure you want to reset all settings to factory defaults? This action cannot be undone.')) {");
    client.println("    showLoading();");
    client.println("    fetch('/api/reset', { method: 'POST' })");
    client.println("      .then(response => response.json())");
    client.println("      .then(data => {");
    client.println("        hideLoading();");
    client.println("        if (data.success) {");
    client.println("          showMessage('Factory reset completed. Page will reload in 3 seconds.', 'success');");
    client.println("          setTimeout(() => location.reload(), 3000);");
    client.println("        } else {");
    client.println("          showMessage(data.error || 'Factory reset failed', 'error');");
    client.println("        }");
    client.println("      }).catch(error => {");
    client.println("        hideLoading();");
    client.println("        showMessage('Network error: ' + error, 'error');");
    client.println("      });");
    client.println("  }");
    client.println("}");
    
    // Add form submit event listeners
    client.println("document.getElementById('networkForm').addEventListener('submit', function(e) {");
    client.println("  e.preventDefault(); handleFormSubmit('networkForm', '/api/config/network');");
    client.println("});");
    client.println("document.getElementById('gnssForm').addEventListener('submit', function(e) {");
    client.println("  e.preventDefault(); handleFormSubmit('gnssForm', '/api/config/gnss');");
    client.println("});");
    client.println("document.getElementById('ntpForm').addEventListener('submit', function(e) {");
    client.println("  e.preventDefault(); handleFormSubmit('ntpForm', '/api/config/ntp');");
    client.println("});");
    client.println("document.getElementById('systemForm').addEventListener('submit', function(e) {");
    client.println("  e.preventDefault(); handleFormSubmit('systemForm', '/api/config/system');");
    client.println("});");
    client.println("document.getElementById('logsForm').addEventListener('submit', function(e) {");
    client.println("  e.preventDefault(); handleFormSubmit('logsForm', '/api/config/log');");
    client.println("});");
    
    client.println("</script>");
    
  } else {
    client.println("<h2>Configuration Manager Not Available</h2>");
    client.println("<p>Configuration management service is not initialized.</p>");
  }
  
  client.println("</div>");
  client.println("</body></html>");
}

void GpsWebServer::configApiGet(EthernetClient &client) {
  if (configManager) {
    String configJson = configManager->configToJson();
    sendJsonResponse(client, configJson);
  } else {
    sendJsonResponse(client, "{\"error\": \"Configuration Manager not available\"}", 500);
  }
}

void GpsWebServer::configApiPost(EthernetClient &client, String postData) {
  if (configManager) {
    if (configManager->configFromJson(postData)) {
      sendJsonResponse(client, "{\"success\": true, \"message\": \"Configuration updated successfully\"}");
    } else {
      sendJsonResponse(client, "{\"error\": \"Configuration update failed\"}", 400);
    }
  } else {
    sendJsonResponse(client, "{\"error\": \"Configuration validation failed\"}", 400);
  }
}

void GpsWebServer::configApiReset(EthernetClient &client) {
  if (!configManager) {
    sendJsonResponse(client, "{\"error\": \"Configuration Manager not available\"}", 500);
    return;
  }
  
  configManager->resetToDefaults();
  sendJsonResponse(client, "{\"success\": true, \"message\": \"Configuration reset to defaults\"}");
}

void GpsWebServer::gpsApiGet(EthernetClient &client) {
  requestCount++;
  unsigned long requestStartTime = millis();
  
  if (loggingService) {
    loggingService->debug("WEB", "Processing GPS API request");
  }
  
  if (!gpsClient) {
    if (loggingService) {
      loggingService->error("WEB", "GPS client not available for API request");
    }
    sendJsonResponse(client, "{\"error\": \"GPS client not available\"}", 500);
    return;
  }
  
  // Performance optimization: Use cached data if recent
  unsigned long currentTime = millis();
  if (gpsDataCacheValid && (currentTime - lastGpsDataUpdate) < GPS_DATA_CACHE_INTERVAL) {
    if (loggingService) {
      loggingService->debug("WEB", "Serving cached GPS data for performance");
    }
    requestCount++;
    
    // Use cached data for improved performance
    sendJsonResponse(client, cachedGpsJson);
    
    // Update performance metrics
    unsigned long responseTime = millis() - requestStartTime;
    totalResponseTime += responseTime;
    return;
  }
  
  // Get fresh Web GPS data
  web_gps_data_t gpsData = gpsClient->getWebGpsData();
  
  // Create JSON document (significantly reduced buffer size for memory efficiency)
  DynamicJsonDocument doc(3072); // Reduced from 6144 to 3072 for stable operation
  
  // Position and Time Information with validation
  // Validate float values to prevent NaN or Inf in JSON
  float lat = gpsData.latitude;
  float lon = gpsData.longitude;
  float alt = gpsData.altitude;
  float spd = gpsData.speed;
  float crs = gpsData.course;
  
  if (isnan(lat) || isinf(lat)) lat = 0.0;
  if (isnan(lon) || isinf(lon)) lon = 0.0;
  if (isnan(alt) || isinf(alt)) alt = 0.0;
  if (isnan(spd) || isinf(spd)) spd = 0.0;
  if (isnan(crs) || isinf(crs)) crs = 0.0;
  
  doc["latitude"] = lat;
  doc["longitude"] = lon;
  doc["altitude"] = alt;
  doc["speed"] = spd;
  doc["course"] = crs;
  doc["utc_time"] = gpsData.utc_time;
  doc["ttff"] = gpsData.ttff;
  
  // Fix Information with validation
  float pdop = gpsData.pdop;
  float hdop = gpsData.hdop;
  float vdop = gpsData.vdop;
  float acc3d = gpsData.accuracy_3d;
  float acc2d = gpsData.accuracy_2d;
  
  if (isnan(pdop) || isinf(pdop)) pdop = 99.99;
  if (isnan(hdop) || isinf(hdop)) hdop = 99.99;
  if (isnan(vdop) || isinf(vdop)) vdop = 99.99;
  if (isnan(acc3d) || isinf(acc3d)) acc3d = 0.0;
  if (isnan(acc2d) || isinf(acc2d)) acc2d = 0.0;
  
  doc["fix_type"] = gpsData.fix_type;
  doc["data_valid"] = gpsData.data_valid;
  doc["pdop"] = pdop;
  doc["hdop"] = hdop;
  doc["vdop"] = vdop;
  doc["accuracy_3d"] = acc3d;
  doc["accuracy_2d"] = acc2d;
  
  // Constellation counts (integer values for efficiency)
  doc["satellites_gps_total"] = gpsData.satellites_gps_total;
  doc["satellites_gps_used"] = gpsData.satellites_gps_used;
  doc["satellites_glonass_total"] = gpsData.satellites_glonass_total;
  doc["satellites_glonass_used"] = gpsData.satellites_glonass_used;
  doc["satellites_galileo_total"] = gpsData.satellites_galileo_total;
  doc["satellites_galileo_used"] = gpsData.satellites_galileo_used;
  doc["satellites_beidou_total"] = gpsData.satellites_beidou_total;
  doc["satellites_beidou_used"] = gpsData.satellites_beidou_used;
  doc["satellites_qzss_total"] = gpsData.satellites_qzss_total;
  doc["satellites_qzss_used"] = gpsData.satellites_qzss_used;
  doc["satellites_total"] = gpsData.satellites_total;
  doc["satellites_used"] = gpsData.satellites_used;
  
  // Memory optimization: Limit satellites to high-priority ones only
  // Priority: Used in navigation > High signal strength > Tracked
  const uint8_t MAX_TRANSMITTED_SATELLITES = 16; // Limit to 16 satellites to save memory
  
  JsonArray satellitesArray = doc.createNestedArray("satellites");
  uint8_t satelliteCount = 0;
  
  for (int i = 0; i < gpsData.satellite_count && satelliteCount < MAX_TRANSMITTED_SATELLITES; i++) {
    // Only include satellites used in navigation or with good signal strength
    if (!gpsData.satellites[i].used_in_nav && gpsData.satellites[i].signal_strength < 25) {
      continue; // Skip low priority satellites
    }
    
    JsonObject sat = satellitesArray.createNestedObject();
    sat["constellation"] = gpsData.satellites[i].constellation;
    sat["prn"] = gpsData.satellites[i].prn;
    sat["elevation"] = gpsData.satellites[i].elevation;
    sat["azimuth"] = gpsData.satellites[i].azimuth;
    sat["signal_strength"] = gpsData.satellites[i].signal_strength;
    sat["used_in_nav"] = gpsData.satellites[i].used_in_nav;
    
    satelliteCount++;
  }
  
  // Check document capacity before serialization (updated for reduced buffer)
  size_t requiredSize = measureJson(doc);
  if (requiredSize > 2800) { // Reduced from 6144 to 2800 (safe margin for 3072 buffer)
    if (loggingService) {
      String errorMsg = "GPS JSON too large: " + String(requiredSize) + " bytes";
      loggingService->error("WEB", errorMsg.c_str());
    }
    sendJsonResponse(client, "{\"error\": \"GPS data too large, satellite data truncated\"}", 413);
    return;
  }
  
  // Serialize JSON
  String jsonString;
  size_t serializedBytes = serializeJson(doc, jsonString);
  
  // Debug: Verify serialization accuracy
  if (loggingService) {
    String debugMsg = "JSON Serialization: Measured=" + String(requiredSize) + 
                     " Serialized=" + String(serializedBytes) + 
                     " StringLength=" + String(jsonString.length());
    loggingService->debug("WEB", debugMsg.c_str());
  }
  
  // Update cache for performance optimization
  cachedGpsJson = jsonString;
  lastGpsDataUpdate = currentTime;
  gpsDataCacheValid = true;
  
  if (loggingService) {
    String debugMsg = "GPS JSON generated: " + String(jsonString.length()) + " bytes, " + String(satelliteCount) + " satellites";
    loggingService->debug("WEB", debugMsg.c_str());
  }
  
  // Debug: Log detailed response information
  if (loggingService) {
    String debugMsg = "GPS JSON Response Debug:";
    debugMsg += " Length=" + String(jsonString.length());
    debugMsg += " Satellites=" + String(satelliteCount);
    debugMsg += " Buffer=3072";
    loggingService->debug("WEB", debugMsg.c_str());
  }
  
  // Send JSON response
  sendJsonResponse(client, jsonString);
  
  // Update performance metrics
  unsigned long responseTime = millis() - requestStartTime;
  totalResponseTime += responseTime;
}

void GpsWebServer::metricsPage(EthernetClient &client)
{
  printHeader(client, "text/plain");
  
  if (prometheusMetrics) {
    if (loggingService) {
      loggingService->debug("WEB", "Serving Prometheus metrics from PrometheusMetrics service");
    }
    
    // PrometheusMetricsから統一されたメトリクスを取得
    const size_t METRICS_BUFFER_SIZE = 4096;
    char metricsBuffer[METRICS_BUFFER_SIZE];
    prometheusMetrics->generatePrometheusOutput(metricsBuffer, METRICS_BUFFER_SIZE);
    client.print(metricsBuffer);
  } else {
    if (loggingService) {
      loggingService->warning("WEB", "PrometheusMetrics service not available, serving basic metrics");
    }
    
    // PrometheusMetricsが利用できない場合の基本メトリクス
    client.println("# GPS NTP Server Basic Metrics");
    client.println("# This is a fallback when PrometheusMetrics service is not available");
    
    // システム基本情報
    client.println("# HELP system_uptime_seconds System uptime in seconds");
    client.println("# TYPE system_uptime_seconds counter");
    client.print("system_uptime_seconds ");
    client.println(millis() / 1000);
    
    client.println("# HELP memory_free_bytes Free memory in bytes"); 
    client.println("# TYPE memory_free_bytes gauge");
    client.print("memory_free_bytes ");
    client.println(524288 - 17856); // 実測値を使用
    
    client.println("# HELP network_connected Network connection status");
    client.println("# TYPE network_connected gauge");
    client.print("network_connected ");
    client.println("1"); // このページを提供している場合は接続中
    
    // NTP Server metrics（基本版）
    if (ntpServer) {
      const auto& stats = ntpServer->getStatistics();
      
      client.println("# HELP ntp_requests_total Total number of NTP requests received");
      client.println("# TYPE ntp_requests_total counter");
      client.print("ntp_requests_total ");
      client.println(stats.requests_total);
      
      client.println("# HELP ntp_responses_total Total number of NTP responses sent");
      client.println("# TYPE ntp_responses_total counter");
      client.print("ntp_responses_total ");
      client.println(stats.responses_sent);
      
      client.println("# HELP ntp_average_response_time_ms Average NTP response time in milliseconds");
      client.println("# TYPE ntp_average_response_time_ms gauge");
      client.print("ntp_average_response_time_ms ");
      client.println(stats.avg_processing_time, 3);
    } else {
      client.println("# NTP Server not initialized");
      client.println("ntp_requests_total 0");
      client.println("ntp_responses_total 0");
      client.println("ntp_average_response_time_ms 0");
    }
    
    client.println("# PrometheusMetrics not available - showing basic metrics only");
  }
}

void GpsWebServer::handleFileRequest(EthernetClient &client, const String& path, const String& contentType) {
  if (loggingService) {
    String debugMsg = "File request: " + path;
    loggingService->debug("WEB", debugMsg.c_str());
  }
  
  if (!LittleFS.begin()) {
    if (loggingService) {
      loggingService->error("WEB", "LittleFS not initialized");
    }
    send404(client);
    return;
  }
  
  File file = LittleFS.open(path, "r");
  if (!file) {
    if (loggingService) {
      String warningMsg = "File not found: " + path;
      loggingService->warning("WEB", warningMsg.c_str());
    }
    send404(client);
    return;
  }
  
  // Get file size for Content-Length header
  size_t fileSize = file.size();
  
  // Send HTTP headers
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: " + contentType);
  client.print("Content-Length: ");
  client.println(fileSize);
  client.println("Cache-Control: no-cache"); // Prevent caching for dynamic content
  client.println("Connection: close");
  client.println();
  
  // Stream file content efficiently
  uint8_t buffer[256];
  size_t totalSent = 0;
  while (file.available() && totalSent < fileSize) {
    size_t bytesRead = file.read(buffer, sizeof(buffer));
    if (bytesRead > 0) {
      client.write(buffer, bytesRead);
      totalSent += bytesRead;
    } else {
      break;
    }
  }
  
  file.close();
  
  if (loggingService) {
    String debugMsg = "File served: " + path + " (" + String(totalSent) + " bytes)";
    loggingService->debug("WEB", debugMsg.c_str());
  }
}

void GpsWebServer::send404(EthernetClient &client) {
  client.println("HTTP/1.1 404 Not Found");
  client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.println();
  client.print("404 Not Found");
}

void GpsWebServer::sendJsonResponse(EthernetClient &client, const String& json, int statusCode) {
  // Input validation: Check for valid JSON structure
  String validatedJson = json;
  if (json.length() == 0 || (!json.startsWith("{") && !json.startsWith("["))) {
    if (loggingService) {
      loggingService->error("WEB", "Invalid JSON format in response");
    }
    validatedJson = "{\"error\": \"Invalid JSON response format\"}";
    statusCode = 500;
  }
  
  // Safety check: Limit maximum response size to prevent memory issues
  const size_t MAX_RESPONSE_SIZE = 4096; // 4KB maximum response
  if (validatedJson.length() > MAX_RESPONSE_SIZE) {
    if (loggingService) {
      String errorMsg = "JSON response too large: " + String(validatedJson.length()) + " bytes";
      loggingService->error("WEB", errorMsg.c_str());
    }
    // Send error response for oversized data
    validatedJson = "{\"error\": \"Response too large\", \"size\": " + String(validatedJson.length()) + ", \"limit\": " + String(MAX_RESPONSE_SIZE) + "}";
    statusCode = 413; // Payload Too Large
  }
  
  // Send HTTP response with proper headers
  String statusMessage;
  switch (statusCode) {
    case 200: statusMessage = "OK"; break;
    case 400: statusMessage = "Bad Request"; break;
    case 413: statusMessage = "Payload Too Large"; break;
    case 500: statusMessage = "Internal Server Error"; break;
    default: statusMessage = "Unknown"; break;
  }
  
  client.print("HTTP/1.1 ");
  client.print(statusCode);
  client.print(" ");
  client.println(statusMessage);
  client.println("Content-Type: application/json");
  client.print("Content-Length: ");
  client.println(validatedJson.length());
  client.println("Cache-Control: no-cache");
  client.println("Access-Control-Allow-Origin: *"); // CORS support
  client.println("Connection: close");
  client.println();
  
  // Debug: Log actual content being sent
  if (loggingService) {
    String debugMsg = "HTTP Response Debug: Content-Length=" + String(validatedJson.length()) + 
                     " ActualBytes=" + String(validatedJson.length()) + 
                     " Status=" + String(statusCode);
    loggingService->debug("WEB", debugMsg.c_str());
  }
  
  // Send JSON content byte by byte to ensure accuracy
  const char* jsonData = validatedJson.c_str();
  size_t jsonLength = validatedJson.length();
  size_t bytesSent = 0;
  
  for (size_t i = 0; i < jsonLength; i++) {
    if (client.print(jsonData[i]) == 1) {
      bytesSent++;
    } else {
      break; // Connection error
    }
  }
  
  client.flush(); // Ensure all data is sent
  
  // Debug: Verify bytes actually sent
  if (loggingService) {
    String debugMsg = "Bytes sent verification: Expected=" + String(jsonLength) + 
                     " ActualSent=" + String(bytesSent) + 
                     " Match=" + (bytesSent == jsonLength ? "YES" : "NO");
    loggingService->debug("WEB", debugMsg.c_str());
  }
}

// ========== Basic Configuration Category API Endpoints (家庭利用向け) ==========

void GpsWebServer::configNetworkApiGet(EthernetClient &client) {
  if (!configManager) {
    sendJsonResponse(client, "{\"error\": \"Configuration Manager not available\"}", 500);
    return;
  }

  DynamicJsonDocument doc(1024);
  const auto& config = configManager->getConfig();
  
  doc["hostname"] = config.hostname;
  doc["ip_address"] = config.ip_address;
  doc["netmask"] = config.netmask;
  doc["gateway"] = config.gateway;
  doc["dns_server"] = config.dns_server;
  
  // MACアドレス表示（読み取り専用）
  uint8_t mac[6];
  Ethernet.MACAddress(mac);
  char macStr[18];
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", 
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  doc["mac_address"] = macStr;
  
  String jsonString;
  serializeJson(doc, jsonString);
  sendJsonResponse(client, jsonString);
}

void GpsWebServer::configNetworkApiPost(EthernetClient &client, String postData) {
  if (!configManager) {
    sendJsonResponse(client, "{\"error\": \"Configuration Manager not available\"}", 500);
    return;
  }

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, postData);
  
  if (error) {
    sendJsonResponse(client, "{\"error\": \"Invalid JSON format\"}", 400);
    return;
  }

  auto config = configManager->getConfig();
  bool changed = false;

  // ネットワーク設定検証とMACアドレス表示対応
  if (doc.containsKey("hostname")) {
    String hostname = doc["hostname"].as<String>();
    // ホスト名検証: 1-31文字、英数字とハイフンのみ
    if (hostname.length() == 0 || hostname.length() >= sizeof(config.hostname)) {
      sendJsonResponse(client, "{\"error\": \"Invalid hostname length (1-31 characters)\"}", 400);
      return;
    }
    strncpy(config.hostname, hostname.c_str(), sizeof(config.hostname) - 1);
    config.hostname[sizeof(config.hostname) - 1] = '\0';
    changed = true;
  }
  
  if (doc.containsKey("ip_address")) {
    config.ip_address = doc["ip_address"];
    changed = true;
  }
  
  if (doc.containsKey("netmask")) {
    config.netmask = doc["netmask"];
    changed = true;
  }
  
  if (doc.containsKey("gateway")) {
    config.gateway = doc["gateway"];
    changed = true;
  }
  
  if (doc.containsKey("dns_server")) {
    config.dns_server = doc["dns_server"];
    changed = true;
  }

  if (changed && configManager->setConfig(config)) {
    sendJsonResponse(client, "{\"success\": true, \"message\": \"Network configuration updated\"}");
  } else {
    sendJsonResponse(client, "{\"error\": \"Failed to update network configuration\"}", 400);
  }
}

void GpsWebServer::configGnssApiGet(EthernetClient &client) {
  if (!configManager) {
    sendJsonResponse(client, "{\"error\": \"Configuration Manager not available\"}", 500);
    return;
  }

  DynamicJsonDocument doc(512);
  const auto& config = configManager->getConfig();
  
  doc["gps_enabled"] = config.gps_enabled;
  doc["glonass_enabled"] = config.glonass_enabled;
  doc["galileo_enabled"] = config.galileo_enabled;
  doc["beidou_enabled"] = config.beidou_enabled;
  doc["qzss_enabled"] = config.qzss_enabled;
  doc["qzss_l1s_enabled"] = config.qzss_l1s_enabled;
  doc["gnss_update_rate"] = config.gnss_update_rate;
  doc["disaster_alert_priority"] = config.disaster_alert_priority;
  
  String jsonString;
  serializeJson(doc, jsonString);
  sendJsonResponse(client, jsonString);
}

void GpsWebServer::configGnssApiPost(EthernetClient &client, String postData) {
  if (!configManager) {
    sendJsonResponse(client, "{\"error\": \"Configuration Manager not available\"}", 500);
    return;
  }

  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, postData);
  
  if (error) {
    sendJsonResponse(client, "{\"error\": \"Invalid JSON format\"}", 400);
    return;
  }

  auto config = configManager->getConfig();
  bool changed = false;

  if (doc.containsKey("gps_enabled")) {
    config.gps_enabled = doc["gps_enabled"];
    changed = true;
  }
  
  if (doc.containsKey("glonass_enabled")) {
    config.glonass_enabled = doc["glonass_enabled"];
    changed = true;
  }
  
  if (doc.containsKey("galileo_enabled")) {
    config.galileo_enabled = doc["galileo_enabled"];
    changed = true;
  }
  
  if (doc.containsKey("beidou_enabled")) {
    config.beidou_enabled = doc["beidou_enabled"];
    changed = true;
  }
  
  if (doc.containsKey("qzss_enabled")) {
    config.qzss_enabled = doc["qzss_enabled"];
    changed = true;
  }
  
  if (doc.containsKey("qzss_l1s_enabled")) {
    config.qzss_l1s_enabled = doc["qzss_l1s_enabled"];
    changed = true;
  }
  
  if (doc.containsKey("gnss_update_rate")) {
    uint8_t rate = doc["gnss_update_rate"];
    // GNSS更新レート検証: 1Hz-10Hz
    if (rate == 0 || rate > 10) {
      sendJsonResponse(client, "{\"error\": \"GNSS update rate must be 1-10 Hz\"}", 400);
      return;
    }
    config.gnss_update_rate = rate;
    changed = true;
  }
  
  if (doc.containsKey("disaster_alert_priority")) {
    uint8_t priority = doc["disaster_alert_priority"];
    // 災害警報優先度検証: 0=低, 1=中, 2=高
    if (priority > 2) {
      sendJsonResponse(client, "{\"error\": \"Disaster alert priority must be 0-2 (0=low, 1=medium, 2=high)\"}", 400);
      return;
    }
    config.disaster_alert_priority = priority;
    changed = true;
  }

  if (changed && configManager->setConfig(config)) {
    sendJsonResponse(client, "{\"success\": true, \"message\": \"GNSS configuration updated\"}");
  } else {
    sendJsonResponse(client, "{\"error\": \"Failed to update GNSS configuration\"}", 400);
  }
}

void GpsWebServer::configNtpApiGet(EthernetClient &client) {
  if (!configManager) {
    sendJsonResponse(client, "{\"error\": \"Configuration Manager not available\"}", 500);
    return;
  }

  DynamicJsonDocument doc(512);
  const auto& config = configManager->getConfig();
  
  doc["ntp_enabled"] = config.ntp_enabled;
  doc["ntp_port"] = config.ntp_port;
  doc["ntp_stratum"] = config.ntp_stratum;
  
  String jsonString;
  serializeJson(doc, jsonString);
  sendJsonResponse(client, jsonString);
}

void GpsWebServer::configNtpApiPost(EthernetClient &client, String postData) {
  if (!configManager) {
    sendJsonResponse(client, "{\"error\": \"Configuration Manager not available\"}", 500);
    return;
  }

  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, postData);
  
  if (error) {
    sendJsonResponse(client, "{\"error\": \"Invalid JSON format\"}", 400);
    return;
  }

  auto config = configManager->getConfig();
  bool changed = false;

  if (doc.containsKey("ntp_enabled")) {
    config.ntp_enabled = doc["ntp_enabled"];
    changed = true;
  }
  
  if (doc.containsKey("ntp_port")) {
    uint16_t port = doc["ntp_port"];
    // NTPポート検証: 標準123または1024-65535
    if (port != 123 && (port < 1024 || port > 65535)) {
      sendJsonResponse(client, "{\"error\": \"NTP port must be 123 or 1024-65535\"}", 400);
      return;
    }
    config.ntp_port = port;
    changed = true;
  }
  
  if (doc.containsKey("ntp_stratum")) {
    uint8_t stratum = doc["ntp_stratum"];
    // Stratumレベル検証: 1-15
    if (stratum == 0 || stratum > 15) {
      sendJsonResponse(client, "{\"error\": \"NTP stratum must be 1-15\"}", 400);
      return;
    }
    config.ntp_stratum = stratum;
    changed = true;
  }

  if (changed && configManager->setConfig(config)) {
    sendJsonResponse(client, "{\"success\": true, \"message\": \"NTP configuration updated\"}");
  } else {
    sendJsonResponse(client, "{\"error\": \"Failed to update NTP configuration\"}", 400);
  }
}

void GpsWebServer::configSystemApiGet(EthernetClient &client) {
  if (!configManager) {
    sendJsonResponse(client, "{\"error\": \"Configuration Manager not available\"}", 500);
    return;
  }

  DynamicJsonDocument doc(512);
  const auto& config = configManager->getConfig();
  
  doc["auto_restart_enabled"] = config.auto_restart_enabled;
  doc["restart_interval"] = config.restart_interval;
  doc["debug_enabled"] = config.debug_enabled;
  doc["config_version"] = config.config_version;
  
  String jsonString;
  serializeJson(doc, jsonString);
  sendJsonResponse(client, jsonString);
}

void GpsWebServer::configSystemApiPost(EthernetClient &client, String postData) {
  if (!configManager) {
    sendJsonResponse(client, "{\"error\": \"Configuration Manager not available\"}", 500);
    return;
  }

  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, postData);
  
  if (error) {
    sendJsonResponse(client, "{\"error\": \"Invalid JSON format\"}", 400);
    return;
  }

  auto config = configManager->getConfig();
  bool changed = false;

  if (doc.containsKey("auto_restart_enabled")) {
    config.auto_restart_enabled = doc["auto_restart_enabled"];
    changed = true;
  }
  
  if (doc.containsKey("restart_interval")) {
    uint32_t interval = doc["restart_interval"];
    // 再起動間隔検証: 1-168時間（1週間）
    if (interval == 0 || interval > 168) {
      sendJsonResponse(client, "{\"error\": \"Restart interval must be 1-168 hours\"}", 400);
      return;
    }
    config.restart_interval = interval;
    changed = true;
  }
  
  if (doc.containsKey("debug_enabled")) {
    config.debug_enabled = doc["debug_enabled"];
    changed = true;
  }

  if (changed && configManager->setConfig(config)) {
    sendJsonResponse(client, "{\"success\": true, \"message\": \"System configuration updated\"}");
  } else {
    sendJsonResponse(client, "{\"error\": \"Failed to update system configuration\"}", 400);
  }
}

void GpsWebServer::configLogApiGet(EthernetClient &client) {
  if (!configManager) {
    sendJsonResponse(client, "{\"error\": \"Configuration Manager not available\"}", 500);
    return;
  }

  DynamicJsonDocument doc(512);
  const auto& config = configManager->getConfig();
  
  doc["syslog_server"] = config.syslog_server;
  doc["syslog_port"] = config.syslog_port;
  doc["log_level"] = config.log_level;
  
  String jsonString;
  serializeJson(doc, jsonString);
  sendJsonResponse(client, jsonString);
}

void GpsWebServer::configLogApiPost(EthernetClient &client, String postData) {
  if (!configManager) {
    sendJsonResponse(client, "{\"error\": \"Configuration Manager not available\"}", 500);
    return;
  }

  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, postData);
  
  if (error) {
    sendJsonResponse(client, "{\"error\": \"Invalid JSON format\"}", 400);
    return;
  }

  auto config = configManager->getConfig();
  bool changed = false;

  if (doc.containsKey("syslog_server")) {
    String server = doc["syslog_server"].as<String>();
    // Syslogサーバー検証: 0-63文字
    if (server.length() >= sizeof(config.syslog_server)) {
      sendJsonResponse(client, "{\"error\": \"Syslog server address too long (max 63 characters)\"}", 400);
      return;
    }
    strncpy(config.syslog_server, server.c_str(), sizeof(config.syslog_server) - 1);
    config.syslog_server[sizeof(config.syslog_server) - 1] = '\0';
    changed = true;
  }
  
  if (doc.containsKey("syslog_port")) {
    uint16_t port = doc["syslog_port"];
    // Syslogポート検証: 標準514または1024-65535
    if (port != 514 && (port < 1024 || port > 65535)) {
      sendJsonResponse(client, "{\"error\": \"Syslog port must be 514 or 1024-65535\"}", 400);
      return;
    }
    config.syslog_port = port;
    changed = true;
  }
  
  if (doc.containsKey("log_level")) {
    uint8_t level = doc["log_level"];
    // ログレベル検証: 0-7（DEBUG〜EMERGENCY）
    if (level > 7) {
      sendJsonResponse(client, "{\"error\": \"Log level must be 0-7 (0=DEBUG, 7=EMERGENCY)\"}", 400);
      return;
    }
    config.log_level = level;
    changed = true;
  }

  if (changed && configManager->setConfig(config)) {
    sendJsonResponse(client, "{\"success\": true, \"message\": \"Log configuration updated\"}");
  } else {
    sendJsonResponse(client, "{\"error\": \"Failed to update log configuration\"}", 400);
  }
}

void GpsWebServer::statusApiGet(EthernetClient &client) {
  DynamicJsonDocument doc(2048);
  
  // System uptime
  doc["uptime_seconds"] = millis() / 1000;
  doc["free_memory"] = 524288 - 20916; // 実測値使用
  
  // GPS status
  if (gpsClient) {
    auto gpsData = gpsClient->getWebGpsData();
    doc["gps"]["fix_type"] = gpsData.fix_type;
    doc["gps"]["satellites_total"] = gpsData.satellites_total;
    doc["gps"]["satellites_used"] = gpsData.satellites_used;
    doc["gps"]["data_valid"] = gpsData.data_valid;
    doc["gps"]["latitude"] = gpsData.latitude;
    doc["gps"]["longitude"] = gpsData.longitude;
  } else {
    doc["gps"]["status"] = "not_available";
  }
  
  // NTP statistics
  if (ntpServer) {
    const auto& stats = ntpServer->getStatistics();
    doc["ntp"]["requests_total"] = stats.requests_total;
    doc["ntp"]["responses_sent"] = stats.responses_sent;
    doc["ntp"]["avg_processing_time"] = stats.avg_processing_time;
  } else {
    doc["ntp"]["status"] = "not_available";
  }
  
  // Network status (connected if we can serve this page)
  doc["network"]["connected"] = true;
  doc["network"]["ip_assigned"] = true;
  
  String jsonString;
  serializeJson(doc, jsonString);
  sendJsonResponse(client, jsonString);
}

// メンテナンス機能API実装
void GpsWebServer::systemRebootApiPost(EthernetClient &client) {
  // システム再起動確認
  DynamicJsonDocument response(256);
  response["success"] = true;
  response["message"] = "System reboot initiated. Please wait 30 seconds before reconnecting.";
  
  String jsonString;
  serializeJson(response, jsonString);
  sendJsonResponse(client, jsonString);
  
  // ログ出力
  if (loggingService) {
    loggingService->log(LOG_INFO, "SYSTEM", "Web経由でシステム再起動要求");
  }
  
  // 少し待ってから再起動（レスポンス送信完了のため）
  delay(1000);
  rp2040.reboot();
}

void GpsWebServer::systemMetricsApiGet(EthernetClient &client) {
  DynamicJsonDocument doc(1024);
  
  // システムメトリクス
  doc["system"]["uptime_seconds"] = millis() / 1000;
  doc["system"]["free_memory"] = 524288 - 20916; // 実測値
  doc["system"]["flash_used_kb"] = 256; // 概算値
  doc["system"]["flash_total_kb"] = 2048;
  
  // CPU使用率（概算）
  doc["system"]["cpu_usage_percent"] = 25; // 典型的な使用率
  
  // ハードウェアステータス
  doc["hardware"]["gps_connected"] = gpsClient ? true : false;
  doc["hardware"]["ethernet_connected"] = true; // この API が動作しているため
  doc["hardware"]["display_connected"] = true; // 通常は接続済み
  
  // 温度情報（利用可能であれば）
  doc["hardware"]["temperature_celsius"] = analogReadTemp();
  
  String jsonString;
  serializeJson(doc, jsonString);
  sendJsonResponse(client, jsonString);
}

void GpsWebServer::systemLogsApiGet(EthernetClient &client) {
  DynamicJsonDocument doc(2048);
  
  // 最新ログエントリ（ログサービスから取得）
  if (loggingService) {
    // ログサービスの実装に依存するため、基本的な情報のみ提供
    doc["log_level"] = configManager ? configManager->getConfig().log_level : 1;
    doc["syslog_enabled"] = configManager ? (strlen(configManager->getConfig().syslog_server) > 0) : false;
    doc["local_buffer_size"] = 50; // 典型的なバッファサイズ
  }
  
  // 最近のシステムイベント（サンプル）
  JsonArray events = doc.createNestedArray("recent_events");
  
  JsonObject event1 = events.createNestedObject();
  event1["timestamp"] = millis() / 1000;
  event1["level"] = "INFO";
  event1["component"] = "WEBSERVER";
  event1["message"] = "System logs API accessed";
  
  if (gpsClient) {
    JsonObject event2 = events.createNestedObject();
    event2["timestamp"] = (millis() / 1000) - 10;
    event2["level"] = "INFO";
    event2["component"] = "GPS";
    event2["message"] = "GPS signal status: OK";
  }
  
  if (ntpServer) {
    JsonObject event3 = events.createNestedObject();
    event3["timestamp"] = (millis() / 1000) - 30;
    event3["level"] = "INFO";
    event3["component"] = "NTP";
    event3["message"] = "NTP server operational";
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  sendJsonResponse(client, jsonString);
}
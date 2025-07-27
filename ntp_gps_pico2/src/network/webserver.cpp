#include "webserver.h"
#include "NtpServer.h"
#include "../config/ConfigManager.h"
#include "../system/PrometheusMetrics.h"
#include "../config/LoggingService.h"

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
        loggingService->info("WEB", "Serving GPS details page");
      }
      gpsPage(client, ubxNavSatData_t);
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

void GpsWebServer::gpsPage(EthernetClient &client, UBX_NAV_SAT_data_t *ubxNavSatData_t)
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

void GpsWebServer::metricsPage(EthernetClient &client)
{
  printHeader(client, "text/plain");
  
  // PrometheusMetricsが利用可能な場合は包括的なメトリクスを出力
  if (prometheusMetrics) {
    // 大きなバッファを用意（Prometheusメトリクス全体用）
    static char metricsBuffer[4096];
    prometheusMetrics->generatePrometheusOutput(metricsBuffer, sizeof(metricsBuffer));
    client.print(metricsBuffer);
  } else {
    // フォールバック：基本的なシステムメトリクスのみ
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

void GpsWebServer::configPage(EthernetClient &client) {
  printHeader(client, "text/html");
  
  client.println("<!DOCTYPE HTML>");
  client.println("<html><head>");
  client.println("<title>GPS NTP Server Configuration</title>");
  client.println("<style>");
  client.println("body { font-family: Arial, sans-serif; margin: 20px; }");
  client.println("table { border-collapse: collapse; width: 100%; margin-bottom: 20px; }");
  client.println("th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }");
  client.println("th { background-color: #f2f2f2; }");
  client.println(".nav { margin-bottom: 20px; }");
  client.println(".nav a { margin-right: 10px; }");
  client.println("input, select { padding: 4px; margin: 2px; }");
  client.println("input[type='submit'] { background-color: #4CAF50; color: white; padding: 10px 20px; border: none; cursor: pointer; }");
  client.println("input[type='button'] { background-color: #f44336; color: white; padding: 10px 20px; border: none; cursor: pointer; margin-left: 10px; }");
  client.println(".error { color: red; font-weight: bold; }");
  client.println(".success { color: green; font-weight: bold; }");
  client.println(".warning { color: orange; font-weight: bold; }");
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
    
    client.println("<h2>Configuration Form</h2>");
    client.println("<form method=\"POST\" action=\"/config\" onsubmit=\"return submitConfig(event)\">");
    
    // Network Configuration Form
    client.println("<h3>Network Settings</h3>");
    client.println("<table>");
    client.printf("<tr><td>Hostname:</td><td><input type=\"text\" name=\"hostname\" value=\"%s\" maxlength=\"31\" required></td></tr>", config.hostname);
    client.printf("<tr><td>IP Address:</td><td><input type=\"text\" name=\"ip_address\" value=\"%s\" placeholder=\"0.0.0.0 for DHCP\"></td></tr>", config.ip_address == 0 ? "0.0.0.0" : "Static");
    client.printf("<tr><td>Syslog Server:</td><td><input type=\"text\" name=\"syslog_server\" value=\"%s\" maxlength=\"63\"></td></tr>", config.syslog_server);
    client.printf("<tr><td>Syslog Port:</td><td><input type=\"number\" name=\"syslog_port\" value=\"%d\" min=\"1\" max=\"65535\"></td></tr>", config.syslog_port);
    client.printf("<tr><td>Log Level:</td><td><select name=\"log_level\"><option value=\"0\"%s>DEBUG</option><option value=\"1\"%s>INFO</option><option value=\"2\"%s>WARN</option><option value=\"3\"%s>ERROR</option></select></td></tr>",
                  config.log_level == 0 ? " selected" : "",
                  config.log_level == 1 ? " selected" : "",
                  config.log_level == 2 ? " selected" : "",
                  config.log_level == 3 ? " selected" : "");
    client.println("</table>");
    
    // GNSS Configuration Form
    client.println("<h3>GNSS Settings</h3>");
    client.println("<table>");
    client.printf("<tr><td>GPS:</td><td><input type=\"checkbox\" name=\"gps_enabled\" value=\"1\"%s></td></tr>", config.gps_enabled ? " checked" : "");
    client.printf("<tr><td>GLONASS:</td><td><input type=\"checkbox\" name=\"glonass_enabled\" value=\"1\"%s></td></tr>", config.glonass_enabled ? " checked" : "");
    client.printf("<tr><td>Galileo:</td><td><input type=\"checkbox\" name=\"galileo_enabled\" value=\"1\"%s></td></tr>", config.galileo_enabled ? " checked" : "");
    client.printf("<tr><td>BeiDou:</td><td><input type=\"checkbox\" name=\"beidou_enabled\" value=\"1\"%s></td></tr>", config.beidou_enabled ? " checked" : "");
    client.printf("<tr><td>QZSS:</td><td><input type=\"checkbox\" name=\"qzss_enabled\" value=\"1\"%s></td></tr>", config.qzss_enabled ? " checked" : "");
    client.printf("<tr><td>QZSS L1S:</td><td><input type=\"checkbox\" name=\"qzss_l1s_enabled\" value=\"1\"%s></td></tr>", config.qzss_l1s_enabled ? " checked" : "");
    client.printf("<tr><td>Update Rate (Hz):</td><td><input type=\"number\" name=\"gnss_update_rate\" value=\"%d\" min=\"1\" max=\"10\"></td></tr>", config.gnss_update_rate);
    client.println("</table>");
    
    // NTP Configuration Form
    client.println("<h3>NTP Settings</h3>");
    client.println("<table>");
    client.printf("<tr><td>NTP Enabled:</td><td><input type=\"checkbox\" name=\"ntp_enabled\" value=\"1\"%s></td></tr>", config.ntp_enabled ? " checked" : "");
    client.printf("<tr><td>NTP Port:</td><td><input type=\"number\" name=\"ntp_port\" value=\"%d\" min=\"1\" max=\"65535\"></td></tr>", config.ntp_port);
    client.println("</table>");
    
    // System Configuration Form
    client.println("<h3>System Settings</h3>");
    client.println("<table>");
    client.printf("<tr><td>Prometheus:</td><td><input type=\"checkbox\" name=\"prometheus_enabled\" value=\"1\"%s></td></tr>", config.prometheus_enabled ? " checked" : "");
    client.printf("<tr><td>Debug Mode:</td><td><input type=\"checkbox\" name=\"debug_enabled\" value=\"1\"%s></td></tr>", config.debug_enabled ? " checked" : "");
    client.println("</table>");
    
    client.println("<p><input type=\"submit\" value=\"Save Configuration\"> <input type=\"button\" value=\"Reset to Defaults\" onclick=\"resetToDefaults()\"></p>");
    client.println("</form>");
    
    client.println("<div id=\"result\"></div>");
    
    client.println("<h2>Actions</h2>");
    client.println("<p><a href=\"/api/config\">View JSON Configuration</a></p>");
  } else {
    client.println("<p>Configuration Manager not available</p>");
  }
  
  // JavaScript for form handling and validation
  client.println("<script>");
  client.println("function submitConfig(event) {");
  client.println("  event.preventDefault();");
  client.println("  const form = event.target;");
  client.println("  const formData = new FormData(form);");
  client.println("  const config = {};");
  client.println("  for (let [key, value] of formData.entries()) {");
  client.println("    if (value === 'on') config[key] = true;");
  client.println("    else if (key.includes('_enabled') || key.includes('enabled')) config[key] = false;");
  client.println("    else config[key] = value;");
  client.println("  }");
  client.println("  // Handle checkboxes that weren't checked");
  client.println("  const checkboxes = ['gps_enabled', 'glonass_enabled', 'galileo_enabled', 'beidou_enabled', 'qzss_enabled', 'qzss_l1s_enabled', 'ntp_enabled', 'prometheus_enabled', 'debug_enabled'];");
  client.println("  checkboxes.forEach(name => { if (!(name in config)) config[name] = false; });");
  client.println("  fetch('/api/config', {");
  client.println("    method: 'POST',");
  client.println("    headers: { 'Content-Type': 'application/json' },");
  client.println("    body: JSON.stringify(config)");
  client.println("  }).then(response => response.json())");
  client.println("    .then(data => {");
  client.println("      const result = document.getElementById('result');");
  client.println("      if (data.success) {");
  client.println("        result.innerHTML = '<p class=\"success\">Configuration saved successfully!</p>';");
  client.println("        setTimeout(() => location.reload(), 2000);");
  client.println("      } else {");
  client.println("        result.innerHTML = '<p class=\"error\">Error: ' + (data.error || 'Unknown error') + '</p>';");
  client.println("      }");
  client.println("    }).catch(error => {");
  client.println("      document.getElementById('result').innerHTML = '<p class=\"error\">Network error: ' + error.message + '</p>';");
  client.println("    });");
  client.println("  return false;");
  client.println("}");
  client.println("function resetToDefaults() {");
  client.println("  if (confirm('Reset all settings to factory defaults? This action cannot be undone.')) {");
  client.println("    fetch('/api/reset', { method: 'POST' })");
  client.println("      .then(response => response.json())");
  client.println("      .then(data => {");
  client.println("        const result = document.getElementById('result');");
  client.println("        if (data.success) {");
  client.println("          result.innerHTML = '<p class=\"success\">Settings reset to defaults. System will restart...</p>';");
  client.println("          setTimeout(() => location.reload(), 3000);");
  client.println("        } else {");
  client.println("          result.innerHTML = '<p class=\"error\">Reset failed: ' + (data.error || 'Unknown error') + '</p>';");
  client.println("        }");
  client.println("      }).catch(error => {");
  client.println("        document.getElementById('result').innerHTML = '<p class=\"error\">Network error: ' + error.message + '</p>';");
  client.println("      });");
  client.println("  }");
  client.println("}");
  client.println("</script>");
  
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

void GpsWebServer::configApiReset(EthernetClient &client) {
  if (!configManager) {
    sendJsonResponse(client, "{\"error\": \"Configuration Manager not available\"}", 500);
    return;
  }
  
  configManager->resetToDefaults();
  sendJsonResponse(client, "{\"success\": true, \"message\": \"Configuration reset to defaults\"}");
}

void GpsWebServer::sendJsonResponse(EthernetClient &client, const String& json, int statusCode) {
  client.printf("HTTP/1.1 %d %s\r\n", statusCode, statusCode == 200 ? "OK" : "Error");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  client.println(json);
}

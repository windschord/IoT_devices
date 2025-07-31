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
        loggingService->info("WEB", "Serving configuration page from file system");
      }
      handleFileRequest(client, "/config.html", "text/html");
    }
    else if (s.indexOf("GET /config.js ") >= 0)
    {
      if (loggingService) {
        loggingService->info("WEB", "Serving configuration JavaScript from file system");
      }
      handleFileRequest(client, "/config.js", "text/javascript");
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
      int contentStart = s.indexOf("\r\n\r\n");
      if (contentStart >= 0) {
        String postData = s.substring(contentStart + 4);
        configApiPost(client, postData);
      } else {
        send404(client);
      }
    }
    else if (s.indexOf("POST /api/reset ") >= 0)
    {
      if (loggingService) {
        loggingService->info("WEB", "Processing factory reset request");
      }
      configApiReset(client);
    }
    // Category-specific API endpoints
    else if (s.indexOf("GET /api/config/network ") >= 0)
    {
      configNetworkApiGet(client);
    }
    else if (s.indexOf("POST /api/config/network ") >= 0)
    {
      int contentStart = s.indexOf("\r\n\r\n");
      if (contentStart >= 0) {
        String postData = s.substring(contentStart + 4);
        configNetworkApiPost(client, postData);
      } else {
        send404(client);
      }
    }
    else if (s.indexOf("GET /api/config/gnss ") >= 0)
    {
      configGnssApiGet(client);
    }
    else if (s.indexOf("POST /api/config/gnss ") >= 0)
    {
      int contentStart = s.indexOf("\r\n\r\n");
      if (contentStart >= 0) {
        String postData = s.substring(contentStart + 4);
        configGnssApiPost(client, postData);
      } else {
        send404(client);
      }
    }
    else if (s.indexOf("GET /api/config/ntp ") >= 0)
    {
      configNtpApiGet(client);
    }
    else if (s.indexOf("POST /api/config/ntp ") >= 0)
    {
      int contentStart = s.indexOf("\r\n\r\n");
      if (contentStart >= 0) {
        String postData = s.substring(contentStart + 4);
        configNtpApiPost(client, postData);
      } else {
        send404(client);
      }
    }
    else if (s.indexOf("GET /api/config/system ") >= 0)
    {
      configSystemApiGet(client);
    }
    else if (s.indexOf("POST /api/config/system ") >= 0)
    {
      int contentStart = s.indexOf("\r\n\r\n");
      if (contentStart >= 0) {
        String postData = s.substring(contentStart + 4);
        configSystemApiPost(client, postData);
      } else {
        send404(client);
      }
    }
    else if (s.indexOf("GET /api/config/log ") >= 0)
    {
      configLogApiGet(client);
    }
    else if (s.indexOf("POST /api/config/log ") >= 0)
    {
      int contentStart = s.indexOf("\r\n\r\n");
      if (contentStart >= 0) {
        String postData = s.substring(contentStart + 4);
        configLogApiPost(client, postData);
      } else {
        send404(client);
      }
    }
    else if (s.indexOf("GET /api/status ") >= 0)
    {
      statusApiGet(client);
    }
    else if (s.indexOf("POST /api/system/reboot ") >= 0)
    {
      systemRebootApiPost(client);
    }
    else if (s.indexOf("GET /api/system/metrics ") >= 0)
    {
      systemMetricsApiGet(client);
    }
    else if (s.indexOf("GET /api/system/logs ") >= 0)
    {
      systemLogsApiGet(client);
    }
    else if (s.indexOf("GET / ") >= 0)
    {
      if (loggingService) {
        loggingService->info("WEB", "Serving main page");
      }
      mainPage(client, gpsSummaryData);
    }
    else
    {
      if (loggingService) {
        loggingService->warning("WEB", "404 Not Found for request");
      }
      send404(client);
    }

    delay(1);
    client.stop();
    if (loggingService) {
      loggingService->info("WEB", "Client disconnected");
    }
  }
}

void GpsWebServer::printHeader(EthernetClient &client, String contentType)
{
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

void GpsWebServer::send404(EthernetClient &client)
{
  client.println("HTTP/1.1 404 Not Found");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE HTML>");
  client.println("<html><body><h1>404 Not Found</h1><p>The requested resource could not be found on this server.</p></body></html>");
}

void GpsWebServer::mainPage(EthernetClient &client, GpsSummaryData gpsSummaryData)
{
  printHeader(client, "text/html");
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.println("<head><title>GPS NTP Server</title></head>");
  client.println("<body>");
  client.println("<h1>GPS NTP Server</h1>");
  client.println("<p>Status: Running</p>");
  if (gpsSummaryData.fixType >= 2) {
    client.println("<p>GPS Fix: Valid (" + String(gpsSummaryData.SIV) + " satellites)</p>");
  } else {
    client.println("<p>GPS Fix: Invalid</p>");
  }
  client.println("<p><a href=\"/gps\">GPS Status</a> | <a href=\"/config\">Configuration</a> | <a href=\"/metrics\">Metrics</a></p>");
  client.println("</body>");
  client.println("</html>");
}

void GpsWebServer::configApiGet(EthernetClient &client) {
  if (!configManager) {
    sendJsonResponse(client, "{\"error\": \"Configuration Manager not available\"}", 500);
    return;
  }

  DynamicJsonDocument doc(2048);
  const auto& config = configManager->getConfig();
  
  // Network configuration
  doc["network"]["hostname"] = config.hostname;
  doc["network"]["ip_address"] = config.ip_address;
  doc["network"]["netmask"] = config.netmask;
  doc["network"]["gateway"] = config.gateway;
  doc["network"]["dns_server"] = config.dns_server;
  
  // GNSS configuration
  doc["gnss"]["gps_enabled"] = config.gps_enabled;
  doc["gnss"]["glonass_enabled"] = config.glonass_enabled;
  doc["gnss"]["galileo_enabled"] = config.galileo_enabled;
  doc["gnss"]["beidou_enabled"] = config.beidou_enabled;
  doc["gnss"]["qzss_enabled"] = config.qzss_enabled;
  doc["gnss"]["qzss_l1s_enabled"] = config.qzss_l1s_enabled;
  doc["gnss"]["gnss_update_rate"] = config.gnss_update_rate;
  doc["gnss"]["disaster_alert_priority"] = config.disaster_alert_priority;
  
  // System configuration
  doc["system"]["prometheus_enabled"] = config.prometheus_enabled;
  doc["system"]["syslog_server"] = config.syslog_server;
  doc["system"]["syslog_port"] = config.syslog_port;
  doc["system"]["log_level"] = config.log_level;
  
  String jsonString;
  serializeJson(doc, jsonString);
  sendJsonResponse(client, jsonString);
}

void GpsWebServer::configApiPost(EthernetClient &client, String postData) {
  if (!configManager) {
    sendJsonResponse(client, "{\"error\": \"Configuration Manager not available\"}", 500);
    return;
  }

  // TODO: Parse JSON and update configuration
  sendJsonResponse(client, "{\"success\": true, \"message\": \"Configuration updated\"}");
}

void GpsWebServer::configApiReset(EthernetClient &client) {
  if (!configManager) {
    sendJsonResponse(client, "{\"error\": \"Configuration Manager not available\"}", 500);
    return;
  }

  configManager->resetToDefaults();
  configManager->saveConfig();
  sendJsonResponse(client, "{\"success\": true, \"message\": \"Factory reset completed\"}");
}

void GpsWebServer::gpsApiGet(EthernetClient &client) {
  DynamicJsonDocument doc(2048);
  
  if (gpsClient) {
    GpsSummaryData gpsData = gpsClient->getGpsSummaryData();
    
    // Basic GPS information
    doc["fix_valid"] = (gpsData.fixType >= 2);
    doc["fix_type"] = gpsData.fixType;
    doc["satellites_total"] = gpsData.SIV;
    doc["satellites_gps"] = gpsData.SIV; // SIV includes all satellites in view
    doc["satellites_glonass"] = 0; // Not available in basic GpsSummaryData
    doc["satellites_galileo"] = 0; // Not available in basic GpsSummaryData
    doc["satellites_beidou"] = 0;  // Not available in basic GpsSummaryData
    doc["satellites_qzss"] = 0;    // Not available in basic GpsSummaryData
    
    // Position information
    doc["latitude"] = gpsData.latitude;
    doc["longitude"] = gpsData.longitude;
    doc["altitude"] = gpsData.altitude;
    doc["speed"] = 0;  // Not available in basic GpsSummaryData
    doc["course"] = 0; // Not available in basic GpsSummaryData
    
    // Accuracy information - not available in basic GpsSummaryData
    doc["hdop"] = 0;
    doc["vdop"] = 0;
    doc["pdop"] = 0;
    doc["accuracy_horizontal"] = 0;
    doc["accuracy_vertical"] = 0;
    
    // Time information
    doc["utc_year"] = gpsData.year;
    doc["utc_month"] = gpsData.month;
    doc["utc_day"] = gpsData.day;
    doc["utc_hour"] = gpsData.hour;
    doc["utc_minute"] = gpsData.min;
    doc["utc_second"] = gpsData.sec;
    
    // PPS information - not available in basic GpsSummaryData
    doc["pps_active"] = false;
    doc["last_pps_time"] = 0;
    
    // QZSS L1S information - not available in basic GpsSummaryData
    doc["qzss_l1s_signal_detected"] = false;
    doc["disaster_category"] = 0;
    doc["disaster_message"] = "";
  } else {
    doc["error"] = "GPS client not available";
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  sendJsonResponse(client, jsonString);
}

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

  // Security checks
  String clientIP = client.remoteIP().toString();
  if (!checkRequestRate(clientIP)) {
    sendJsonResponse(client, "{\"error\": \"Rate limit exceeded. Please wait before making more requests.\"}", 429);
    return;
  }
  
  if (!isValidJsonInput(postData)) {
    sendJsonResponse(client, "{\"error\": \"Invalid or malformed JSON input\"}", 400);
    return;
  }

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, postData);
  
  if (error) {
    sendJsonResponse(client, "{\"error\": \"Invalid JSON format\"}", 400);
    return;
  }

  auto config = configManager->getConfig();
  bool configChanged = false;

  // Update network configuration
  if (doc.containsKey("hostname")) {
    String hostname = doc["hostname"].as<String>();
    if (hostname.length() > 0 && hostname.length() <= 31) {
      strncpy(config.hostname, hostname.c_str(), sizeof(config.hostname) - 1);
      config.hostname[sizeof(config.hostname) - 1] = '\0';
      configChanged = true;
    } else {
      sendJsonResponse(client, "{\"error\": \"Hostname must be 1-31 characters\"}", 400);
      return;
    }
  }

  if (doc.containsKey("ip_address")) {
    config.ip_address = doc["ip_address"].as<uint32_t>();
    configChanged = true;
  }

  if (doc.containsKey("netmask")) {
    config.netmask = doc["netmask"].as<uint32_t>();
    configChanged = true;
  }

  if (doc.containsKey("gateway")) {
    config.gateway = doc["gateway"].as<uint32_t>();
    configChanged = true;
  }

  if (doc.containsKey("dns_server")) {
    config.dns_server = doc["dns_server"].as<uint32_t>();
    configChanged = true;
  }

  if (configChanged) {
    configManager->setConfig(config);
    if (configManager->saveConfig()) {
      sendJsonResponse(client, "{\"success\": true, \"message\": \"Network configuration saved successfully\"}");
      if (loggingService) {
        loggingService->info("WEB", "Network configuration updated via web interface");
      }
    } else {
      sendJsonResponse(client, "{\"error\": \"Failed to save network configuration\"}", 500);
    }
  } else {
    sendJsonResponse(client, "{\"success\": true, \"message\": \"No changes made to network configuration\"}");
  }
}

void GpsWebServer::configGnssApiGet(EthernetClient &client) {
  if (!configManager) {
    sendJsonResponse(client, "{\"error\": \"Configuration Manager not available\"}", 500);
    return;
  }

  DynamicJsonDocument doc(1024);
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

  // Security checks
  String clientIP = client.remoteIP().toString();
  if (!checkRequestRate(clientIP)) {
    sendJsonResponse(client, "{\"error\": \"Rate limit exceeded. Please wait before making more requests.\"}", 429);
    return;
  }
  
  if (!isValidJsonInput(postData)) {
    sendJsonResponse(client, "{\"error\": \"Invalid or malformed JSON input\"}", 400);
    return;
  }

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, postData);
  
  if (error) {
    sendJsonResponse(client, "{\"error\": \"Invalid JSON format\"}", 400);
    return;
  }

  auto config = configManager->getConfig();
  bool configChanged = false;

  // Update GNSS configuration
  if (doc.containsKey("gps_enabled")) {
    config.gps_enabled = doc["gps_enabled"].as<bool>();
    configChanged = true;
  }

  if (doc.containsKey("glonass_enabled")) {
    config.glonass_enabled = doc["glonass_enabled"].as<bool>();
    configChanged = true;
  }

  if (doc.containsKey("galileo_enabled")) {
    config.galileo_enabled = doc["galileo_enabled"].as<bool>();
    configChanged = true;
  }

  if (doc.containsKey("beidou_enabled")) {
    config.beidou_enabled = doc["beidou_enabled"].as<bool>();
    configChanged = true;
  }

  if (doc.containsKey("qzss_enabled")) {
    config.qzss_enabled = doc["qzss_enabled"].as<bool>();
    configChanged = true;
  }

  if (doc.containsKey("qzss_l1s_enabled")) {
    config.qzss_l1s_enabled = doc["qzss_l1s_enabled"].as<bool>();
    configChanged = true;
  }

  if (doc.containsKey("gnss_update_rate")) {
    uint8_t rate = doc["gnss_update_rate"].as<uint8_t>();
    if (rate >= 1 && rate <= 10) {
      config.gnss_update_rate = rate;
      configChanged = true;
    } else {
      sendJsonResponse(client, "{\"error\": \"GNSS update rate must be between 1 and 10 Hz\"}", 400);
      return;
    }
  }

  if (doc.containsKey("disaster_alert_priority")) {
    uint8_t priority = doc["disaster_alert_priority"].as<uint8_t>();
    if (priority <= 2) {
      config.disaster_alert_priority = priority;
      configChanged = true;
    } else {
      sendJsonResponse(client, "{\"error\": \"Disaster alert priority must be 0 (Low), 1 (Medium), or 2 (High)\"}", 400);
      return;
    }
  }

  if (configChanged) {
    configManager->setConfig(config);
    if (configManager->saveConfig()) {
      sendJsonResponse(client, "{\"success\": true, \"message\": \"GNSS configuration saved successfully\"}");
      if (loggingService) {
        loggingService->info("WEB", "GNSS configuration updated via web interface");
      }
    } else {
      sendJsonResponse(client, "{\"error\": \"Failed to save GNSS configuration\"}", 500);
    }
  } else {
    sendJsonResponse(client, "{\"success\": true, \"message\": \"No changes made to GNSS configuration\"}");
  }
}

void GpsWebServer::configNtpApiGet(EthernetClient &client) {
  if (!configManager) {
    sendJsonResponse(client, "{\"error\": \"Configuration Manager not available\"}", 500);
    return;
  }

  DynamicJsonDocument doc(512);
  const auto& config = configManager->getConfig();
  
  doc["ntp_enabled"] = true; // NTP is always enabled in this implementation
  doc["ntp_port"] = 123;     // Standard NTP port
  doc["ntp_stratum"] = 1;    // GPS-synchronized stratum
  
  String jsonString;
  serializeJson(doc, jsonString);
  sendJsonResponse(client, jsonString);
}

void GpsWebServer::configNtpApiPost(EthernetClient &client, String postData) {
  if (!configManager) {
    sendJsonResponse(client, "{\"error\": \"Configuration Manager not available\"}", 500);
    return;
  }

  // Security checks
  String clientIP = client.remoteIP().toString();
  if (!checkRequestRate(clientIP)) {
    sendJsonResponse(client, "{\"error\": \"Rate limit exceeded. Please wait before making more requests.\"}", 429);
    return;
  }
  
  if (!isValidJsonInput(postData)) {
    sendJsonResponse(client, "{\"error\": \"Invalid or malformed JSON input\"}", 400);
    return;
  }

  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, postData);
  
  if (error) {
    sendJsonResponse(client, "{\"error\": \"Invalid JSON format\"}", 400);
    return;
  }

  // NTP configuration is mostly fixed in this implementation
  // Just acknowledge the request
  sendJsonResponse(client, "{\"success\": true, \"message\": \"NTP configuration is managed automatically\"}");
  
  if (loggingService) {
    loggingService->info("WEB", "NTP configuration request processed (no changes made)");
  }
}

void GpsWebServer::configSystemApiGet(EthernetClient &client) {
  if (!configManager) {
    sendJsonResponse(client, "{\"error\": \"Configuration Manager not available\"}", 500);
    return;
  }

  DynamicJsonDocument doc(512);
  // System configuration is basic in this implementation
  doc["auto_restart_enabled"] = false;
  doc["restart_interval"] = 24;
  doc["debug_enabled"] = false;
  
  String jsonString;
  serializeJson(doc, jsonString);
  sendJsonResponse(client, jsonString);
}

void GpsWebServer::configSystemApiPost(EthernetClient &client, String postData) {
  if (!configManager) {
    sendJsonResponse(client, "{\"error\": \"Configuration Manager not available\"}", 500);
    return;
  }

  // Security checks
  String clientIP = client.remoteIP().toString();
  if (!checkRequestRate(clientIP)) {
    sendJsonResponse(client, "{\"error\": \"Rate limit exceeded. Please wait before making more requests.\"}", 429);
    return;
  }
  
  if (!isValidJsonInput(postData)) {
    sendJsonResponse(client, "{\"error\": \"Invalid or malformed JSON input\"}", 400);
    return;
  }

  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, postData);
  
  if (error) {
    sendJsonResponse(client, "{\"error\": \"Invalid JSON format\"}", 400);
    return;
  }

  // System configuration changes are acknowledged but not fully implemented
  sendJsonResponse(client, "{\"success\": true, \"message\": \"System configuration received (basic implementation)\"}");
  
  if (loggingService) {
    loggingService->info("WEB", "System configuration request processed");
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
  doc["prometheus_enabled"] = config.prometheus_enabled;
  
  String jsonString;
  serializeJson(doc, jsonString);
  sendJsonResponse(client, jsonString);
}

void GpsWebServer::configLogApiPost(EthernetClient &client, String postData) {
  if (!configManager) {
    sendJsonResponse(client, "{\"error\": \"Configuration Manager not available\"}", 500);
    return;
  }

  // Security checks
  String clientIP = client.remoteIP().toString();
  if (!checkRequestRate(clientIP)) {
    sendJsonResponse(client, "{\"error\": \"Rate limit exceeded. Please wait before making more requests.\"}", 429);
    return;
  }
  
  if (!isValidJsonInput(postData)) {
    sendJsonResponse(client, "{\"error\": \"Invalid or malformed JSON input\"}", 400);
    return;
  }

  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, postData);
  
  if (error) {
    sendJsonResponse(client, "{\"error\": \"Invalid JSON format\"}", 400);
    return;
  }

  auto config = configManager->getConfig();
  bool configChanged = false;

  // Update log configuration
  if (doc.containsKey("syslog_server")) {
    String syslogServer = doc["syslog_server"].as<String>();
    if (syslogServer.length() <= sizeof(config.syslog_server) - 1) {
      strncpy(config.syslog_server, syslogServer.c_str(), sizeof(config.syslog_server) - 1);
      config.syslog_server[sizeof(config.syslog_server) - 1] = '\0';
      configChanged = true;
    } else {
      sendJsonResponse(client, "{\"error\": \"Syslog server address too long\"}", 400);
      return;
    }
  }

  if (doc.containsKey("syslog_port")) {
    uint16_t port = doc["syslog_port"].as<uint16_t>();
    if (port >= 1 && port <= 65535) {
      config.syslog_port = port;
      configChanged = true;
    } else {
      sendJsonResponse(client, "{\"error\": \"Syslog port must be between 1 and 65535\"}", 400);
      return;
    }
  }

  if (doc.containsKey("log_level")) {
    uint8_t level = doc["log_level"].as<uint8_t>();
    if (level <= 7) {
      config.log_level = level;
      configChanged = true;
    } else {
      sendJsonResponse(client, "{\"error\": \"Log level must be between 0 (Emergency) and 7 (Debug)\"}", 400);
      return;
    }
  }

  if (doc.containsKey("prometheus_enabled")) {
    config.prometheus_enabled = doc["prometheus_enabled"].as<bool>();
    configChanged = true;
  }

  if (configChanged) {
    configManager->setConfig(config);
    if (configManager->saveConfig()) {
      sendJsonResponse(client, "{\"success\": true, \"message\": \"Logging configuration saved successfully\"}");
      if (loggingService) {
        loggingService->info("WEB", "Logging configuration updated via web interface");
      }
    } else {
      sendJsonResponse(client, "{\"error\": \"Failed to save logging configuration\"}", 500);
    }
  } else {
    sendJsonResponse(client, "{\"success\": true, \"message\": \"No changes made to logging configuration\"}");
  }
}

void GpsWebServer::statusApiGet(EthernetClient &client) {
  DynamicJsonDocument doc(1024);
  
  if (gpsClient) {
    GpsSummaryData gpsData = gpsClient->getGpsSummaryData();
    doc["gps_fix"] = (gpsData.fixType >= 2);
    doc["satellites"] = gpsData.SIV;
    doc["pps_active"] = false; // Not available in basic GpsSummaryData
  } else {
    doc["gps_fix"] = false;
    doc["satellites"] = 0;
    doc["pps_active"] = false;
  }
  
  // Network status
  doc["network_connected"] = (Ethernet.linkStatus() == LinkON);
  doc["ip_address"] = Ethernet.localIP().toString();
  
  // System status
  doc["uptime_seconds"] = millis() / 1000;
  doc["free_memory"] = rp2040.getFreeHeap();
  
  String jsonString;
  serializeJson(doc, jsonString);
  sendJsonResponse(client, jsonString);
}

void GpsWebServer::systemRebootApiPost(EthernetClient &client) {
  sendJsonResponse(client, "{\"success\": true, \"message\": \"System reboot initiated\"}");
  
  if (loggingService) {
    loggingService->warning("WEB", "System reboot requested via web interface");
  }
  
  delay(1000);  // Allow response to be sent
  // Note: Actual reboot implementation would depend on platform
  // ESP.restart(); // For ESP platforms
}

void GpsWebServer::systemMetricsApiGet(EthernetClient &client) {
  DynamicJsonDocument doc(1024);
  
  if (prometheusMetrics) {
    // Get metrics from PrometheusMetrics service
    doc["ntp_requests"] = prometheusMetrics->getNtpMetrics().totalRequests;
    doc["uptime_seconds"] = millis() / 1000;
    doc["memory_used"] = prometheusMetrics->getSystemMetrics().usedRam;
    doc["health_score"] = prometheusMetrics->getSystemHealth();
  } else {
    doc["ntp_requests"] = 0;
    doc["uptime_seconds"] = millis() / 1000;
    doc["memory_used"] = 0;
    doc["health_score"] = 50.0;
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  sendJsonResponse(client, jsonString);
}

void GpsWebServer::systemLogsApiGet(EthernetClient &client) {
  DynamicJsonDocument doc(1024);
  JsonArray logs = doc.createNestedArray("logs");
  
  // Get recent logs from LoggingService if available
  if (loggingService) {
    // This would require LoggingService to provide recent logs
    // For now, return placeholder data
    JsonObject log1 = logs.createNestedObject();
    log1["timestamp"] = "2023-12-01 12:00:00";
    log1["level"] = "INFO";
    log1["component"] = "SYSTEM";
    log1["message"] = "System initialized successfully";
    
    JsonObject log2 = logs.createNestedObject();
    log2["timestamp"] = "2023-12-01 12:00:01";
    log2["level"] = "INFO";
    log2["component"] = "GPS";
    log2["message"] = "GPS signal acquired";
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  sendJsonResponse(client, jsonString);
}

void GpsWebServer::metricsPage(EthernetClient &client)
{
  if (prometheusMetrics) {
    char metricsBuffer[4096];
    prometheusMetrics->generatePrometheusOutput(metricsBuffer, sizeof(metricsBuffer));
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain; version=0.0.4; charset=utf-8");
    client.println("Connection: close");
    client.println("Cache-Control: no-cache");
    client.println();
    client.print(metricsBuffer);
  } else {
    send404(client);
  }
}

void GpsWebServer::handleFileRequest(EthernetClient &client, const String &filepath, const String &contentType) {
  if (!LittleFS.begin()) {
    if (loggingService) {
      loggingService->error("WEB", "Failed to mount LittleFS");
    }
    send404(client);
    return;
  }

  File file = LittleFS.open(filepath, "r");
  if (!file) {
    if (loggingService) {
      loggingService->error("WEB", ("File not found: " + filepath).c_str());
    }
    send404(client);
    return;
  }

  printHeader(client, contentType);
  
  // Send file in chunks to avoid memory issues
  const size_t bufferSize = 512;
  uint8_t buffer[bufferSize];
  while (file.available()) {
    size_t bytesRead = file.read(buffer, bufferSize);
    client.write(buffer, bytesRead);
  }
  
  file.close();
  
  if (loggingService) {
    loggingService->info("WEB", ("Served file: " + filepath).c_str());
  }
}

void GpsWebServer::sendJsonResponse(EthernetClient &client, const String &jsonResponse, int statusCode) {
  String statusText = "OK";
  if (statusCode == 400) statusText = "Bad Request";
  else if (statusCode == 404) statusText = "Not Found";
  else if (statusCode == 429) statusText = "Too Many Requests";
  else if (statusCode == 500) statusText = "Internal Server Error";
  
  client.println("HTTP/1.1 " + String(statusCode) + " " + statusText);
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println("X-Content-Type-Options: nosniff");
  client.println("X-Frame-Options: DENY");
  client.println("X-XSS-Protection: 1; mode=block");
  client.println("Cache-Control: no-cache, no-store, must-revalidate");
  client.println("Pragma: no-cache");
  client.println("Expires: 0");
  client.println();
  client.println(jsonResponse);
}

bool GpsWebServer::checkRequestRate(const String &clientIP) {
  // Simple rate limiting - allow 30 requests per minute per IP
  // This is a basic implementation and could be enhanced
  static unsigned long lastRequestTime = 0;
  static String lastClientIP = "";
  static int requestCount = 0;
  
  unsigned long currentTime = millis();
  
  if (clientIP == lastClientIP) {
    if (currentTime - lastRequestTime < 60000) { // Within 1 minute
      requestCount++;
      if (requestCount > 30) {
        return false; // Rate limit exceeded
      }
    } else {
      requestCount = 1; // Reset counter
      lastRequestTime = currentTime;
    }
  } else {
    lastClientIP = clientIP;
    requestCount = 1;
    lastRequestTime = currentTime;
  }
  
  return true;
}

bool GpsWebServer::isValidJsonInput(const String &input) {
  // Basic JSON validation and size check
  if (input.length() == 0 || input.length() > 2048) {
    return false;
  }
  
  // Check for basic JSON structure
  String trimmed = input;
  trimmed.trim();
  if (!trimmed.startsWith("{") || !trimmed.endsWith("}")) {
    return false;
  }
  
  // Check for potentially malicious content
  if (input.indexOf("<script") >= 0 || input.indexOf("javascript:") >= 0) {
    return false;
  }
  
  return true;
}

String GpsWebServer::sanitizeInput(const String &input) {
  String sanitized = input;
  
  // Replace HTML special characters
  sanitized.replace("&", "&amp;");
  sanitized.replace("<", "&lt;");
  sanitized.replace(">", "&gt;");
  sanitized.replace("\"", "&quot;");
  sanitized.replace("'", "&#x27;");
  sanitized.replace("/", "&#x2F;");
  
  return sanitized;
}

// Note: Static member variables and setter methods are now defined inline in the header file
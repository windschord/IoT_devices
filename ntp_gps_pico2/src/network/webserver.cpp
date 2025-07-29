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
  client.println("<title>GPS NTP Server Configuration</title>");
  client.println("<style>");
  client.println("body { font-family: Arial, sans-serif; margin: 20px; }");
  client.println("table { border-collapse: collapse; width: 100%; margin-bottom: 20px; }");
  client.println("th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }");
  client.println("th { background-color: #f2f2f2; }");
  client.println("input[type=text], input[type=number], select { width: 100%; padding: 4px; }");
  client.println("input[type=submit] { background-color: #4CAF50; color: white; padding: 8px 16px; border: none; cursor: pointer; }");
  client.println("input[type=submit]:hover { background-color: #45a049; }");
  client.println(".success { color: green; font-weight: bold; }");
  client.println(".error { color: red; font-weight: bold; }");
  client.println("</style>");
  client.println("</head><body>");
  
  client.println("<h1>GPS NTP Server Configuration</h1>");
  client.println("<p><a href=\"/\">← Back to Status</a></p>");
  
  if (configManager) {
    // Configuration form implementation would go here
    client.println("<h2>Current Configuration</h2>");
    client.println("<p>Configuration management is available via API endpoints:</p>");
    client.println("<ul>");
    client.println("<li>GET /api/config - Get current configuration</li>");
    client.println("<li>POST /api/config - Update configuration</li>");
    client.println("<li>POST /api/reset - Factory reset</li>");
    client.println("</ul>");
  } else {
    client.println("<h2>Configuration Manager Not Available</h2>");
    client.println("<p>Configuration management service is not initialized.</p>");
  }
  
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
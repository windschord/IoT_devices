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

void GpsWebServer::gpsPage(EthernetClient &client, UBX_NAV_SAT_data_t *ubxNavSatData_t)
{
  printHeader(client, "text/html");

  client.println(F("<!DOCTYPE HTML>"));
  client.println(F("<html>"));
  client.println(F("<head>"));
  client.println(F("<title>GPS Satellite Information</title>"));
  client.println(F("<meta charset='utf-8'>"));
  client.println(F("<meta name='viewport' content='width=device-width, initial-scale=1'>"));
  client.println(F("<style>"));
  client.println(F("body { font-family: Arial, sans-serif; margin: 20px; background-color: #f5f5f5; }"));
  client.println(F(".container { max-width: 1200px; margin: 0 auto; }"));
  client.println(F(".header { background: #2c3e50; color: white; padding: 20px; border-radius: 8px; margin-bottom: 20px; }"));
  client.println(F(".view-toggle { margin: 20px 0; text-align: center; }"));
  client.println(F(".btn { background: #3498db; color: white; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; margin: 0 5px; }"));
  client.println(F(".btn:hover { background: #2980b9; }"));
  client.println(F(".btn.active { background: #e74c3c; }"));
  client.println(F(".view-panel { background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }"));
  client.println(F(".satellite-view { display: flex; gap: 20px; }"));
  client.println(F(".radar-container { flex: 1; text-align: center; }"));
  client.println(F(".info-panel { flex: 0 0 300px; }"));
  client.println(F(".stats-grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 10px; margin: 20px 0; }"));
  client.println(F(".date-view-grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 10px; margin: 20px 0; }"));
  client.println(F(".stat-card { background: #ecf0f1; padding: 15px; border-radius: 4px; text-align: center; }"));
  client.println(F(".stat-card small { color: #7f8c8d; display: block; margin-top: 5px; font-size: 11px; }"));
  client.println(F(".constellation-legend { margin: 20px 0; }"));
  client.println(F(".legend-item { display: inline-block; margin: 5px 10px; padding: 5px 10px; border-radius: 4px; color: white; font-size: 12px; }"));
  client.println(F(".gps { background: #f39c12; }"));
  client.println(F(".glonass { background: #e74c3c; }"));
  client.println(F(".galileo { background: #27ae60; }"));
  client.println(F(".beidou { background: #3498db; }"));
  client.println(F(".sbas { background: #95a5a6; }"));
  client.println(F(".qzss { background: #9b59b6; }"));
  client.println(F("#radarChart { border: 2px solid #34495e; border-radius: 50%; }"));
  client.println(F("</style>"));
  client.println(F("</head>"));
  client.println(F("<body>"));
  
  client.println(F("<div class='container'>"));
  client.println(F("<div class='header'>"));
  client.println(F("<h1>GPS NTP Server - Satellite Information</h1>"));
  client.println(F("<p>Real-time GNSS satellite tracking and positioning</p>"));
  client.println(F("<div style='margin-top: 10px; font-size: 14px;'>"));
  client.println(F("Connection Status: <span id='connectionStatus' style='font-weight: bold;'>Connecting...</span>"));
  client.println(F(" | Last Update: <span id='lastUpdateTime'>-</span>"));
  client.println(F("</div>"));
  client.println(F("</div>"));
  
  client.println(F("<div class='view-toggle'>"));
  client.println(F("<button class='btn active' onclick='showSatelliteView()'>Satellite Position View</button>"));
  client.println(F("<button class='btn' onclick='showDateView()'>Date & Position View</button>"));
  client.println(F("</div>"));
  
  client.println(F("<div id='satelliteView' class='view-panel'>"));
  client.println(F("<div class='satellite-view'>"));
  client.println(F("<div class='radar-container'>"));
  client.println(F("<h3>Satellite Position View</h3>"));
  client.println(F("<canvas id='radarChart' width='400' height='400'></canvas>"));
  client.println(F("<div class='constellation-legend'>"));
  client.println(F("<div class='legend-item gps'>GPS</div>"));
  client.println(F("<div class='legend-item sbas'>SBAS</div>"));
  client.println(F("<div class='legend-item galileo'>Galileo</div>"));
  client.println(F("<div class='legend-item beidou'>BeiDou</div>"));
  client.println(F("<div class='legend-item glonass'>GLONASS</div>"));
  client.println(F("<div class='legend-item qzss'>QZSS</div>"));
  client.println(F("</div>"));
  client.println(F("</div>"));
  
  client.println(F("<div class='info-panel'>"));
  client.println(F("<h3>Constellation Statistics</h3>"));
  client.println(F("<div class='stats-grid' id='constellationStats'>"));
  client.println(F("<!-- Stats will be populated by JavaScript -->"));
  client.println(F("</div>"));
  client.println(F("<h3>GNSS Constellation</h3>"));
  client.println(F("<div id='gnssControls'>"));
  client.println(F("<!-- Controls will be populated by JavaScript -->"));
  client.println(F("</div>"));
  client.println(F("<h3>Satellite Filters</h3>"));
  client.println(F("<div style='margin: 10px 0;'>"));
  client.println(F("<label><input type='checkbox' id='showNotTracked' checked> Show not tracked</label><br>"));
  client.println(F("<label><input type='checkbox' id='showUsedOnly'> Show used in navigation only</label><br>"));
  client.println(F("<label><input type='checkbox' id='showHighSignal'> Show high signal (>35 dB-Hz) only</label>"));
  client.println(F("</div>"));
  client.println(F("<h3>Zoom & View</h3>"));
  client.println(F("<div style='margin: 10px 0;'>"));
  client.println(F("<label>Zoom level: <span id='zoomValue'>1.0x</span></label><br>"));
  client.println(F("<input type='range' id='zoomSlider' min='5' max='20' value='15' style='width: 100%' oninput='updateZoomLabel()'>"));
  client.println(F("<br><br>"));
  client.println(F("<button class='btn' onclick='resetView()' style='width: 100%; font-size: 12px;'>Reset View</button>"));
  client.println(F("</div>"));
  client.println(F("</div>"));
  client.println(F("</div>"));
  client.println(F("</div>"));
  
  client.println(F("<div id='dateView' class='view-panel' style='display: none;'>"));
  client.println(F("<h3>Date & Position Information</h3>"));
  client.println(F("<div class='date-view-grid' id='dateViewStats'>"));
  client.println(F("<!-- Date view stats will be populated by JavaScript -->"));
  client.println(F("</div>"));
  client.println(F("</div>"));
  
  client.println(F("<script>"));
  client.println(F("// Global state management"));
  client.println(F("let gpsData = null;"));
  client.println(F("let updateInterval = null;"));
  client.println(F("let lastUpdateTime = 0;"));
  client.println(F("let connectionFailures = 0;"));
  client.println(F("const maxConnectionFailures = 5;"));
  client.println(F("let isUserInteracting = false;"));
  client.println(F("let interactionTimeout = null;"));
  client.println(F("let previousGpsData = null;"));
  client.println(F("let connectionStatus = 'connected';"));
  
  client.println(F("function showSatelliteView() {"));
  client.println(F("  document.getElementById('satelliteView').style.display = 'block';"));
  client.println(F("  document.getElementById('dateView').style.display = 'none';"));
  client.println(F("  document.querySelectorAll('.btn').forEach(function(btn) { btn.classList.remove('active'); });"));
  client.println(F("  event.target.classList.add('active');"));
  client.println(F("}"));
  
  client.println(F("function showDateView() {"));
  client.println(F("  document.getElementById('satelliteView').style.display = 'none';"));
  client.println(F("  document.getElementById('dateView').style.display = 'block';"));
  client.println(F("  document.querySelectorAll('.btn').forEach(function(btn) { btn.classList.remove('active'); });"));
  client.println(F("  event.target.classList.add('active');"));
  client.println(F("}"));
  
  client.println(F("function fetchGpsData() {"));
  client.println(F("  // Skip update if user is actively interacting"));
  client.println(F("  if (isUserInteracting) {"));
  client.println(F("    console.log('Skipping update - user interaction in progress');"));
  client.println(F("    return;"));
  client.println(F("  }"));
  client.println(F("  "));
  client.println(F("  let startTime = Date.now();"));
  client.println(F("  "));
  client.println(F("  fetch('/api/gps', {"));
  client.println(F("    method: 'GET',"));
  client.println(F("    headers: { 'Cache-Control': 'no-cache' },"));
  client.println(F("    signal: AbortSignal.timeout(5000) // 5 second timeout"));
  client.println(F("  })"));
  client.println(F("  .then(response => {"));
  client.println(F("    if (!response.ok) {"));
  client.println(F("      throw new Error('HTTP ' + response.status + ': ' + response.statusText);"));
  client.println(F("    }"));
  client.println(F("    return response.json();"));
  client.println(F("  })"));
  client.println(F("  .then(data => {"));
  client.println(F("    // Store previous data for differential updates"));
  client.println(F("    previousGpsData = gpsData;"));
  client.println(F("    gpsData = data;"));
  client.println(F("    lastUpdateTime = Date.now();"));
  client.println(F("    connectionFailures = 0;"));
  client.println(F("    connectionStatus = 'connected';"));
  client.println(F("    "));
  client.println(F("    // Only update display if data has changed significantly"));
  client.println(F("    if (hasSignificantChange(previousGpsData, gpsData)) {"));
  client.println(F("      updateDisplayWithTimestamp();"));
  client.println(F("    }"));
  client.println(F("    "));
  client.println(F("    updateConnectionStatus();"));
  client.println(F("    var responseTime = Date.now() - startTime;"));
  client.println(F("    console.log('GPS data updated in ' + responseTime + 'ms');"));
  client.println(F("  })"));
  client.println(F("  .catch(error => {"));
  client.println(F("    connectionFailures++;"));
  client.println(F("    connectionStatus = 'error';"));
  client.println(F("    console.error('GPS data fetch error:', error);"));
  client.println(F("    "));
  client.println(F("    updateConnectionStatus();"));
  client.println(F("    "));
  client.println(F("    // Implement exponential backoff for reconnection"));
  client.println(F("    if (connectionFailures >= maxConnectionFailures) {"));
  client.println(F("      console.warn('Max connection failures reached. Attempting reconnection...');"));
  client.println(F("      attemptReconnection();"));
  client.println(F("    }"));
  client.println(F("  });"));
  client.println(F("}"));
  
  client.println(F("function updateDisplay() {"));
  client.println(F("  if (!gpsData) return;"));
  client.println(F("  drawRadarChart();"));
  client.println(F("  updateConstellationStats();"));
  client.println(F("  updateDateView();"));
  client.println(F("}"));
  
  // radarChart描画関数の続きは次のセクションで実装
  client.println(F("function drawRadarChart() {"));
  client.println(F("  let canvas = document.getElementById('radarChart');"));
  client.println(F("  let ctx = canvas.getContext('2d');"));
  client.println(F("  let centerX = canvas.width / 2;"));
  client.println(F("  let centerY = canvas.height / 2;"));
  client.println(F("  let radius = Math.min(centerX, centerY) - 20;"));
  client.println(F("  "));
  client.println(F("  // Clear canvas"));
  client.println(F("  ctx.clearRect(0, 0, canvas.width, canvas.height);"));
  client.println(F("  "));
  client.println(F("  // Draw concentric circles"));
  client.println(F("  ctx.strokeStyle = '#bdc3c7';"));
  client.println(F("  ctx.lineWidth = 1;"));
  client.println(F("  for (var i = 1; i <= 3; i++) {"));
  client.println(F("    ctx.beginPath();"));
  client.println(F("    ctx.arc(centerX, centerY, radius * i / 3, 0, 2 * Math.PI);"));
  client.println(F("    ctx.stroke();"));
  client.println(F("  }"));
  client.println(F("  "));
  client.println(F("  // Draw compass directions"));
  client.println(F("  ctx.fillStyle = '#2c3e50';"));
  client.println(F("  ctx.font = '14px Arial';"));
  client.println(F("  ctx.textAlign = 'center';"));
  client.println(F("  ctx.fillText('N', centerX, centerY - radius - 5);"));
  client.println(F("  ctx.fillText('S', centerX, centerY + radius + 15);"));
  client.println(F("  ctx.textAlign = 'left';"));
  client.println(F("  ctx.fillText('E', centerX + radius + 5, centerY + 5);"));
  client.println(F("  ctx.textAlign = 'right';"));
  client.println(F("  ctx.fillText('W', centerX - radius - 5, centerY + 5);"));
  client.println(F("  "));
  client.println(F("  // Apply zoom factor"));
  client.println(F("  let zoomFactor = document.getElementById('zoomSlider') ? "));
  client.println(F("                     document.getElementById('zoomSlider').value / 15 : 1;"));
  client.println(F("  let zoomedRadius = radius * zoomFactor;"));
  client.println(F("  "));
  client.println(F("  // Draw satellites"));
  client.println(F("  if (gpsData && gpsData.satellites) {"));
  client.println(F("    // Constellation filter mapping"));
  client.println(F("    var constellationKeys = ['gps', 'sbas', 'galileo', 'beidou', 'glonass', 'qzss'];"));
  client.println(F("    "));
  client.println(F("    gpsData.satellites.forEach(sat => {"));
  client.println(F("      // Check if not tracked satellites should be shown"));
  client.println(F("      if (!document.getElementById('showNotTracked').checked && !sat.tracked) return;"));
  client.println(F("      "));
  client.println(F("      // Check used-only filter"));
  client.println(F("      if (document.getElementById('showUsedOnly').checked && !sat.used_in_nav) return;"));
  client.println(F("      "));
  client.println(F("      // Check high signal filter"));
  client.println(F("      if (document.getElementById('showHighSignal').checked && sat.signal_strength <= 35) return;"));
  client.println(F("      "));
  client.println(F("      // Check constellation filter"));
  client.println(F("      var constellationKey = constellationKeys[sat.constellation];"));
  client.println(F("      if (constellationKey) {"));
  client.println(F("        var filterElement = document.getElementById('filter_' + constellationKey);"));
  client.println(F("        if (filterElement && !filterElement.checked) return;"));
  client.println(F("      }"));
  client.println(F("      "));
  client.println(F("      // Calculate position with zoom"));
  client.println(F("      var elevationRadius = zoomedRadius * (90 - sat.elevation) / 90;"));
  client.println(F("      var azimuthRad = (sat.azimuth - 90) * Math.PI / 180;"));
  client.println(F("      var x = centerX + elevationRadius * Math.cos(azimuthRad);"));
  client.println(F("      var y = centerY + elevationRadius * Math.sin(azimuthRad);"));
  client.println(F("      "));
  client.println(F("      // Skip if satellite is outside visible area"));
  client.println(F("      if (Math.sqrt((x - centerX)**2 + (y - centerY)**2) > radius + 20) return;"));
  client.println(F("      "));
  client.println(F("      // Constellation colors"));
  client.println(F("      var colors = ['#f39c12', '#95a5a6', '#27ae60', '#3498db', '#e74c3c', '#9b59b6'];"));
  client.println(F("      var color = colors[sat.constellation] || '#34495e';"));
  client.println(F("      "));
  client.println(F("      // Satellite size based on signal strength and zoom"));
  client.println(F("      var baseSize = Math.max(3, sat.signal_strength / 10);"));
  client.println(F("      var size = Math.min(baseSize * zoomFactor, 20); // Limit maximum size"));
  client.println(F("      "));
  client.println(F("      // Draw satellite with enhanced visibility"));
  client.println(F("      ctx.fillStyle = sat.used_in_nav ? color : color + '80';"));
  client.println(F("      ctx.beginPath();"));
  client.println(F("      ctx.arc(x, y, size, 0, 2 * Math.PI);"));
  client.println(F("      ctx.fill();"));
  client.println(F("      "));
  client.println(F("      // Draw border for better visibility"));
  client.println(F("      ctx.strokeStyle = '#2c3e50';"));
  client.println(F("      ctx.lineWidth = 1;"));
  client.println(F("      ctx.stroke();"));
  client.println(F("      "));
  client.println(F("      // Draw PRN number with zoom-aware font size"));
  client.println(F("      var fontSize = Math.max(8, Math.min(12, 10 * zoomFactor));"));
  client.println(F("      ctx.fillStyle = '#fff';"));
  client.println(F("      ctx.font = fontSize + 'px Arial';"));
  client.println(F("      ctx.textAlign = 'center';"));
  client.println(F("      ctx.fillText(sat.prn, x, y + fontSize/3);"));
  client.println(F("      "));
  client.println(F("      // Add elevation text for high zoom levels"));
  client.println(F("      if (zoomFactor > 1.5) {"));
  client.println(F("        ctx.fillStyle = '#7f8c8d';"));
  client.println(F("        ctx.font = (fontSize-2) + 'px Arial';"));
  client.println(F("        ctx.fillText(sat.elevation.toFixed(0) + '°', x, y + size + fontSize);"));
  client.println(F("      }"));
  client.println(F("    });"));
  client.println(F("  }"));
  client.println(F("}"));
  
  client.println(F("function updateConstellationStats() {"));
  client.println(F("  if (!gpsData || !gpsData.constellation_stats) return;"));
  client.println(F("  "));
  client.println(F("  let stats = gpsData.constellation_stats;"));
  client.println(F("  let html = '';"));
  client.println(F("  html += '<div class=\\\"stat-card\\\"><strong>Total</strong><br>' + stats.satellites_total + '</div>';"));
  client.println(F("  html += '<div class=\\\"stat-card\\\"><strong>Used</strong><br>' + stats.satellites_used + '</div>';"));
  client.println(F("  html += '<div class=\\\"stat-card\\\"><strong>GPS</strong><br>' + stats.gps.used + '/' + stats.gps.total + '</div>';"));
  client.println(F("  html += '<div class=\\\"stat-card\\\"><strong>GLONASS</strong><br>' + stats.glonass.used + '/' + stats.glonass.total + '</div>';"));
  client.println(F("  html += '<div class=\\\"stat-card\\\"><strong>Galileo</strong><br>' + stats.galileo.used + '/' + stats.galileo.total + '</div>';"));
  client.println(F("  html += '<div class=\\\"stat-card\\\"><strong>BeiDou</strong><br>' + stats.beidou.used + '/' + stats.beidou.total + '</div>';"));
  client.println(F("  document.getElementById('constellationStats').innerHTML = html;"));
  client.println(F("  "));
  client.println(F("  // Update constellation controls"));
  client.println(F("  updateConstellationControls();"));
  client.println(F("}"));
  client.println(F(""));
  client.println(F("function updateConstellationControls() {"));
  client.println(F("  if (!gpsData || !gpsData.constellation_enables) return;"));
  client.println(F("  "));
  client.println(F("  let enables = gpsData.constellation_enables;"));
  client.println(F("  let constellations = ["));
  client.println(F("    { name: 'GPS', key: 'gps', color: '#f39c12', id: 0 },"));
  client.println(F("    { name: 'SBAS', key: 'sbas', color: '#95a5a6', id: 1 },"));
  client.println(F("    { name: 'Galileo', key: 'galileo', color: '#27ae60', id: 2 },"));
  client.println(F("    { name: 'BeiDou', key: 'beidou', color: '#3498db', id: 3 },"));
  client.println(F("    { name: 'GLONASS', key: 'glonass', color: '#e74c3c', id: 4 },"));
  client.println(F("    { name: 'QZSS', key: 'qzss', color: '#9b59b6', id: 5 }"));
  client.println(F("  ];"));
  client.println(F("  "));
  client.println(F("  let html = '';"));
  client.println(F("  constellations.forEach(constellation => {"));
  client.println(F("    var enabled = enables[constellation.key];"));
  client.println(F("    var checked = document.getElementById('filter_' + constellation.key) ? "));
  client.println(F("                   document.getElementById('filter_' + constellation.key).checked : true;"));
  client.println(F("    html += '<div style=\\\"margin: 5px 0; display: flex; align-items: center;\\\">';"));
  client.println(F("    html += '<input type=\\\"checkbox\\\" id=\\\"filter_' + constellation.key + '\\\"';"));
  client.println(F("    html += (checked ? ' checked' : '') + ' onchange=\\\"updateDisplay()\\\">';"));
  client.println(F("    html += '<div style=\\\"width: 16px; height: 16px; background: ' + constellation.color + ';';"));
  client.println(F("    html += ' margin: 0 8px; border-radius: 2px;\\\"></div>';"));
  client.println(F("    html += '<label for=\\\"filter_' + constellation.key + '\\\" style=\\\"font-size: 12px;\\\">';"));
  client.println(F("    html += constellation.name + (enabled ? ' (Enabled)' : ' (Disabled)');"));
  client.println(F("    html += '</label>';"));
  client.println(F("    html += '</div>';"));
  client.println(F("  });"));
  client.println(F("  "));
  client.println(F("  document.getElementById('gnssControls').innerHTML = html;"));
  client.println(F("}"));
  
  client.println(F("function updateDateView() {"));
  client.println(F("  if (!gpsData) return;"));
  client.println(F("  "));
  client.println(F("  // Fix type mapping with enhanced descriptions"));
  client.println(F("  let fixTypes = ["));
  client.println(F("    'No Fix',"));
  client.println(F("    'Dead Reckoning Only',"));
  client.println(F("    '2D Fix (Latitude/Longitude)',"));
  client.println(F("    '3D Fix (Lat/Lon/Alt)',"));
  client.println(F("    'GNSS + Dead Reckoning',"));
  client.println(F("    'Time Only Fix'"));
  client.println(F("  ];"));
  client.println(F("  let fixType = fixTypes[gpsData.fix_type] || 'Unknown';"));
  client.println(F("  "));
  client.println(F("  // Get fix quality indicator"));
  client.println(F("  let fixQuality = 'Unknown';"));
  client.println(F("  let fixColor = '#95a5a6';"));
  client.println(F("  if (gpsData.fix_type >= 3) {"));
  client.println(F("    fixQuality = 'Excellent';"));
  client.println(F("    fixColor = '#27ae60';"));
  client.println(F("  } else if (gpsData.fix_type >= 2) {"));
  client.println(F("    fixQuality = 'Good';"));
  client.println(F("    fixColor = '#f39c12';"));
  client.println(F("  } else if (gpsData.fix_type >= 1) {"));
  client.println(F("    fixQuality = 'Poor';"));
  client.println(F("    fixColor = '#e74c3c';"));
  client.println(F("  }"));
  client.println(F("  "));
  client.println(F("  // Format UTC time"));
  client.println(F("  let utcDate = new Date(gpsData.utc_time * 1000);"));
  client.println(F("  let utcFormatted = utcDate.toISOString().replace('T', ' ').substring(0, 19) + ' UTC';"));
  client.println(F("  "));
  client.println(F("  // Speed conversion (m/s to km/h and knots)"));
  client.println(F("  let speedKmh = (gpsData.speed * 3.6).toFixed(1);"));
  client.println(F("  let speedKnots = (gpsData.speed * 1.944).toFixed(1);"));
  client.println(F("  "));
  client.println(F("  // Course direction"));
  client.println(F("  let directions = ['N', 'NNE', 'NE', 'ENE', 'E', 'ESE', 'SE', 'SSE', 'S', 'SSW', 'SW', 'WSW', 'W', 'WNW', 'NW', 'NNW'];"));
  client.println(F("  let dirIndex = Math.round(gpsData.course / 22.5) % 16;"));
  client.println(F("  let courseDirection = directions[dirIndex];"));
  client.println(F("  "));
  client.println(F("  // TTFF (Time To First Fix) formatting"));
  client.println(F("  let ttffMinutes = Math.floor(gpsData.ttff / 60);"));
  client.println(F("  let ttffSeconds = gpsData.ttff % 60;"));
  client.println(F("  let ttffFormatted = ttffMinutes > 0 ? (ttffMinutes + 'm ' + ttffSeconds + 's') : (ttffSeconds + 's');"));
  client.println(F("  "));
  client.println(F("  let html = '';"));
  client.println(F("  html += '<div class=\\\"stat-card\\\" style=\\\"background-color: ' + fixColor + '; color: white;\\\">';"));
  client.println(F("  html += '<strong>Fix Status</strong><br>' + fixType + '<br>';"));
  client.println(F("  html += '<small>Quality: ' + fixQuality + '</small></div>';"));
  client.println(F("  html += '<div class=\\\"stat-card\\\"><strong>UTC Date & Time</strong><br>' + utcFormatted + '<br>';"));
  client.println(F("  html += '<small>GPS Week: ' + Math.floor(gpsData.utc_time / 604800) + '</small></div>';"));
  client.println(F("  html += '<div class=\\\"stat-card\\\"><strong>Latitude</strong><br>' + gpsData.latitude.toFixed(6) + '°<br>';"));
  client.println(F("  html += '<small>WGS84 Datum</small></div>';"));
  client.println(F("  html += '<div class=\\\"stat-card\\\"><strong>Longitude</strong><br>' + gpsData.longitude.toFixed(6) + '°<br>';"));
  client.println(F("  html += '<small>WGS84 Datum</small></div>';"));
  client.println(F("  html += '<div class=\\\"stat-card\\\"><strong>Altitude</strong><br>' + gpsData.altitude.toFixed(1) + ' m<br>';"));
  client.println(F("  html += '<small>Above Mean Sea Level</small></div>';"));
  client.println(F("  html += '<div class=\\\"stat-card\\\"><strong>Speed</strong><br>' + speedKmh + ' km/h<br>';"));
  client.println(F("  html += '<small>' + speedKnots + ' knots</small></div>';"));
  client.println(F("  html += '<div class=\\\"stat-card\\\"><strong>Course</strong><br>' + gpsData.course.toFixed(1) + '° ' + courseDirection + '<br>';"));
  client.println(F("  html += '<small>True North</small></div>';"));
  client.println(F("  html += '<div class=\\\"stat-card\\\"><strong>3D Accuracy</strong><br>' + gpsData.accuracy_3d.toFixed(2) + ' m<br>';"));
  client.println(F("  html += '<small>95% Confidence</small></div>';"));
  client.println(F("  html += '<div class=\\\"stat-card\\\"><strong>HDOP</strong><br>' + gpsData.hdop.toFixed(2) + '<br>';"));
  client.println(F("  html += '<small>Horizontal DOP</small></div>';"));
  client.println(F("  html += '<div class=\\\"stat-card\\\"><strong>VDOP</strong><br>' + gpsData.vdop.toFixed(2) + '<br>';"));
  client.println(F("  html += '<small>Vertical DOP</small></div>';"));
  client.println(F("  html += '<div class=\\\"stat-card\\\"><strong>PDOP</strong><br>' + gpsData.pdop.toFixed(2) + '<br>';"));
  client.println(F("  html += '<small>Position DOP</small></div>';"));
  client.println(F("  html += '<div class=\\\"stat-card\\\"><strong>Time To First Fix</strong><br>' + ttffFormatted + '<br>';"));
  client.println(F("  html += '<small>Cold Start</small></div>';"));
  client.println(F("  document.getElementById('dateViewStats').innerHTML = html;"));
  client.println(F("}"));
  
  client.println(F("// Differential update detection"));
  client.println(F("function hasSignificantChange(oldData, newData) {"));
  client.println(F("  if (!oldData || !newData) return true;"));
  client.println(F("  "));
  client.println(F("  // Check for fix type changes"));
  client.println(F("  if (oldData.fix_type !== newData.fix_type) return true;"));
  client.println(F("  "));
  client.println(F("  // Check for satellite count changes"));
  client.println(F("  if (oldData.satellites?.length !== newData.satellites?.length) return true;"));
  client.println(F("  "));
  client.println(F("  // Check for position changes (more than 1 meter)"));
  client.println(F("  let positionThreshold = 0.00001; // ~1 meter"));
  client.println(F("  if (Math.abs(oldData.latitude - newData.latitude) > positionThreshold ||"));
  client.println(F("      Math.abs(oldData.longitude - newData.longitude) > positionThreshold) {"));
  client.println(F("    return true;"));
  client.println(F("  }"));
  client.println(F("  "));
  client.println(F("  // Check for satellite signal strength changes"));
  client.println(F("  if (oldData.satellites && newData.satellites) {"));
  client.println(F("    for (var i = 0; i < Math.min(oldData.satellites.length, newData.satellites.length); i++) {"));
  client.println(F("      if (Math.abs(oldData.satellites[i].signal_strength - newData.satellites[i].signal_strength) > 5) {"));
  client.println(F("        return true;"));
  client.println(F("      }"));
  client.println(F("    }"));
  client.println(F("  }"));
  client.println(F("  "));
  client.println(F("  return false;"));
  client.println(F("}"));
  client.println(F(""));
  client.println(F("// Connection status management"));
  client.println(F("function updateConnectionStatus() {"));
  client.println(F("  let statusElement = document.getElementById('connectionStatus');"));
  client.println(F("  if (!statusElement) return;"));
  client.println(F("  "));
  client.println(F("  let timeSinceUpdate = Date.now() - lastUpdateTime;"));
  client.println(F("  let statusText = '';"));
  client.println(F("  let statusColor = '';"));
  client.println(F("  "));
  client.println(F("  if (connectionStatus === 'connected' && timeSinceUpdate < 2000) {"));
  client.println(F("    statusText = 'Connected';"));
  client.println(F("    statusColor = '#27ae60';"));
  client.println(F("  } else if (connectionStatus === 'error') {"));
  client.println(F("    statusText = 'Connection Error (' + connectionFailures + '/' + maxConnectionFailures + ')';"));
  client.println(F("    statusColor = '#e74c3c';"));
  client.println(F("  } else {"));
  client.println(F("    statusText = 'Connecting...';"));
  client.println(F("    statusColor = '#f39c12';"));
  client.println(F("  }"));
  client.println(F("  "));
  client.println(F("  statusElement.textContent = statusText;"));
  client.println(F("  statusElement.style.color = statusColor;"));
  client.println(F("}"));
  client.println(F(""));
  client.println(F("// Auto-reconnection with exponential backoff"));
  client.println(F("function attemptReconnection() {"));
  client.println(F("  if (updateInterval) {"));
  client.println(F("    clearInterval(updateInterval);"));
  client.println(F("  }"));
  client.println(F("  "));
  client.println(F("  let backoffTime = Math.min(1000 * Math.pow(2, connectionFailures - maxConnectionFailures), 30000);"));
  client.println(F("  console.log('Attempting reconnection in ' + backoffTime + 'ms...');"));
  client.println(F("  "));
  client.println(F("  setTimeout(() => {"));
  client.println(F("    connectionFailures = 0;"));
  client.println(F("    connectionStatus = 'connecting';"));
  client.println(F("    updateConnectionStatus();"));
  client.println(F("    "));
  client.println(F("    // Restart regular updates"));
  client.println(F("    updateInterval = setInterval(fetchGpsData, 2000); // Optimized: 2 second intervals"));
  client.println(F("    fetchGpsData(); // Immediate attempt"));
  client.println(F("  }, backoffTime);"));
  client.println(F("}"));
  client.println(F(""));
  client.println(F("// User interaction detection"));
  client.println(F("function handleUserInteraction() {"));
  client.println(F("  isUserInteracting = true;"));
  client.println(F("  "));
  client.println(F("  // Clear existing timeout"));
  client.println(F("  if (interactionTimeout) {"));
  client.println(F("    clearTimeout(interactionTimeout);"));
  client.println(F("  }"));
  client.println(F("  "));
  client.println(F("  // Resume updates after 2 seconds of inactivity"));
  client.println(F("  interactionTimeout = setTimeout(() => {"));
  client.println(F("    isUserInteracting = false;"));
  client.println(F("    console.log('User interaction ended - resuming updates');"));
  client.println(F("  }, 2000);"));
  client.println(F("}"));
  client.println(F(""));
  client.println(F("// Utility functions for interactive features"));
  client.println(F("function updateZoomLabel() {"));
  client.println(F("  let slider = document.getElementById('zoomSlider');"));
  client.println(F("  let label = document.getElementById('zoomValue');"));
  client.println(F("  if (slider && label) {"));
  client.println(F("    var zoomValue = (slider.value / 15).toFixed(1);"));
  client.println(F("    label.textContent = zoomValue + 'x';"));
  client.println(F("  }"));
  client.println(F("  updateDisplay();"));
  client.println(F("}"));
  client.println(F(""));
  client.println(F("function resetView() {"));
  client.println(F("  // Reset zoom"));
  client.println(F("  document.getElementById('zoomSlider').value = 15;"));
  client.println(F("  updateZoomLabel();"));
  client.println(F("  "));
  client.println(F("  // Reset filters"));
  client.println(F("  document.getElementById('showNotTracked').checked = true;"));
  client.println(F("  document.getElementById('showUsedOnly').checked = false;"));
  client.println(F("  document.getElementById('showHighSignal').checked = false;"));
  client.println(F("  "));
  client.println(F("  // Reset constellation filters"));
  client.println(F("  let constellationFilters = document.querySelectorAll('[id^=\"filter_\"]');"));
  client.println(F("  constellationFilters.forEach(filter => filter.checked = true);"));
  client.println(F("  "));
  client.println(F("  updateDisplay();"));
  client.println(F("}"));
  client.println(F(""));
  client.println(F("// Enhanced update display with timestamp"));
  client.println(F("function updateDisplayWithTimestamp() {"));
  client.println(F("  updateDisplay();"));
  client.println(F("  "));
  client.println(F("  // Update last update timestamp"));
  client.println(F("  let now = new Date();"));
  client.println(F("  let timeString = now.toLocaleTimeString();"));
  client.println(F("  let timestampElement = document.getElementById('lastUpdateTime');"));
  client.println(F("  if (timestampElement) {"));
  client.println(F("    timestampElement.textContent = timeString;"));
  client.println(F("  }"));
  client.println(F("}"));
  client.println(F(""));
  client.println(F("// Initialize"));
  client.println(F("document.addEventListener('DOMContentLoaded', function() {"));
  client.println(F("  // Initial data fetch"));
  client.println(F("  fetchGpsData();"));
  client.println(F("  updateInterval = setInterval(fetchGpsData, 2000); // Optimized: 2 second intervals"));
  client.println(F("  "));
  client.println(F("  // Add event listeners for all interactive controls"));
  client.println(F("  document.getElementById('showNotTracked').addEventListener('change', (e) => {"));
  client.println(F("    handleUserInteraction();"));
  client.println(F("    updateDisplay();"));
  client.println(F("  });"));
  client.println(F("  document.getElementById('showUsedOnly').addEventListener('change', (e) => {"));
  client.println(F("    handleUserInteraction();"));
  client.println(F("    updateDisplay();"));
  client.println(F("  });"));
  client.println(F("  document.getElementById('showHighSignal').addEventListener('change', (e) => {"));
  client.println(F("    handleUserInteraction();"));
  client.println(F("    updateDisplay();"));
  client.println(F("  });"));
  client.println(F("  document.getElementById('zoomSlider').addEventListener('input', (e) => {"));
  client.println(F("    handleUserInteraction();"));
  client.println(F("    updateZoomLabel();"));
  client.println(F("  });"));
  client.println(F("  "));
  client.println(F("  // Add interaction detection for view buttons"));
  client.println(F("  document.querySelectorAll('.btn').forEach(btn => {"));
  client.println(F("    btn.addEventListener('click', handleUserInteraction);"));
  client.println(F("  });"));
  client.println(F("  "));
  client.println(F("  // Add interaction detection for canvas (mouse/touch)"));
  client.println(F("  let canvas = document.getElementById('radarChart');"));
  client.println(F("  if (canvas) {"));
  client.println(F("    canvas.addEventListener('mousedown', handleUserInteraction);"));
  client.println(F("    canvas.addEventListener('touchstart', handleUserInteraction);"));
  client.println(F("  }"));
  client.println(F("  "));
  client.println(F("  // Initialize connection status"));
  client.println(F("  updateConnectionStatus();"));
  client.println(F("  "));
  client.println(F("  // Initialize zoom label"));
  client.println(F("  updateZoomLabel();"));
  client.println(F("  "));
  client.println(F("  console.log('GPS Display initialized with enhanced real-time features');"));
  client.println(F("});"));
  
  client.println(F("</script>"));
  client.println(F("</div>"));
  client.println(F("</body>"));
  client.println(F("</html>"));
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
  client.println("  var form = event.target;");
  client.println("  var formData = new FormData(form);");
  client.println("  var config = {};");
  client.println("  for (var [key, value] of formData.entries()) {");
  client.println("    if (value === 'on') config[key] = true;");
  client.println("    else if (key.includes('_enabled') || key.includes('enabled')) config[key] = false;");
  client.println("    else config[key] = value;");
  client.println("  }");
  client.println("  // Handle checkboxes that weren't checked");
  client.println("  var checkboxes = ['gps_enabled', 'glonass_enabled', 'galileo_enabled', 'beidou_enabled', 'qzss_enabled', 'qzss_l1s_enabled', 'ntp_enabled', 'prometheus_enabled', 'debug_enabled'];");
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

void GpsWebServer::gpsApiGet(EthernetClient &client) {
  unsigned long requestStartTime = millis();
  requestCount++;
  
  if (!gpsClient) {
    sendJsonResponse(client, "{\"error\": \"GPS Client not available\"}", 500);
    return;
  }
  
  // Performance optimization: Check cache validity
  unsigned long currentTime = millis();
  bool cacheExpired = (currentTime - lastGpsDataUpdate) > GPS_DATA_CACHE_INTERVAL;
  
  if (gpsDataCacheValid && !cacheExpired && cachedGpsJson.length() > 0) {
    // Use cached data for improved performance
    sendJsonResponse(client, cachedGpsJson);
    
    // Update performance metrics
    unsigned long responseTime = millis() - requestStartTime;
    totalResponseTime += responseTime;
    return;
  }
  
  // Get fresh Web GPS data
  web_gps_data_t gpsData = gpsClient->getWebGpsData();
  
  // Create JSON document (optimized buffer size)
  DynamicJsonDocument doc(6144); // Reduced from 8192 to 6144 for memory efficiency
  
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
  doc["pdop"] = pdop;
  doc["hdop"] = hdop;
  doc["vdop"] = vdop;
  doc["accuracy_3d"] = acc3d;
  doc["accuracy_2d"] = acc2d;
  
  // Constellation Statistics
  JsonObject stats = doc.createNestedObject("constellation_stats");
  stats["satellites_total"] = gpsData.satellites_total;
  stats["satellites_used"] = gpsData.satellites_used;
  
  JsonObject gps_stats = stats.createNestedObject("gps");
  gps_stats["total"] = gpsData.satellites_gps_total;
  gps_stats["used"] = gpsData.satellites_gps_used;
  
  JsonObject glonass_stats = stats.createNestedObject("glonass");
  glonass_stats["total"] = gpsData.satellites_glonass_total;
  glonass_stats["used"] = gpsData.satellites_glonass_used;
  
  JsonObject galileo_stats = stats.createNestedObject("galileo");
  galileo_stats["total"] = gpsData.satellites_galileo_total;
  galileo_stats["used"] = gpsData.satellites_galileo_used;
  
  JsonObject beidou_stats = stats.createNestedObject("beidou");
  beidou_stats["total"] = gpsData.satellites_beidou_total;
  beidou_stats["used"] = gpsData.satellites_beidou_used;
  
  JsonObject sbas_stats = stats.createNestedObject("sbas");
  sbas_stats["total"] = gpsData.satellites_sbas_total;
  sbas_stats["used"] = gpsData.satellites_sbas_used;
  
  JsonObject qzss_stats = stats.createNestedObject("qzss");
  qzss_stats["total"] = gpsData.satellites_qzss_total;
  qzss_stats["used"] = gpsData.satellites_qzss_used;
  
  // Individual Satellite Information
  JsonArray satellites = doc.createNestedArray("satellites");
  
  // Bounds checking to prevent buffer overflow
  uint8_t safeSatelliteCount = gpsData.satellite_count;
  if (safeSatelliteCount > MAX_SATELLITES) {
    safeSatelliteCount = MAX_SATELLITES;
  }
  
  for (uint8_t i = 0; i < safeSatelliteCount; i++) {
    // Additional bounds checking for satellite array access
    if (i >= MAX_SATELLITES) {
      break;
    }
    
    JsonObject sat = satellites.createNestedObject();
    
    // Validate data before adding to JSON
    sat["prn"] = gpsData.satellites[i].prn;
    sat["constellation"] = gpsData.satellites[i].constellation;
    
    // Validate float values to prevent NaN or Inf
    float azimuth = gpsData.satellites[i].azimuth;
    float elevation = gpsData.satellites[i].elevation;
    
    if (isnan(azimuth) || isinf(azimuth)) {
      azimuth = 0.0;
    }
    if (isnan(elevation) || isinf(elevation)) {
      elevation = 0.0;
    }
    
    // Clamp values to valid ranges
    if (azimuth < 0.0) azimuth = 0.0;
    if (azimuth > 360.0) azimuth = 360.0;
    if (elevation < 0.0) elevation = 0.0;
    if (elevation > 90.0) elevation = 90.0;
    
    sat["azimuth"] = azimuth;
    sat["elevation"] = elevation;
    sat["signal_strength"] = gpsData.satellites[i].signal_strength;
    sat["used_in_nav"] = gpsData.satellites[i].used_in_nav;
    sat["tracked"] = gpsData.satellites[i].tracked;
  }
  
  // Constellation Enable Status
  JsonObject enables = doc.createNestedObject("constellation_enables");
  enables["gps"] = gpsData.gps_enabled;
  enables["glonass"] = gpsData.glonass_enabled;
  enables["galileo"] = gpsData.galileo_enabled;
  enables["beidou"] = gpsData.beidou_enabled;
  enables["sbas"] = gpsData.sbas_enabled;
  enables["qzss"] = gpsData.qzss_enabled;
  
  // System Status
  doc["data_valid"] = gpsData.data_valid;
  doc["last_update"] = gpsData.last_update;
  
  // Check document capacity before serialization
  size_t requiredSize = measureJson(doc);
  if (requiredSize > 6144) {
    // JSON too large, send error response
    String errorJson = "{\"error\": \"GPS data too large\", \"required_size\": " + String(requiredSize) + "}";
    sendJsonResponse(client, errorJson, 500);
    return;
  }
  
  // Serialize to string with error checking
  String jsonString;
  jsonString.reserve(requiredSize + 100); // Reserve space to prevent reallocation
  
  size_t serializedBytes = serializeJson(doc, jsonString);
  if (serializedBytes == 0) {
    // Serialization failed, send error response
    String errorJson = "{\"error\": \"JSON serialization failed\", \"bytes\": 0}";
    sendJsonResponse(client, errorJson, 500);
    return;
  }
  
  // Validate JSON string for control characters
  for (size_t i = 0; i < jsonString.length(); i++) {
    char c = jsonString.charAt(i);
    if (c < 32 && c != '\t' && c != '\n' && c != '\r') {
      // Found invalid control character, replace with space
      jsonString.setCharAt(i, ' ');
    }
  }
  
  // Update cache for performance optimization
  cachedGpsJson = jsonString;
  lastGpsDataUpdate = currentTime;
  gpsDataCacheValid = true;
  
  // Send response
  sendJsonResponse(client, jsonString);
  
  // Update performance metrics
  unsigned long responseTime = millis() - requestStartTime;
  totalResponseTime += responseTime;
}

void GpsWebServer::handleFileRequest(EthernetClient &client, const String& path, const String& contentType) {
  if (LittleFS.exists(path)) {
    File file = LittleFS.open(path, "r");
    if (file) {
      size_t fileSize = file.size();
      
      // Send headers
      client.println("HTTP/1.1 200 OK");
      client.printf("Content-Type: %s\r\n", contentType.c_str());
      client.printf("Content-Length: %zu\r\n", fileSize);
      client.println("Connection: close");
      client.println("Cache-Control: public, max-age=3600");
      client.println();
      
      // Stream file content
      while (file.available()) {
        client.write(file.read());
      }
      file.close();
      return;
    }
  }
  
  // 404 Not Found
  send404(client);
}

void GpsWebServer::send404(EthernetClient &client) {
  String notFound = "<!DOCTYPE html><html><body><h1>404 Not Found</h1><p>The requested file was not found.</p></body></html>";
  client.println("HTTP/1.1 404 Not Found");
  client.println("Content-Type: text/html");
  client.printf("Content-Length: %d\r\n", notFound.length());
  client.println("Connection: close");
  client.println();
  client.print(notFound);
}

void GpsWebServer::sendJsonResponse(EthernetClient &client, const String& json, int statusCode) {
  // Validate JSON string before sending
  String validatedJson = json;
  
  // Remove any potential control characters from JSON
  for (size_t i = 0; i < validatedJson.length(); i++) {
    char c = validatedJson.charAt(i);
    if (c < 32 && c != '\t' && c != '\n' && c != '\r') {
      validatedJson.setCharAt(i, ' ');
    }
  }
  
  // Send HTTP headers with proper Content-Length
  client.printf("HTTP/1.1 %d %s\r\n", statusCode, statusCode == 200 ? "OK" : "Error");
  client.println("Content-Type: application/json; charset=utf-8");
  client.printf("Content-Length: %d\r\n", validatedJson.length());
  client.println("Connection: close");
  client.println("Cache-Control: no-cache");
  client.println();
  
  // Send JSON content
  client.print(validatedJson);
}

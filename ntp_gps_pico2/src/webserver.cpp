#include <webserver.h>

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

  client.println("<h1>GPS Data</h1>");
  client.println("<a href=\"/gps\">GPS</a>");

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
  
  // Basic Prometheus-style metrics
  client.println("# HELP system_uptime_seconds System uptime in seconds");
  client.println("# TYPE system_uptime_seconds counter");
  client.print("system_uptime_seconds ");
  client.println(millis() / 1000);
  
  client.println("# HELP memory_free_bytes Free memory in bytes"); 
  client.println("# TYPE memory_free_bytes gauge");
  client.print("memory_free_bytes ");
  client.println(524288 - 16880); // Placeholder - actual free memory calculation would be more complex
  
  client.println("# HELP network_connected Network connection status");
  client.println("# TYPE network_connected gauge");
  client.print("network_connected ");
  client.println("1"); // Placeholder - would need network status from main
}
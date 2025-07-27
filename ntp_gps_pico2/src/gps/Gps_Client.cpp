#include "Gps_Client.h"
#include <time.h>
#include <math.h>

byte l1s_msg_buf[32]; // MAX 250 BITS
QZQSM dc_report;
DCXDecoder dcx_decoder;

void GpsClient::getPVTdata(UBX_NAV_PVT_data_t *data)
{
  gpsSummaryData.latitude = data->lat;
  gpsSummaryData.longitude = data->lon;
  gpsSummaryData.altitude = data->hMSL;
  gpsSummaryData.SIV = data->numSV;
  gpsSummaryData.timeValid = data->valid.bits.validTime;
  gpsSummaryData.dateValid = data->valid.bits.validDate;
  gpsSummaryData.year = data->year;
  gpsSummaryData.month = data->month;
  gpsSummaryData.day = data->day;
  gpsSummaryData.hour = data->hour;
  gpsSummaryData.min = data->min;
  gpsSummaryData.sec = data->sec;
  gpsSummaryData.msec = data->iTOW % 1000;
  gpsSummaryData.fixType = data->fixType;
}

// https://github.com/SWITCHSCIENCE/samplecodes/blob/master/GPS_shield_for_ESPr/espr_dev_qzss_drc_drx_decode/espr_dev_qzss_drc_drx_decode.ino
// dwrdを16進数文字列に変換して出力する関数
const char *GpsClient::dwrd_to_str(uint32_t value)
{
  static const char hex_chars[] = "0123456789ABCDEF"; // 16進数文字
  static char buffer[9];                              // 8桁 + 終端文字
  // リトルエンディアンなので入れ替える
  buffer[8] = '\0';
  buffer[7] = hex_chars[value & 0xF];
  buffer[6] = hex_chars[value >> 4 & 0xF];
  buffer[5] = hex_chars[value >> 8 & 0xF];
  buffer[4] = hex_chars[value >> 12 & 0xF];
  buffer[3] = hex_chars[value >> 16 & 0xF];
  buffer[2] = hex_chars[value >> 20 & 0xF];
  buffer[1] = hex_chars[value >> 24 & 0xF];
  buffer[0] = hex_chars[value >> 28 & 0xF];
  return buffer;
}

void GpsClient::newSFRBX(UBX_RXM_SFRBX_data_t *data)
{
#if defined(DEBUG_CONSOLE_GPS)
  stream_.print("SFRBX gnssId: ");
  stream_.print(data->gnssId);
  stream_.print(" svId: ");
  stream_.print(data->svId);
  stream_.print(" freqId: ");
  stream_.print(data->freqId);
  stream_.print(" numWords: ");
  stream_.print(data->numWords);
  stream_.print(" version: ");
  stream_.print(data->version);
  stream_.print(" ");
  for (int i = 0; i < data->numWords; i++)
  {
    stream_.print(dwrd_to_str(data->dwrd[i]));
  }
  stream_.println();
#endif

  // QZSS L1Sメッセージ解析
  if (data->gnssId == 5)
  {

    // SFRBXのdwrdはリトルエンディアンなので入れ替える
    for (int i = 0; i < min(int(data->numWords), 8); i++)
    {
      l1s_msg_buf[(i << 2) + 0] = (data->dwrd[i] >> 24) & 0xff;
      l1s_msg_buf[(i << 2) + 1] = (data->dwrd[i] >> 16) & 0xff;
      l1s_msg_buf[(i << 2) + 2] = (data->dwrd[i] >> 8) & 0xff;
      l1s_msg_buf[(i << 2) + 3] = (data->dwrd[i]) & 0xff;
    }

    byte pab = l1s_msg_buf[0];
    byte mt = l1s_msg_buf[1] >> 2;

    if (pab == 0x53 || pab == 0x9A || pab == 0xC6)
    {
      // Message Typeを表示
      struct
      {
        byte mt;
        const char *desc;
      } MTTable[] = {
          {0, "Test Mode"},
          {43, "DC Report"},
          {44, "DCX message"},
          {47, "Monitoring Station Information"},
          {48, "PRN Mask"},
          {49, "Data Issue Number"},
          {50, "DGPS Correction"},
          {51, "Satellite Health"},
          {63, "Null message"},
      };
      for (int i = 0; i < sizeof(MTTable) / sizeof(MTTable[0]); i++)
      {
        if (MTTable[i].mt == mt)
        {
          stream_.print(mt);
          stream_.print(" ");
          stream_.println(MTTable[i].desc);
          break;
        }
      }
      // 災害・危機管理通報サービス（DC Report）のメッセージ内容を表示
      if (mt == 43)
      {
        dc_report.SetYear(2024); // todo
        dc_report.Decode(l1s_msg_buf);
        stream_.println(dc_report.GetReport());
      }
      // 災害・危機管理通報サービス（拡張）（DCX）のメッセージ内容を表示
      else if (mt == 44)
      {
        dcx_decoder.decode(l1s_msg_buf);
        dcx_decoder.printSummary(stream_, dcx_decoder.r);
        
#if defined(DEBUG_CONSOLE_DCX_ALL)
        dcx_decoder.printAll(stream_, dcx_decoder.r);
#endif
      }
    }
  }
}

void GpsClient::newNAVSAT(UBX_NAV_SAT_data_t *data)
{

  ubxNavSatData_t = data;

  // Web GPS表示用データ処理
  processNavSatData(data);

#if defined(DEBUG_CONSOLE_GPS)
#define NUM_GNSS 7
  int nGNSS[NUM_GNSS] = {0};
  for (uint16_t block = 0; block < data->header.numSvs; block++)
  {
    if (data->blocks[block].gnssId < NUM_GNSS)
    {
      nGNSS[data->blocks[block].gnssId]++;
    }
  }
  stream_.print(F("Satellites: "));
  stream_.print(data->header.numSvs);
  const char *gnssName[] = {"GPS", "SBAS", "Galileo", "BeiDou", "IMES", "QZSS", "GLONASS"};
  for (uint16_t i = 0; i < NUM_GNSS; i++)
  {
    if (nGNSS[i])
    {
      stream_.print(" ");
      stream_.print(gnssName[i]);
      stream_.print(": ");
      stream_.print(nGNSS[i]);
    }
  }
  stream_.println();
#endif
}

// Web GPS表示用データ取得メソッド
web_gps_data_t GpsClient::getWebGpsData() 
{
  webGpsData.last_update = millis();
  return webGpsData;
}

// Web GPS表示用データ更新メソッド
void GpsClient::updateWebGpsData(UBX_NAV_PVT_data_t *pvtData, UBX_NAV_SAT_data_t *satData) 
{
  if (!pvtData) return;
  
  // Position and Time Information
  webGpsData.latitude = pvtData->lat / 10000000.0; // Convert from 1e-7 degrees to degrees
  webGpsData.longitude = pvtData->lon / 10000000.0;
  webGpsData.altitude = pvtData->hMSL / 1000.0; // Convert from mm to meters
  webGpsData.speed = pvtData->gSpeed / 1000.0; // Convert from mm/s to m/s
  webGpsData.course = pvtData->headMot / 100000.0; // Convert from 1e-5 degrees to degrees
  
  // Convert GPS time to Unix timestamp
  // GPS epoch starts Jan 6, 1980, Unix epoch starts Jan 1, 1970
  // Difference is 315964800 seconds
  if (pvtData->valid.bits.validTime && pvtData->valid.bits.validDate) {
    struct tm timeinfo = {0};
    timeinfo.tm_year = pvtData->year - 1900;
    timeinfo.tm_mon = pvtData->month - 1;
    timeinfo.tm_mday = pvtData->day;
    timeinfo.tm_hour = pvtData->hour;
    timeinfo.tm_min = pvtData->min;
    timeinfo.tm_sec = pvtData->sec;
    webGpsData.utc_time = mktime(&timeinfo);
  }
  
  // Fix Information
  webGpsData.fix_type = pvtData->fixType;
  webGpsData.pdop = pvtData->pDOP / 100.0; // Convert from 0.01 to actual value
  webGpsData.hdop = pvtData->hAcc / 1000.0; // Horizontal accuracy in meters
  webGpsData.vdop = pvtData->vAcc / 1000.0; // Vertical accuracy in meters
  webGpsData.accuracy_3d = sqrt(pow(pvtData->hAcc/1000.0, 2) + pow(pvtData->vAcc/1000.0, 2));
  webGpsData.accuracy_2d = pvtData->hAcc / 1000.0;
  
  // Satellite count from PVT
  webGpsData.satellites_used = pvtData->numSV;
  
  // Process satellite data if available
  if (satData) {
    processNavSatData(satData);
  }
  
  // Constellation enable status (default all enabled)
  webGpsData.gps_enabled = true;
  webGpsData.glonass_enabled = true;
  webGpsData.galileo_enabled = true;
  webGpsData.beidou_enabled = true;
  webGpsData.sbas_enabled = true;
  webGpsData.qzss_enabled = true;
  
  // System status
  webGpsData.data_valid = true;
  webGpsData.last_update = millis();
}

// NAV-SATデータ処理メソッド
void GpsClient::processNavSatData(UBX_NAV_SAT_data_t *satData) 
{
  if (!satData) return;
  
  // Reset counters
  resetConstellationStats();
  webGpsData.satellite_count = 0;
  
  // Process each satellite
  for (uint16_t i = 0; i < satData->header.numSvs && i < MAX_SATELLITES; i++) {
    if (webGpsData.satellite_count >= MAX_SATELLITES) break;
    
    satellite_info_t &sat = webGpsData.satellites[webGpsData.satellite_count];
    auto &block = satData->blocks[i];
    
    // Basic satellite information
    sat.prn = block.svId;
    sat.constellation = mapGnssIdToConstellation(block.gnssId);
    sat.signal_strength = block.cno;
    sat.elevation = block.elev;
    sat.azimuth = block.azim;
    sat.used_in_nav = (block.flags.bits.svUsed == 1);
    sat.tracked = (block.flags.bits.qualityInd > 0);
    
    webGpsData.satellite_count++;
  }
  
  // Calculate constellation statistics
  calculateConstellationStats();
  webGpsData.satellites_total = webGpsData.satellite_count;
}

// GNSS ID をコンステレーションタイプにマッピング
uint8_t GpsClient::mapGnssIdToConstellation(uint8_t gnssId) 
{
  switch (gnssId) {
    case 0: return 0; // GPS
    case 1: return 1; // SBAS
    case 2: return 2; // Galileo
    case 3: return 3; // BeiDou
    case 5: return 5; // QZSS
    case 6: return 4; // GLONASS
    default: return 0; // Default to GPS
  }
}

// コンステレーション統計のリセット
void GpsClient::resetConstellationStats() 
{
  webGpsData.satellites_gps_total = 0;
  webGpsData.satellites_gps_used = 0;
  webGpsData.satellites_glonass_total = 0;
  webGpsData.satellites_glonass_used = 0;
  webGpsData.satellites_galileo_total = 0;
  webGpsData.satellites_galileo_used = 0;
  webGpsData.satellites_beidou_total = 0;
  webGpsData.satellites_beidou_used = 0;
  webGpsData.satellites_sbas_total = 0;
  webGpsData.satellites_sbas_used = 0;
  webGpsData.satellites_qzss_total = 0;
  webGpsData.satellites_qzss_used = 0;
}

// コンステレーション統計の計算
void GpsClient::calculateConstellationStats() 
{
  uint8_t used_count = 0;
  
  for (uint8_t i = 0; i < webGpsData.satellite_count; i++) {
    satellite_info_t &sat = webGpsData.satellites[i];
    
    switch (sat.constellation) {
      case 0: // GPS
        webGpsData.satellites_gps_total++;
        if (sat.used_in_nav) {
          webGpsData.satellites_gps_used++;
          used_count++;
        }
        break;
      case 1: // SBAS
        webGpsData.satellites_sbas_total++;
        if (sat.used_in_nav) {
          webGpsData.satellites_sbas_used++;
          used_count++;
        }
        break;
      case 2: // Galileo
        webGpsData.satellites_galileo_total++;
        if (sat.used_in_nav) {
          webGpsData.satellites_galileo_used++;
          used_count++;
        }
        break;
      case 3: // BeiDou
        webGpsData.satellites_beidou_total++;
        if (sat.used_in_nav) {
          webGpsData.satellites_beidou_used++;
          used_count++;
        }
        break;
      case 4: // GLONASS
        webGpsData.satellites_glonass_total++;
        if (sat.used_in_nav) {
          webGpsData.satellites_glonass_used++;
          used_count++;
        }
        break;
      case 5: // QZSS
        webGpsData.satellites_qzss_total++;
        if (sat.used_in_nav) {
          webGpsData.satellites_qzss_used++;
          used_count++;
        }
        break;
    }
  }
  
  webGpsData.satellites_used = used_count;
}

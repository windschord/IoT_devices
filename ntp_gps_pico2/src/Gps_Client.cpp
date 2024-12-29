#include <Gps_Client.h>

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

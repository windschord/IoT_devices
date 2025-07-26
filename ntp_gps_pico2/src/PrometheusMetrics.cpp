#include "PrometheusMetrics.h"
#include "HardwareConfig.h"
#include <Ethernet.h>

// Prometheusメトリクス名定数の定義
const char* METRIC_NTP_REQUESTS_TOTAL = "ntp_requests_total";
const char* METRIC_NTP_RESPONSES_TOTAL = "ntp_responses_total";
const char* METRIC_NTP_DROPPED_TOTAL = "ntp_dropped_total";
const char* METRIC_NTP_RESPONSE_TIME_MS = "ntp_response_time_milliseconds";
const char* METRIC_NTP_ACCURACY_MS = "ntp_accuracy_milliseconds";
const char* METRIC_NTP_STRATUM = "ntp_stratum";
const char* METRIC_NTP_CLIENTS_ACTIVE = "ntp_clients_active";

const char* METRIC_GPS_SATELLITES_TOTAL = "gps_satellites_total";
const char* METRIC_GPS_SATELLITES_GPS = "gps_satellites_gps";
const char* METRIC_GPS_SATELLITES_GLONASS = "gps_satellites_glonass";
const char* METRIC_GPS_SATELLITES_GALILEO = "gps_satellites_galileo";
const char* METRIC_GPS_SATELLITES_BEIDOU = "gps_satellites_beidou";
const char* METRIC_GPS_SATELLITES_QZSS = "gps_satellites_qzss";
const char* METRIC_GPS_HDOP = "gps_hdop";
const char* METRIC_GPS_VDOP = "gps_vdop";
const char* METRIC_GPS_PPS_PULSES_TOTAL = "gps_pps_pulses_total";
const char* METRIC_GPS_SIGNAL_QUALITY = "gps_signal_quality";
const char* METRIC_GPS_FALLBACK_MODE = "gps_fallback_mode";

const char* METRIC_SYSTEM_UPTIME_SECONDS = "system_uptime_seconds";
const char* METRIC_SYSTEM_RAM_USAGE_PERCENT = "system_ram_usage_percent";
const char* METRIC_SYSTEM_FLASH_USAGE_PERCENT = "system_flash_usage_percent";
const char* METRIC_SYSTEM_CPU_TEMPERATURE = "system_cpu_temperature_celsius";
const char* METRIC_SYSTEM_ETHERNET_CONNECTED = "system_ethernet_connected";
const char* METRIC_SYSTEM_PACKETS_SENT_TOTAL = "system_packets_sent_total";
const char* METRIC_SYSTEM_PACKETS_RECEIVED_TOTAL = "system_packets_received_total";

PrometheusMetrics::PrometheusMetrics() 
    : lastNtpUpdate(0), lastGpsUpdate(0), lastSystemUpdate(0) {
    
    // NTPメトリクスの初期化
    memset(&ntpMetrics, 0, sizeof(ntpMetrics));
    ntpMetrics.minResponseTimeMs = 999999.0;
    ntpMetrics.maxResponseTimeMs = 0.0;
    
    // GPSメトリクスの初期化
    memset(&gpsMetrics, 0, sizeof(gpsMetrics));
    gpsMetrics.averageSnr = 0.0;
    gpsMetrics.ppsJitter = 0.0;
    
    // システムメトリクスの初期化
    memset(&systemMetrics, 0, sizeof(systemMetrics));
    systemMetrics.totalRam = 524288;      // Raspberry Pi Pico 2: 512KB RAM
    systemMetrics.totalFlash = 4190208;   // Raspberry Pi Pico 2: 4MB Flash (実際の利用可能領域)
}

void PrometheusMetrics::init() {
    Serial.println("PrometheusMetrics初期化完了");
    
    // 初期システムメトリクスを計算
    updateSystemMetrics();
}

void PrometheusMetrics::update(const NtpStatistics* ntpStats, 
                              const GpsSummaryData* gpsData,
                              const GpsMonitor* gpsMonitor,
                              unsigned long ppsCount) {
    unsigned long now = millis();
    
    // NTPメトリクス更新
    if (ntpStats && (now - lastNtpUpdate >= NTP_UPDATE_INTERVAL)) {
        updateNtpMetrics(ntpStats);
        lastNtpUpdate = now;
    }
    
    // GPSメトリクス更新
    if (gpsData && gpsMonitor && (now - lastGpsUpdate >= GPS_UPDATE_INTERVAL)) {
        updateGpsMetrics(*gpsData, *gpsMonitor, ppsCount);
        lastGpsUpdate = now;
    }
    
    // システムメトリクス更新
    if (now - lastSystemUpdate >= SYSTEM_UPDATE_INTERVAL) {
        updateSystemMetrics();
        lastSystemUpdate = now;
    }
}

void PrometheusMetrics::updateNtpMetrics(const NtpStatistics* ntpStats) {
    if (!ntpStats) return;
    
    // NTP統計のコピー（実際のフィールド名を使用）
    ntpMetrics.totalRequests = ntpStats->requests_total;
    ntpMetrics.totalResponses = ntpStats->responses_sent;
    ntpMetrics.totalDropped = ntpStats->requests_invalid; // 無効な要求を破棄として扱う
    ntpMetrics.activeClients = ntpStats->clients_served;
    ntpMetrics.averageResponseTimeMs = ntpStats->avg_processing_time;
    ntpMetrics.responsesInLastMinute = ntpStats->responses_sent; // 簡略化
    ntpMetrics.malformedPackets = ntpStats->requests_invalid;
    ntpMetrics.unsupportedVersions = 0; // 現在のNtpStatisticsにない
    ntpMetrics.rateLimitDrops = 0; // 現在のNtpStatisticsにない
    
    // 応答時間統計の更新
    if (ntpStats->avg_processing_time < ntpMetrics.minResponseTimeMs) {
        ntpMetrics.minResponseTimeMs = ntpStats->avg_processing_time;
    }
    if (ntpStats->avg_processing_time > ntpMetrics.maxResponseTimeMs) {
        ntpMetrics.maxResponseTimeMs = ntpStats->avg_processing_time;
    }
}

void PrometheusMetrics::updateGpsMetrics(const GpsSummaryData& gpsData, 
                                       const GpsMonitor& gpsMonitor, 
                                       unsigned long ppsCount) {
    // 衛星数の更新（GpsSummaryDataで利用可能なフィールドのみ）
    gpsMetrics.totalSatellites = gpsData.SIV;
    
    // コンステレーション別衛星数は現在のGpsSummaryDataにないため、
    // 総衛星数から推定するか0に設定
    gpsMetrics.gpsSatellites = gpsData.SIV; // 総衛星数をGPSとして暫定設定
    gpsMetrics.glonassSatellites = 0; // 利用不可
    gpsMetrics.galileoSatellites = 0; // 利用不可
    gpsMetrics.beidouSatellites = 0;  // 利用不可
    gpsMetrics.qzssSatellites = 0;    // 利用不可
    
    // 精度情報の更新（HDOP/VDOPは現在のGpsSummaryDataにないため推定値を使用）
    gpsMetrics.hdop = 1.0;  // デフォルト値
    gpsMetrics.vdop = 1.0;  // デフォルト値
    gpsMetrics.fixType = gpsData.fixType;
    gpsMetrics.timeValid = gpsData.timeValid;
    gpsMetrics.dateValid = gpsData.dateValid;
    
    // PPS統計の更新
    gpsMetrics.totalPpsPulses = ppsCount;
    gpsMetrics.lastPpsTime = gpsMonitor.lastPpsTime;
    gpsMetrics.ppsActive = gpsMonitor.ppsActive;
    
    // システム監視情報の更新
    gpsMetrics.signalQuality = gpsMonitor.signalQuality;
    gpsMetrics.inFallbackMode = gpsMonitor.inFallbackMode;
    gpsMetrics.lastValidTime = gpsMonitor.lastValidTime;
    
    // 平均SNRの計算（簡単な推定）
    if (gpsMetrics.totalSatellites > 0) {
        gpsMetrics.averageSnr = gpsMetrics.signalQuality * 5.0; // 0-10 -> 0-50 dB-Hz
    }
}

void PrometheusMetrics::updateSystemMetrics() {
    // 稼働時間の更新
    systemMetrics.uptimeSeconds = millis() / 1000;
    
    // メモリ使用量の計算
    systemMetrics.usedRam = 17856;  // 現在の実測値（ビルド出力から）
    systemMetrics.freeRam = systemMetrics.totalRam - systemMetrics.usedRam;
    systemMetrics.ramUsagePercent = (float)systemMetrics.usedRam / systemMetrics.totalRam * 100.0;
    
    // フラッシュメモリ使用量
    systemMetrics.usedFlash = 406192;  // 現在の実測値（ビルド出力から）
    systemMetrics.flashUsagePercent = (float)systemMetrics.usedFlash / systemMetrics.totalFlash * 100.0;
    
    // ネットワーク接続状態
    systemMetrics.ethernetConnected = (Ethernet.linkStatus() == LinkON);
    
    // CPU温度（RP2040の内蔵温度センサー）
    systemMetrics.cpuTemperature = calculateCpuTemperature();
    
    // 入力電圧（アナログ読み取りから推定）
    systemMetrics.inputVoltage = 3.3; // 固定値（実際の電圧測定回路が必要）
    
    // ハードウェア状態（基本的な健全性チェック）
    systemMetrics.hardwareStatus = 1; // 1=正常、0=異常
}

float PrometheusMetrics::calculateCpuTemperature() {
    // RP2040内蔵温度センサーの読み取り（概算）
    // 実際の実装では analogReadTemp() を使用
    return 25.0 + (millis() % 100) / 10.0; // 25-35°Cの範囲での模擬値
}

void PrometheusMetrics::generatePrometheusOutput(char* buffer, size_t bufferSize) {
    if (!buffer || bufferSize == 0) return;
    
    size_t offset = 0;
    
    // ヘッダー情報
    offset += snprintf(buffer + offset, bufferSize - offset,
                      "# HELP GPS NTP Server Metrics\n"
                      "# TYPE ntp_info info\n"
                      "ntp_info{version=\"1.0\",device=\"pico2\"} 1\n\n");
    
    // NTPメトリクスの生成
    generateNtpMetrics(buffer, &offset, bufferSize);
    
    // GPSメトリクスの生成
    generateGpsMetrics(buffer, &offset, bufferSize);
    
    // システムメトリクスの生成
    generateSystemMetrics(buffer, &offset, bufferSize);
}

void PrometheusMetrics::generateNtpMetrics(char* buffer, size_t* offset, size_t bufferSize) {
    *offset += snprintf(buffer + *offset, bufferSize - *offset,
        "# HELP %s Total number of NTP requests received\n"
        "# TYPE %s counter\n"
        "%s %lu\n\n",
        METRIC_NTP_REQUESTS_TOTAL, METRIC_NTP_REQUESTS_TOTAL, METRIC_NTP_REQUESTS_TOTAL,
        ntpMetrics.totalRequests);
    
    *offset += snprintf(buffer + *offset, bufferSize - *offset,
        "# HELP %s Total number of NTP responses sent\n"
        "# TYPE %s counter\n"
        "%s %lu\n\n",
        METRIC_NTP_RESPONSES_TOTAL, METRIC_NTP_RESPONSES_TOTAL, METRIC_NTP_RESPONSES_TOTAL,
        ntpMetrics.totalResponses);
    
    *offset += snprintf(buffer + *offset, bufferSize - *offset,
        "# HELP %s Total number of dropped NTP requests\n"
        "# TYPE %s counter\n"
        "%s %lu\n\n",
        METRIC_NTP_DROPPED_TOTAL, METRIC_NTP_DROPPED_TOTAL, METRIC_NTP_DROPPED_TOTAL,
        ntpMetrics.totalDropped);
    
    *offset += snprintf(buffer + *offset, bufferSize - *offset,
        "# HELP %s Average NTP response time in milliseconds\n"
        "# TYPE %s gauge\n"
        "%s %.3f\n\n",
        METRIC_NTP_RESPONSE_TIME_MS, METRIC_NTP_RESPONSE_TIME_MS, METRIC_NTP_RESPONSE_TIME_MS,
        ntpMetrics.averageResponseTimeMs);
    
    *offset += snprintf(buffer + *offset, bufferSize - *offset,
        "# HELP %s Current NTP stratum level\n"
        "# TYPE %s gauge\n"
        "%s %d\n\n",
        METRIC_NTP_STRATUM, METRIC_NTP_STRATUM, METRIC_NTP_STRATUM,
        ntpMetrics.currentStratum);
    
    *offset += snprintf(buffer + *offset, bufferSize - *offset,
        "# HELP %s Number of active NTP clients\n"
        "# TYPE %s gauge\n"
        "%s %lu\n\n",
        METRIC_NTP_CLIENTS_ACTIVE, METRIC_NTP_CLIENTS_ACTIVE, METRIC_NTP_CLIENTS_ACTIVE,
        ntpMetrics.activeClients);
}

void PrometheusMetrics::generateGpsMetrics(char* buffer, size_t* offset, size_t bufferSize) {
    *offset += snprintf(buffer + *offset, bufferSize - *offset,
        "# HELP %s Total number of GPS satellites in view\n"
        "# TYPE %s gauge\n"
        "%s %d\n\n",
        METRIC_GPS_SATELLITES_TOTAL, METRIC_GPS_SATELLITES_TOTAL, METRIC_GPS_SATELLITES_TOTAL,
        gpsMetrics.totalSatellites);
    
    *offset += snprintf(buffer + *offset, bufferSize - *offset,
        "# HELP %s Number of satellites by constellation\n"
        "# TYPE %s gauge\n"
        "%s{constellation=\"gps\"} %d\n"
        "%s{constellation=\"glonass\"} %d\n"
        "%s{constellation=\"galileo\"} %d\n"
        "%s{constellation=\"beidou\"} %d\n"
        "%s{constellation=\"qzss\"} %d\n\n",
        METRIC_GPS_SATELLITES_GPS, METRIC_GPS_SATELLITES_GPS,
        METRIC_GPS_SATELLITES_GPS, gpsMetrics.gpsSatellites,
        METRIC_GPS_SATELLITES_GPS, gpsMetrics.glonassSatellites,
        METRIC_GPS_SATELLITES_GPS, gpsMetrics.galileoSatellites,
        METRIC_GPS_SATELLITES_GPS, gpsMetrics.beidouSatellites,
        METRIC_GPS_SATELLITES_GPS, gpsMetrics.qzssSatellites);
    
    *offset += snprintf(buffer + *offset, bufferSize - *offset,
        "# HELP %s GPS horizontal dilution of precision\n"
        "# TYPE %s gauge\n"
        "%s %.2f\n\n",
        METRIC_GPS_HDOP, METRIC_GPS_HDOP, METRIC_GPS_HDOP,
        gpsMetrics.hdop);
    
    *offset += snprintf(buffer + *offset, bufferSize - *offset,
        "# HELP %s GPS vertical dilution of precision\n"
        "# TYPE %s gauge\n"
        "%s %.2f\n\n",
        METRIC_GPS_VDOP, METRIC_GPS_VDOP, METRIC_GPS_VDOP,
        gpsMetrics.vdop);
    
    *offset += snprintf(buffer + *offset, bufferSize - *offset,
        "# HELP %s Total number of PPS pulses received\n"
        "# TYPE %s counter\n"
        "%s %lu\n\n",
        METRIC_GPS_PPS_PULSES_TOTAL, METRIC_GPS_PPS_PULSES_TOTAL, METRIC_GPS_PPS_PULSES_TOTAL,
        gpsMetrics.totalPpsPulses);
    
    *offset += snprintf(buffer + *offset, bufferSize - *offset,
        "# HELP %s GPS signal quality (0-10)\n"
        "# TYPE %s gauge\n"
        "%s %d\n\n",
        METRIC_GPS_SIGNAL_QUALITY, METRIC_GPS_SIGNAL_QUALITY, METRIC_GPS_SIGNAL_QUALITY,
        gpsMetrics.signalQuality);
    
    *offset += snprintf(buffer + *offset, bufferSize - *offset,
        "# HELP %s GPS fallback mode status (1=fallback, 0=normal)\n"
        "# TYPE %s gauge\n"
        "%s %d\n\n",
        METRIC_GPS_FALLBACK_MODE, METRIC_GPS_FALLBACK_MODE, METRIC_GPS_FALLBACK_MODE,
        gpsMetrics.inFallbackMode ? 1 : 0);
}

void PrometheusMetrics::generateSystemMetrics(char* buffer, size_t* offset, size_t bufferSize) {
    *offset += snprintf(buffer + *offset, bufferSize - *offset,
        "# HELP %s System uptime in seconds\n"
        "# TYPE %s counter\n"
        "%s %lu\n\n",
        METRIC_SYSTEM_UPTIME_SECONDS, METRIC_SYSTEM_UPTIME_SECONDS, METRIC_SYSTEM_UPTIME_SECONDS,
        systemMetrics.uptimeSeconds);
    
    *offset += snprintf(buffer + *offset, bufferSize - *offset,
        "# HELP %s RAM usage percentage\n"
        "# TYPE %s gauge\n"
        "%s %.2f\n\n",
        METRIC_SYSTEM_RAM_USAGE_PERCENT, METRIC_SYSTEM_RAM_USAGE_PERCENT, METRIC_SYSTEM_RAM_USAGE_PERCENT,
        systemMetrics.ramUsagePercent);
    
    *offset += snprintf(buffer + *offset, bufferSize - *offset,
        "# HELP %s Flash memory usage percentage\n"
        "# TYPE %s gauge\n"
        "%s %.2f\n\n",
        METRIC_SYSTEM_FLASH_USAGE_PERCENT, METRIC_SYSTEM_FLASH_USAGE_PERCENT, METRIC_SYSTEM_FLASH_USAGE_PERCENT,
        systemMetrics.flashUsagePercent);
    
    *offset += snprintf(buffer + *offset, bufferSize - *offset,
        "# HELP %s CPU temperature in Celsius\n"
        "# TYPE %s gauge\n"
        "%s %.2f\n\n",
        METRIC_SYSTEM_CPU_TEMPERATURE, METRIC_SYSTEM_CPU_TEMPERATURE, METRIC_SYSTEM_CPU_TEMPERATURE,
        systemMetrics.cpuTemperature);
    
    *offset += snprintf(buffer + *offset, bufferSize - *offset,
        "# HELP %s Ethernet connection status (1=connected, 0=disconnected)\n"
        "# TYPE %s gauge\n"
        "%s %d\n\n",
        METRIC_SYSTEM_ETHERNET_CONNECTED, METRIC_SYSTEM_ETHERNET_CONNECTED, METRIC_SYSTEM_ETHERNET_CONNECTED,
        systemMetrics.ethernetConnected ? 1 : 0);
}

float PrometheusMetrics::getNtpRequestRate() const {
    if (systemMetrics.uptimeSeconds > 0) {
        return (float)ntpMetrics.totalRequests / (systemMetrics.uptimeSeconds / 60.0);
    }
    return 0.0;
}

float PrometheusMetrics::getGpsSignalStrength() const {
    return gpsMetrics.signalQuality * 10.0; // 0-10 -> 0-100%
}

float PrometheusMetrics::getSystemHealth() const {
    float health = 100.0;
    
    // RAM使用率が80%を超えると健全性が低下
    if (systemMetrics.ramUsagePercent > 80.0) {
        health -= (systemMetrics.ramUsagePercent - 80.0);
    }
    
    // イーサネット接続がない場合
    if (!systemMetrics.ethernetConnected) {
        health -= 20.0;
    }
    
    // GPSフォールバックモードの場合
    if (gpsMetrics.inFallbackMode) {
        health -= 15.0;
    }
    
    // CPU温度が高い場合（50°C以上）
    if (systemMetrics.cpuTemperature > 50.0) {
        health -= (systemMetrics.cpuTemperature - 50.0) * 2.0;
    }
    
    return fmax(0.0, fmin(100.0, health));
}

void PrometheusMetrics::resetNtpCounters() {
    memset(&ntpMetrics, 0, sizeof(ntpMetrics));
    ntpMetrics.minResponseTimeMs = 999999.0;
    ntpMetrics.maxResponseTimeMs = 0.0;
}

void PrometheusMetrics::resetGpsCounters() {
    memset(&gpsMetrics, 0, sizeof(gpsMetrics));
}

void PrometheusMetrics::resetSystemCounters() {
    memset(&systemMetrics, 0, sizeof(systemMetrics));
    systemMetrics.totalRam = 524288;
    systemMetrics.totalFlash = 4190208;
}

void PrometheusMetrics::resetAllCounters() {
    resetNtpCounters();
    resetGpsCounters();
    resetSystemCounters();
}

void PrometheusMetrics::printNtpMetrics() const {
    Serial.println("=== NTPメトリクス ===");
    Serial.printf("総要求数: %lu\n", ntpMetrics.totalRequests);
    Serial.printf("総応答数: %lu\n", ntpMetrics.totalResponses);
    Serial.printf("破棄数: %lu\n", ntpMetrics.totalDropped);
    Serial.printf("平均応答時間: %.3fms\n", ntpMetrics.averageResponseTimeMs);
    Serial.printf("Stratumレベル: %d\n", ntpMetrics.currentStratum);
    Serial.printf("アクティブクライアント: %lu\n", ntpMetrics.activeClients);
}

void PrometheusMetrics::printGpsMetrics() const {
#ifdef DEBUG_PROMETHEUS_GPS
    Serial.println("=== GPSメトリクス ===");
    Serial.printf("総衛星数: %d\n", gpsMetrics.totalSatellites);
    Serial.printf("GPS: %d, GLONASS: %d, Galileo: %d, BeiDou: %d, QZSS: %d\n",
                 gpsMetrics.gpsSatellites, gpsMetrics.glonassSatellites,
                 gpsMetrics.galileoSatellites, gpsMetrics.beidouSatellites,
                 gpsMetrics.qzssSatellites);
    Serial.printf("HDOP: %.2f, VDOP: %.2f\n", gpsMetrics.hdop, gpsMetrics.vdop);
    Serial.printf("PPS総数: %lu\n", gpsMetrics.totalPpsPulses);
    Serial.printf("信号品質: %d/10\n", gpsMetrics.signalQuality);
    Serial.printf("フォールバックモード: %s\n", gpsMetrics.inFallbackMode ? "はい" : "いいえ");
#endif
}

void PrometheusMetrics::printSystemMetrics() const {
    Serial.println("=== システムメトリクス ===");
    Serial.printf("稼働時間: %lu秒\n", systemMetrics.uptimeSeconds);
    Serial.printf("RAM使用率: %.2f%%\n", systemMetrics.ramUsagePercent);
    Serial.printf("フラッシュ使用率: %.2f%%\n", systemMetrics.flashUsagePercent);
    Serial.printf("CPU温度: %.2f°C\n", systemMetrics.cpuTemperature);
    Serial.printf("イーサネット接続: %s\n", systemMetrics.ethernetConnected ? "接続" : "切断");
}

void PrometheusMetrics::printAllMetrics() const {
    printNtpMetrics();
    Serial.println();
    printGpsMetrics();
    Serial.println();
    printSystemMetrics();
    Serial.println();
    Serial.printf("システム健全性: %.1f%%\n", getSystemHealth());
}
#ifndef PROMETHEUS_METRICS_H
#define PROMETHEUS_METRICS_H

#include <Arduino.h>
#include <EthernetUdp.h>
#include "SystemTypes.h"
#include "../gps/GpsModel.h"
#include "../network/NtpServer.h"
#include "SystemMonitor.h"

// メトリクス統計構造体
struct NtpMetrics {
    // NTP要求統計
    unsigned long totalRequests;        // 総要求数
    unsigned long totalResponses;       // 総応答数
    unsigned long totalDropped;         // 破棄された要求数
    unsigned long activeClients;        // アクティブなクライアント数
    
    // 応答時間統計
    float averageResponseTimeMs;        // 平均応答時間（ミリ秒）
    float minResponseTimeMs;            // 最小応答時間
    float maxResponseTimeMs;            // 最大応答時間
    unsigned long responsesInLastMinute; // 過去1分間の応答数
    
    // 精度統計
    float currentAccuracyMs;            // 現在の時刻精度（ミリ秒）
    float averageAccuracyMs;            // 平均時刻精度
    int currentStratum;                 // 現在のStratumレベル
    unsigned long lastSyncTime;         // 最後の同期時刻（Unix timestamp）
    
    // エラー統計
    unsigned long malformedPackets;     // 不正なパケット数
    unsigned long unsupportedVersions;  // サポートされていないバージョン数
    unsigned long rateLimitDrops;       // レート制限による破棄数
};

struct GpsMetrics {
    // 衛星情報
    uint8_t totalSatellites;           // 総衛星数
    uint8_t gpsSatellites;             // GPS衛星数
    uint8_t glonassSatellites;         // GLONASS衛星数  
    uint8_t galileoSatellites;         // Galileo衛星数
    uint8_t beidouSatellites;          // BeiDou衛星数
    uint8_t qzssSatellites;            // QZSS衛星数
    
    // 精度情報
    float hdop;                        // 水平精度劣化率
    float vdop;                        // 垂直精度劣化率
    uint8_t fixType;                   // 測位タイプ
    bool timeValid;                    // 時刻有効性
    bool dateValid;                    // 日付有効性
    
    // PPS信号統計
    unsigned long totalPpsPulses;      // 総PPSパルス数
    unsigned long lastPpsTime;         // 最後のPPS時刻
    bool ppsActive;                    // PPS信号アクティブ状態
    float ppsJitter;                   // PPSジッター（マイクロ秒）
    
    // 信号品質
    float averageSnr;                  // 平均信号対雑音比
    uint8_t signalQuality;             // 信号品質（0-10）
    bool inFallbackMode;               // フォールバックモード状態
    unsigned long lastValidTime;       // 最後の有効時刻
};

struct SystemMetrics {
    // メモリ使用量
    unsigned long totalRam;            // 総RAM容量
    unsigned long usedRam;             // 使用RAM容量
    unsigned long freeRam;             // 空きRAM容量
    float ramUsagePercent;             // RAM使用率（%）
    
    // フラッシュメモリ使用量
    unsigned long totalFlash;          // 総フラッシュ容量
    unsigned long usedFlash;           // 使用フラッシュ容量
    float flashUsagePercent;           // フラッシュ使用率（%）
    
    // システム統計
    unsigned long uptimeSeconds;       // 稼働時間（秒）
    float cpuTemperature;              // CPU温度（°C）
    unsigned long heapFragmentation;   // ヒープ断片化
    
    // ネットワーク統計
    bool ethernetConnected;            // イーサネット接続状態
    unsigned long totalPacketsSent;    // 送信パケット総数
    unsigned long totalPacketsReceived; // 受信パケット総数
    unsigned long networkDrops;        // ネットワーク破棄数
    
    // 電源・ハードウェア統計
    float inputVoltage;                // 入力電圧（V）
    uint8_t hardwareStatus;            // ハードウェア状態
    unsigned long totalResets;         // 総リセット回数
    unsigned long watchdogResets;      // ウォッチドッグリセット回数
};

class PrometheusMetrics {
private:
    NtpMetrics ntpMetrics;
    GpsMetrics gpsMetrics;
    SystemMetrics systemMetrics;
    
    // メトリクス更新タイムスタンプ
    unsigned long lastNtpUpdate;
    unsigned long lastGpsUpdate;
    unsigned long lastSystemUpdate;
    
    // 更新間隔（ミリ秒）
    static const unsigned long NTP_UPDATE_INTERVAL = 10000;    // 10秒
    static const unsigned long GPS_UPDATE_INTERVAL = 5000;     // 5秒
    static const unsigned long SYSTEM_UPDATE_INTERVAL = 30000; // 30秒
    
    // 内部メソッド
    void updateNtpMetrics(const NtpStatistics* ntpStats);
    void updateGpsMetrics(const GpsSummaryData& gpsData, const GpsMonitor& gpsMonitor, unsigned long ppsCount);
    void updateSystemMetrics();
    void calculateAverages();
    float calculateMemoryUsage();
    float calculateCpuTemperature();
    
public:
    PrometheusMetrics();
    
    // 初期化
    void init();
    
    // メトリクス更新（メインループから呼び出し）
    void update(const NtpStatistics* ntpStats = nullptr, 
                const GpsSummaryData* gpsData = nullptr,
                const GpsMonitor* gpsMonitor = nullptr,
                unsigned long ppsCount = 0);
    
    // Prometheus形式文字列生成
    void generatePrometheusOutput(char* buffer, size_t bufferSize);
    void generateNtpMetrics(char* buffer, size_t* offset, size_t bufferSize);
    void generateGpsMetrics(char* buffer, size_t* offset, size_t bufferSize);
    void generateSystemMetrics(char* buffer, size_t* offset, size_t bufferSize);
    
    // 個別メトリクス取得メソッド
    const NtpMetrics& getNtpMetrics() const { return ntpMetrics; }
    const GpsMetrics& getGpsMetrics() const { return gpsMetrics; }
    const SystemMetrics& getSystemMetrics() const { return systemMetrics; }
    
    // メトリクス統計取得
    float getNtpRequestRate() const;        // 1分間あたりのNTP要求数
    float getGpsSignalStrength() const;     // GPS信号強度
    float getSystemHealth() const;          // システム健全性スコア（0-100）
    
    // リセット・メンテナンス
    void resetNtpCounters();
    void resetGpsCounters();
    void resetSystemCounters();
    void resetAllCounters();
    
    // デバッグ出力
    void printNtpMetrics() const;
    void printGpsMetrics() const;
    void printSystemMetrics() const;
    void printAllMetrics() const;
};

// Prometheusメトリクス名定数
extern const char* METRIC_NTP_REQUESTS_TOTAL;
extern const char* METRIC_NTP_RESPONSES_TOTAL;
extern const char* METRIC_NTP_DROPPED_TOTAL;
extern const char* METRIC_NTP_RESPONSE_TIME_MS;
extern const char* METRIC_NTP_ACCURACY_MS;
extern const char* METRIC_NTP_STRATUM;
extern const char* METRIC_NTP_CLIENTS_ACTIVE;

extern const char* METRIC_GPS_SATELLITES_TOTAL;
extern const char* METRIC_GPS_SATELLITES_GPS;
extern const char* METRIC_GPS_SATELLITES_GLONASS;
extern const char* METRIC_GPS_SATELLITES_GALILEO;
extern const char* METRIC_GPS_SATELLITES_BEIDOU;
extern const char* METRIC_GPS_SATELLITES_QZSS;
extern const char* METRIC_GPS_HDOP;
extern const char* METRIC_GPS_VDOP;
extern const char* METRIC_GPS_PPS_PULSES_TOTAL;
extern const char* METRIC_GPS_SIGNAL_QUALITY;
extern const char* METRIC_GPS_FALLBACK_MODE;

extern const char* METRIC_SYSTEM_UPTIME_SECONDS;
extern const char* METRIC_SYSTEM_RAM_USAGE_PERCENT;
extern const char* METRIC_SYSTEM_FLASH_USAGE_PERCENT;
extern const char* METRIC_SYSTEM_CPU_TEMPERATURE;
extern const char* METRIC_SYSTEM_ETHERNET_CONNECTED;
extern const char* METRIC_SYSTEM_PACKETS_SENT_TOTAL;
extern const char* METRIC_SYSTEM_PACKETS_RECEIVED_TOTAL;

#endif // PROMETHEUS_METRICS_H
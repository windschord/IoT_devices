#ifndef GPS_WEB_SERVER_H
#define GPS_WEB_SERVER_H

#include <Arduino.h>
#include <Ethernet.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
#include "../gps/Gps_model.h"
#include "http/HttpRequestParser.h"
#include "http/HttpResponseBuilder.h"
#include "routing/RouteHandler.h"
#include "routing/ApiRouter.h"
#include "routing/FileRouter.h"
#include "filesystem/FileSystemHandler.h"
#include "filesystem/CacheManager.h"

// Forward declarations
class NtpServer;
class ConfigManager;
class PrometheusMetrics;
class LoggingService;
class GpsClient;

/**
 * @brief GPS Webサーバークラス（軽量化版）
 * 
 * 新しいアーキテクチャに基づいた軽量なWebサーバー実装。
 * 処理を専門クラスに委譲し、設定可能なルーティングを提供します。
 */
class ModernGpsWebServer {
public:
    /**
     * @brief サーバー設定構造体
     */
    struct ServerConfig {
        bool enableCaching;         // キャッシュ有効フラグ
        bool enableCompression;     // 圧縮有効フラグ
        int maxCacheEntries;       // 最大キャッシュエントリ数
        size_t maxCacheSize;       // 最大キャッシュサイズ
        int requestTimeout;        // リクエストタイムアウト（秒）
        bool enableAccessLog;      // アクセスログ有効フラグ
        bool enableDebugApi;       // デバッグAPI有効フラグ
    };

    /**
     * @brief ModernGpsWebServerコンストラクター
     */
    ModernGpsWebServer();

    /**
     * @brief デストラクター
     */
    ~ModernGpsWebServer();

    /**
     * @brief 依存性注入メソッド（従来との互換性維持）
     */
    void setNtpServer(NtpServer* ntpServerInstance) { ntpServer_ = ntpServerInstance; }
    void setConfigManager(ConfigManager* configManagerInstance);
    void setPrometheusMetrics(PrometheusMetrics* prometheusMetricsInstance);
    void setLoggingService(LoggingService* loggingServiceInstance);
    void setGpsClient(GpsClient* gpsClientInstance);

    /**
     * @brief サーバー設定
     * @param config サーバー設定
     */
    void configure(const ServerConfig& config);

    /**
     * @brief サーバー初期化
     * @return bool 初期化成功フラグ
     */
    bool initialize();

    /**
     * @brief メインクライアント処理関数（従来との互換性維持）
     * @param stream 出力ストリーム
     * @param server イーサネットサーバー
     * @param ubxNavSatData_t 衛星データ（使用されない）
     * @param gpsSummaryData GPS概要データ（使用されない）
     */
    void handleClient(Stream& stream, EthernetServer& server, 
                     UBX_NAV_SAT_data_t* ubxNavSatData_t, 
                     GpsSummaryData gpsSummaryData);

    /**
     * @brief 新しいクライアント処理関数
     * @param server イーサネットサーバー
     */
    void processRequests(EthernetServer& server);

    /**
     * @brief 統計情報取得
     */
    struct Statistics {
        unsigned long requestCount;
        unsigned long errorCount;
        unsigned long averageResponseTime;
        float cacheHitRatio;
    };
    
    Statistics getStatistics() const;

    /**
     * @brief パフォーマンス監視メソッド（従来との互換性維持）
     */
    void invalidateGpsCache();
    unsigned long getRequestCount() const { return statistics_.requestCount; }
    unsigned long getAverageResponseTime() const { return statistics_.averageResponseTime; }

private:
    // 依存性
    NtpServer* ntpServer_;
    ConfigManager* configManager_;
    PrometheusMetrics* prometheusMetrics_;
    LoggingService* loggingService_;
    GpsClient* gpsClient_;

    // 新しいアーキテクチャのコンポーネント
    RouteHandler routeHandler_;
    ApiRouter apiRouter_;
    FileRouter fileRouter_;
    FileSystemHandler fileSystemHandler_;
    CacheManager* cacheManager_;

    // 設定と統計
    ServerConfig config_;
    Statistics statistics_;
    bool initialized_;

    /**
     * @brief ルーティングテーブル設定
     */
    void setupRoutes();

    /**
     * @brief 単一リクエスト処理
     * @param client イーサネットクライアント
     */
    void handleSingleRequest(EthernetClient& client);

    /**
     * @brief リクエスト読み込み
     * @param client イーサネットクライアント
     * @return String 読み込まれたリクエスト
     */
    String readRequest(EthernetClient& client);

    /**
     * @brief ログ出力
     * @param level ログレベル
     * @param message メッセージ
     */
    void log(const String& level, const String& message);

    /**
     * @brief 統計更新
     * @param responseTime レスポンス時間
     * @param isError エラーフラグ
     */
    void updateStatistics(unsigned long responseTime, bool isError);

    /**
     * @brief デフォルト設定取得
     * @return ServerConfig デフォルト設定
     */
    ServerConfig getDefaultConfig() const;
};

#endif
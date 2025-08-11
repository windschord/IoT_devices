#ifndef API_ROUTER_H
#define API_ROUTER_H

#include <Arduino.h>
#include <Ethernet.h>
#include "RouteHandler.h"
#include "../http/HttpRequestParser.h"
#include "../http/HttpResponseBuilder.h"

// Forward declarations
class ConfigManager;
class GpsClient;
class PrometheusMetrics;
class LoggingService;

/**
 * @brief APIエンドポイント管理クラス
 * 
 * REST APIエンドポイントの管理と処理を担当し、
 * JSON APIレスポンスの生成を統一的に処理します。
 */
class ApiRouter {
public:
    /**
     * @brief APIRouterコンストラクター
     */
    ApiRouter();

    /**
     * @brief 依存性注入メソッド
     */
    void setConfigManager(ConfigManager* configManager) { configManager_ = configManager; }
    void setGpsClient(GpsClient* gpsClient) { gpsClient_ = gpsClient; }
    void setPrometheusMetrics(PrometheusMetrics* prometheusMetrics) { prometheusMetrics_ = prometheusMetrics; }
    void setLoggingService(LoggingService* loggingService) { loggingService_ = loggingService; }

    /**
     * @brief APIルートを設定
     * @param routeHandler ルートハンドラーの参照
     */
    void setupRoutes(RouteHandler& routeHandler);

    // === GPS API ハンドラー ===
    /**
     * @brief GPS情報取得 API (GET /api/gps)
     */
    static void handleGpsGet(EthernetClient& client, const HttpRequestParser::ParsedRequest& request);

    // === 設定 API ハンドラー ===
    /**
     * @brief 全設定取得 API (GET /api/config)
     */
    static void handleConfigGet(EthernetClient& client, const HttpRequestParser::ParsedRequest& request);

    /**
     * @brief ネットワーク設定取得 API (GET /api/config/network)
     */
    static void handleConfigNetworkGet(EthernetClient& client, const HttpRequestParser::ParsedRequest& request);

    /**
     * @brief ネットワーク設定更新 API (POST /api/config/network)
     */
    static void handleConfigNetworkPost(EthernetClient& client, const HttpRequestParser::ParsedRequest& request);

    /**
     * @brief GNSS設定取得 API (GET /api/config/gnss)
     */
    static void handleConfigGnssGet(EthernetClient& client, const HttpRequestParser::ParsedRequest& request);

    /**
     * @brief GNSS設定更新 API (POST /api/config/gnss)
     */
    static void handleConfigGnssPost(EthernetClient& client, const HttpRequestParser::ParsedRequest& request);

    /**
     * @brief NTP設定取得 API (GET /api/config/ntp)
     */
    static void handleConfigNtpGet(EthernetClient& client, const HttpRequestParser::ParsedRequest& request);

    /**
     * @brief NTP設定更新 API (POST /api/config/ntp)
     */
    static void handleConfigNtpPost(EthernetClient& client, const HttpRequestParser::ParsedRequest& request);

    /**
     * @brief システム設定取得 API (GET /api/config/system)
     */
    static void handleConfigSystemGet(EthernetClient& client, const HttpRequestParser::ParsedRequest& request);

    /**
     * @brief システム設定更新 API (POST /api/config/system)
     */
    static void handleConfigSystemPost(EthernetClient& client, const HttpRequestParser::ParsedRequest& request);

    /**
     * @brief ログ設定取得 API (GET /api/config/log)
     */
    static void handleConfigLogGet(EthernetClient& client, const HttpRequestParser::ParsedRequest& request);

    /**
     * @brief ログ設定更新 API (POST /api/config/log)
     */
    static void handleConfigLogPost(EthernetClient& client, const HttpRequestParser::ParsedRequest& request);

    // === システム API ハンドラー ===
    /**
     * @brief システム状態取得 API (GET /api/status)
     */
    static void handleStatusGet(EthernetClient& client, const HttpRequestParser::ParsedRequest& request);

    /**
     * @brief システム再起動 API (POST /api/system/reboot)
     */
    static void handleSystemRebootPost(EthernetClient& client, const HttpRequestParser::ParsedRequest& request);

    /**
     * @brief システムメトリクス取得 API (GET /api/system/metrics)
     */
    static void handleSystemMetricsGet(EthernetClient& client, const HttpRequestParser::ParsedRequest& request);

    /**
     * @brief システムログ取得 API (GET /api/system/logs)
     */
    static void handleSystemLogsGet(EthernetClient& client, const HttpRequestParser::ParsedRequest& request);

    /**
     * @brief ファクトリーリセット API (POST /api/reset)
     */
    static void handleFactoryReset(EthernetClient& client, const HttpRequestParser::ParsedRequest& request);

    /**
     * @brief デバッグファイル一覧 API (GET /api/debug/files)
     */
    static void handleDebugFilesGet(EthernetClient& client, const HttpRequestParser::ParsedRequest& request);

private:
    ConfigManager* configManager_;
    GpsClient* gpsClient_;
    PrometheusMetrics* prometheusMetrics_;
    LoggingService* loggingService_;

    // 静的インスタンス（ハンドラー関数からアクセスするため）
    static ApiRouter* instance_;

    /**
     * @brief レート制限チェック
     * @param client EthernetClientの参照
     * @return bool リクエスト許可フラグ
     */
    static bool checkRateLimit(EthernetClient& client);

    /**
     * @brief JSON入力バリデーション
     * @param jsonInput JSON文字列
     * @return bool バリデーション成功フラグ
     */
    static bool validateJsonInput(const String& jsonInput);

    /**
     * @brief エラーレスポンス送信
     * @param client EthernetClientの参照
     * @param statusCode HTTPステータスコード
     * @param message エラーメッセージ
     */
    static void sendErrorResponse(EthernetClient& client, 
                                 HttpResponseBuilder::StatusCode statusCode, 
                                 const String& message);

    /**
     * @brief JSONレスポンス送信
     * @param client EthernetClientの参照
     * @param jsonResponse JSONレスポンス
     * @param statusCode HTTPステータスコード
     */
    static void sendJsonResponse(EthernetClient& client, 
                                const String& jsonResponse,
                                HttpResponseBuilder::StatusCode statusCode = HttpResponseBuilder::OK);
};

#endif
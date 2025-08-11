#ifndef FILE_ROUTER_H
#define FILE_ROUTER_H

#include <Arduino.h>
#include <Ethernet.h>
#include "RouteHandler.h"
#include "../http/HttpRequestParser.h"
#include "../http/HttpResponseBuilder.h"

// Forward declaration
class LoggingService;

/**
 * @brief 静的ファイル配信管理クラス
 * 
 * LittleFSからの静的ファイル配信を担当し、
 * MIMEタイプ判定、キャッシュ制御、エラーハンドリングを統一的に処理します。
 */
class FileRouter {
public:
    /**
     * @brief ファイル配信設定構造体
     */
    struct FileRoute {
        String urlPath;      // URL パス（例: "/config"）
        String filePath;     // ファイルパス（例: "/config.html"）
        String mimeType;     // MIME タイプ
        bool cacheEnabled;   // キャッシュ有効フラグ
        int cacheDuration;   // キャッシュ持続時間（秒）
        bool enabled;        // 有効/無効フラグ
    };

    /**
     * @brief FileRouterコンストラクター
     */
    FileRouter();

    /**
     * @brief 依存性注入メソッド
     */
    void setLoggingService(LoggingService* loggingService) { loggingService_ = loggingService; }

    /**
     * @brief ファイルルートを設定
     * @param routeHandler ルートハンドラーの参照
     */
    void setupRoutes(RouteHandler& routeHandler);

    /**
     * @brief ファイルルートを追加
     * @param urlPath URL パス
     * @param filePath ファイルパス
     * @param mimeType MIME タイプ（省略時は自動判定）
     * @param cacheEnabled キャッシュ有効フラグ
     * @param cacheDuration キャッシュ持続時間
     * @return bool 追加成功フラグ
     */
    bool addFileRoute(const String& urlPath, 
                      const String& filePath,
                      const String& mimeType = "",
                      bool cacheEnabled = true,
                      int cacheDuration = 3600);

    /**
     * @brief デフォルトルート設定
     */
    void setupDefaultRoutes();

    // === 静的ファイルハンドラー ===
    /**
     * @brief メインページハンドラー (GET /)
     */
    static void handleMainPage(EthernetClient& client, const HttpRequestParser::ParsedRequest& request);

    /**
     * @brief GPS ページハンドラー (GET /gps)
     */
    static void handleGpsPage(EthernetClient& client, const HttpRequestParser::ParsedRequest& request);

    /**
     * @brief GPS JavaScript ハンドラー (GET /gps.js)
     */
    static void handleGpsScript(EthernetClient& client, const HttpRequestParser::ParsedRequest& request);

    /**
     * @brief 設定ページハンドラー (GET /config)
     */
    static void handleConfigPage(EthernetClient& client, const HttpRequestParser::ParsedRequest& request);

    /**
     * @brief 設定 JavaScript ハンドラー (GET /config.js)
     */
    static void handleConfigScript(EthernetClient& client, const HttpRequestParser::ParsedRequest& request);

    /**
     * @brief Prometheus メトリクスハンドラー (GET /metrics)
     */
    static void handleMetricsPage(EthernetClient& client, const HttpRequestParser::ParsedRequest& request);

    /**
     * @brief 汎用ファイルハンドラー
     * @param client EthernetClientの参照
     * @param filepath ファイルパス
     * @param mimeType MIME タイプ
     * @param cacheEnabled キャッシュ有効フラグ
     * @param cacheDuration キャッシュ持続時間
     */
    static void handleFileRequest(EthernetClient& client, 
                                 const String& filepath,
                                 const String& mimeType,
                                 bool cacheEnabled = true,
                                 int cacheDuration = 3600);

    /**
     * @brief ファイル拡張子からMIMEタイプを判定
     * @param filepath ファイルパス
     * @return String MIME タイプ
     */
    static String getMimeType(const String& filepath);

private:
    static const int MAX_FILE_ROUTES = 20;
    FileRoute fileRoutes_[MAX_FILE_ROUTES];
    int routeCount_;
    LoggingService* loggingService_;

    // 静的インスタンス（ハンドラー関数からアクセスするため）
    static FileRouter* instance_;

    /**
     * @brief LittleFSファイルサイズ取得
     * @param filepath ファイルパス
     * @return size_t ファイルサイズ（失敗時は0）
     */
    static size_t getFileSize(const String& filepath);

    /**
     * @brief ファイル存在チェック
     * @param filepath ファイルパス
     * @return bool 存在フラグ
     */
    static bool fileExists(const String& filepath);

    /**
     * @brief キャッシュヘッダー生成
     * @param cacheDuration キャッシュ持続時間（秒）
     * @return String キャッシュヘッダー文字列
     */
    static String generateCacheHeaders(int cacheDuration);
};

#endif
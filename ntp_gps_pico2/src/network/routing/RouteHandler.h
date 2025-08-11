#ifndef ROUTE_HANDLER_H
#define ROUTE_HANDLER_H

#include <Arduino.h>
#include <Ethernet.h>
#include "../http/HttpRequestParser.h"
#include "../http/HttpResponseBuilder.h"

/**
 * @brief URLルーティング管理クラス
 * 
 * HTTPリクエストのパスとメソッドに基づいて、
 * 適切なハンドラー関数にリクエストをルーティングします。
 */
class RouteHandler {
public:
    /**
     * @brief ルートハンドラー関数の型定義
     * @param client EthernetClientの参照
     * @param request 解析済みHTTPリクエスト
     */
    typedef void (*HandlerFunction)(EthernetClient& client, const HttpRequestParser::ParsedRequest& request);

    /**
     * @brief HTTPメソッド列挙型
     */
    enum HttpMethod {
        GET,
        POST,
        PUT,
        DELETE,
        OPTIONS,
        ANY
    };

    /**
     * @brief ルート情報構造体
     */
    struct Route {
        String pattern;           // URLパターン（例: "/api/config/*"）
        HttpMethod method;        // HTTPメソッド
        HandlerFunction handler;  // ハンドラー関数
        int priority;            // 優先度（低い値ほど高優先度）
        bool enabled;            // 有効/無効フラグ
    };

    /**
     * @brief RouteHandlerコンストラクター
     */
    RouteHandler();

    /**
     * @brief ルートを追加
     * @param pattern URLパターン
     * @param method HTTPメソッド
     * @param handler ハンドラー関数
     * @param priority 優先度（デフォルト: 100）
     * @return bool 追加成功フラグ
     */
    bool addRoute(const String& pattern, HttpMethod method, HandlerFunction handler, int priority = 100);

    /**
     * @brief GETルートを追加（便利メソッド）
     * @param pattern URLパターン
     * @param handler ハンドラー関数
     * @param priority 優先度
     * @return bool 追加成功フラグ
     */
    bool addGetRoute(const String& pattern, HandlerFunction handler, int priority = 100);

    /**
     * @brief POSTルートを追加（便利メソッド）
     * @param pattern URLパターン
     * @param handler ハンドラー関数
     * @param priority 優先度
     * @return bool 追加成功フラグ
     */
    bool addPostRoute(const String& pattern, HandlerFunction handler, int priority = 100);

    /**
     * @brief ルートを無効化
     * @param pattern URLパターン
     * @param method HTTPメソッド
     * @return bool 無効化成功フラグ
     */
    bool disableRoute(const String& pattern, HttpMethod method);

    /**
     * @brief ルートを有効化
     * @param pattern URLパターン
     * @param method HTTPメソッド
     * @return bool 有効化成功フラグ
     */
    bool enableRoute(const String& pattern, HttpMethod method);

    /**
     * @brief リクエストを適切なハンドラーにルーティング
     * @param client EthernetClientの参照
     * @param request 解析済みHTTPリクエスト
     * @return bool ルーティング成功フラグ
     */
    bool route(EthernetClient& client, const HttpRequestParser::ParsedRequest& request);

    /**
     * @brief 登録されたルート数を取得
     * @return int ルート数
     */
    int getRouteCount() const;

    /**
     * @brief すべてのルートをクリア
     */
    void clearRoutes();

private:
    static const int MAX_ROUTES = 50;
    Route routes_[MAX_ROUTES];
    int routeCount_;

    /**
     * @brief 文字列のHTTPメソッドを列挙型に変換
     * @param methodStr HTTPメソッド文字列
     * @return HttpMethod HTTPメソッド列挙型
     */
    HttpMethod stringToMethod(const String& methodStr);

    /**
     * @brief URLパターンマッチング
     * @param pattern URLパターン
     * @param path 実際のパス
     * @return bool マッチ成功フラグ
     */
    bool matchPattern(const String& pattern, const String& path);

    /**
     * @brief ルートの優先度でソート
     */
    void sortRoutesByPriority();
};

#endif
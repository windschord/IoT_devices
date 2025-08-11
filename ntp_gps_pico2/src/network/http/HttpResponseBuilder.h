#ifndef HTTP_RESPONSE_BUILDER_H
#define HTTP_RESPONSE_BUILDER_H

#include <Arduino.h>
#include <Ethernet.h>

/**
 * @brief HTTPレスポンス生成専用クラス
 * 
 * HTTPレスポンスの構築と送信を担当し、ヘッダー追加、セキュリティヘッダー、
 * レスポンス送信を統一的に処理します。
 */
class HttpResponseBuilder {
public:
    /**
     * @brief HTTPステータスコード
     */
    enum StatusCode {
        OK = 200,
        BAD_REQUEST = 400,
        NOT_FOUND = 404,
        TOO_MANY_REQUESTS = 429,
        INTERNAL_SERVER_ERROR = 500
    };

    /**
     * @brief レスポンスビルダーコンストラクター
     * @param client EthernetClientの参照
     */
    explicit HttpResponseBuilder(EthernetClient& client);

    /**
     * @brief ステータスコードを設定
     * @param statusCode HTTPステータスコード
     * @return HttpResponseBuilder& チェーン可能
     */
    HttpResponseBuilder& setStatus(StatusCode statusCode);

    /**
     * @brief Content-Typeヘッダーを設定
     * @param contentType コンテンツタイプ
     * @return HttpResponseBuilder& チェーン可能
     */
    HttpResponseBuilder& setContentType(const String& contentType);

    /**
     * @brief カスタムヘッダーを追加
     * @param name ヘッダー名
     * @param value ヘッダー値
     * @return HttpResponseBuilder& チェーン可能
     */
    HttpResponseBuilder& addHeader(const String& name, const String& value);

    /**
     * @brief セキュリティヘッダーを追加
     * @return HttpResponseBuilder& チェーン可能
     */
    HttpResponseBuilder& addSecurityHeaders();

    /**
     * @brief キャッシュ無効化ヘッダーを追加
     * @return HttpResponseBuilder& チェーン可能
     */
    HttpResponseBuilder& addNoCacheHeaders();

    /**
     * @brief レスポンスボディを設定
     * @param body レスポンスボディ
     * @return HttpResponseBuilder& チェーン可能
     */
    HttpResponseBuilder& setBody(const String& body);

    /**
     * @brief JSONレスポンス用の設定を適用
     * @param jsonBody JSONボディ
     * @param statusCode HTTPステータスコード
     * @return HttpResponseBuilder& チェーン可能
     */
    HttpResponseBuilder& json(const String& jsonBody, StatusCode statusCode = OK);

    /**
     * @brief HTMLレスポンス用の設定を適用
     * @param htmlBody HTMLボディ
     * @param statusCode HTTPステータスコード
     * @return HttpResponseBuilder& チェーン可能
     */
    HttpResponseBuilder& html(const String& htmlBody, StatusCode statusCode = OK);

    /**
     * @brief レスポンスを送信
     */
    void send();

    /**
     * @brief 404エラーレスポンスを送信
     * @param client EthernetClientの参照
     */
    static void send404(EthernetClient& client);

    /**
     * @brief エラーレスポンスを送信
     * @param client EthernetClientの参照
     * @param statusCode エラーコード
     * @param message エラーメッセージ
     */
    static void sendError(EthernetClient& client, StatusCode statusCode, const String& message);

private:
    EthernetClient& client_;
    StatusCode statusCode_;
    String contentType_;
    String customHeaders_;
    String body_;
    bool securityHeaders_;
    bool noCacheHeaders_;

    /**
     * @brief ステータステキストを取得
     * @param statusCode HTTPステータスコード
     * @return String ステータステキスト
     */
    static String getStatusText(StatusCode statusCode);
};

#endif
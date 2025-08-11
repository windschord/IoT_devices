#ifndef HTTP_REQUEST_PARSER_H
#define HTTP_REQUEST_PARSER_H

#include <Arduino.h>

/**
 * @brief HTTP リクエスト解析専用クラス
 * 
 * HTTPリクエストの解析処理を担当し、リクエストライン、ヘッダー、ボディの
 * 解析を効率的に行います。
 */
class HttpRequestParser {
public:
    struct ParsedRequest {
        String method;          // GET, POST, etc.
        String path;           // URL path
        String queryString;    // Query parameters
        String httpVersion;    // HTTP version
        String body;           // Request body
        int contentLength;     // Content-Length header value
        bool isValid;          // Parse success flag
    };

    /**
     * @brief HTTPリクエスト文字列を解析する
     * @param rawRequest 生のHTTPリクエスト文字列
     * @return ParsedRequest 解析されたリクエストデータ
     */
    static ParsedRequest parseRequest(const String& rawRequest);

    /**
     * @brief リクエストラインを解析する
     * @param requestLine リクエストライン文字列
     * @param method メソッド名の出力先
     * @param path パスの出力先
     * @param queryString クエリ文字列の出力先
     * @param httpVersion HTTPバージョンの出力先
     * @return bool 解析成功フラグ
     */
    static bool parseRequestLine(const String& requestLine, 
                                String& method, 
                                String& path, 
                                String& queryString,
                                String& httpVersion);

    /**
     * @brief Content-Lengthヘッダーを解析する
     * @param headers ヘッダー文字列
     * @return int Content-Lengthの値（見つからない場合は0）
     */
    static int parseContentLength(const String& headers);

    /**
     * @brief HTTPリクエストからボディを抽出する
     * @param rawRequest 生のHTTPリクエスト文字列
     * @param contentLength 期待するボディサイズ
     * @return String 抽出されたボディ
     */
    static String extractBody(const String& rawRequest, int contentLength);

private:
    /**
     * @brief URLをパスとクエリ文字列に分割する
     * @param url URL文字列
     * @param path パスの出力先
     * @param queryString クエリ文字列の出力先
     */
    static void splitUrl(const String& url, String& path, String& queryString);
};

#endif
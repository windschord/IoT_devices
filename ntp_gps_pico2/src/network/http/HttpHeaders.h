#ifndef HTTP_HEADERS_H
#define HTTP_HEADERS_H

#include <Arduino.h>

/**
 * @brief HTTPヘッダー処理統一化クラス
 * 
 * HTTPヘッダーの解析、生成、アクセスを統一的に処理し、
 * セキュリティヘッダーやキャッシュ制御ヘッダーの管理を行います。
 */
class HttpHeaders {
public:
    /**
     * @brief HTTPヘッダーコンテナクラス
     */
    class Container {
    public:
        Container() = default;

        /**
         * @brief ヘッダー値を取得
         * @param name ヘッダー名（大文字小文字不問）
         * @return String ヘッダー値（存在しない場合は空文字列）
         */
        String get(const String& name) const;

        /**
         * @brief ヘッダーを設定
         * @param name ヘッダー名
         * @param value ヘッダー値
         */
        void set(const String& name, const String& value);

        /**
         * @brief ヘッダーが存在するかチェック
         * @param name ヘッダー名
         * @return bool 存在フラグ
         */
        bool has(const String& name) const;

        /**
         * @brief すべてのヘッダーをクリア
         */
        void clear();

        /**
         * @brief ヘッダー数を取得
         * @return int ヘッダー数
         */
        int count() const;

    private:
        static const int MAX_HEADERS = 20;
        struct HeaderPair {
            String name;
            String value;
            bool used;
        };
        HeaderPair headers_[MAX_HEADERS];
        int headerCount_;

        /**
         * @brief ヘッダー名を正規化（小文字化）
         * @param name ヘッダー名
         * @return String 正規化されたヘッダー名
         */
        String normalizeName(const String& name) const;
    };

    /**
     * @brief 生のHTTPヘッダー文字列を解析
     * @param rawHeaders 生のヘッダー文字列
     * @return Container 解析されたヘッダーコンテナ
     */
    static Container parse(const String& rawHeaders);

    /**
     * @brief セキュリティヘッダーを生成
     * @return String セキュリティヘッダー文字列
     */
    static String generateSecurityHeaders();

    /**
     * @brief キャッシュ無効化ヘッダーを生成
     * @return String キャッシュ無効化ヘッダー文字列
     */
    static String generateNoCacheHeaders();

    /**
     * @brief JSON用の標準ヘッダーを生成
     * @return String JSON用ヘッダー文字列
     */
    static String generateJsonHeaders();

    /**
     * @brief HTML用の標準ヘッダーを生成
     * @return String HTML用ヘッダー文字列
     */
    static String generateHtmlHeaders();

    /**
     * @brief テキスト用の標準ヘッダーを生成
     * @param contentType コンテンツタイプ（デフォルト: text/plain）
     * @return String テキスト用ヘッダー文字列
     */
    static String generateTextHeaders(const String& contentType = "text/plain");

    /**
     * @brief CORS（Cross-Origin Resource Sharing）ヘッダーを生成
     * @param allowedOrigins 許可するオリジン（デフォルト: *）
     * @param allowedMethods 許可するメソッド（デフォルト: GET, POST, OPTIONS）
     * @return String CORSヘッダー文字列
     */
    static String generateCorsHeaders(const String& allowedOrigins = "*", 
                                     const String& allowedMethods = "GET, POST, OPTIONS");

private:
    /**
     * @brief ヘッダー行を解析
     * @param headerLine 単一のヘッダー行
     * @param name ヘッダー名の出力先
     * @param value ヘッダー値の出力先
     * @return bool 解析成功フラグ
     */
    static bool parseHeaderLine(const String& headerLine, String& name, String& value);
};

#endif
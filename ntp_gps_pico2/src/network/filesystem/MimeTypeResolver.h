#ifndef MIME_TYPE_RESOLVER_H
#define MIME_TYPE_RESOLVER_H

#include <Arduino.h>

/**
 * @brief MIMEタイプ判定クラス
 * 
 * ファイル拡張子やファイル内容に基づいてMIMEタイプを判定し、
 * 適切なContent-Typeヘッダーを生成します。
 */
class MimeTypeResolver {
public:
    /**
     * @brief MIMEタイプ情報構造体
     */
    struct MimeInfo {
        String mimeType;        // MIMEタイプ
        String charset;         // 文字セット
        bool isText;           // テキストファイルフラグ
        bool isCompressible;   // 圧縮可能フラグ
    };

    /**
     * @brief ファイルパスからMIMEタイプを取得
     * @param filepath ファイルパス
     * @return String MIMEタイプ
     */
    static String getMimeType(const String& filepath);

    /**
     * @brief ファイルパスから詳細なMIME情報を取得
     * @param filepath ファイルパス
     * @return MimeInfo MIME情報
     */
    static MimeInfo getMimeInfo(const String& filepath);

    /**
     * @brief ファイル拡張子からMIMEタイプを取得
     * @param extension 拡張子（ピリオド付き、例: ".html"）
     * @return String MIMEタイプ
     */
    static String getMimeTypeByExtension(const String& extension);

    /**
     * @brief ファイル内容からMIMEタイプを推定
     * @param content ファイル内容（最初の数バイト）
     * @param size 内容サイズ
     * @return String 推定MIMEタイプ
     */
    static String getMimeTypeByContent(const uint8_t* content, size_t size);

    /**
     * @brief Content-Typeヘッダー文字列を生成
     * @param filepath ファイルパス
     * @return String Content-Typeヘッダー値
     */
    static String generateContentTypeHeader(const String& filepath);

    /**
     * @brief テキストファイルかどうか判定
     * @param mimeType MIMEタイプ
     * @return bool テキストファイルフラグ
     */
    static bool isTextFile(const String& mimeType);

    /**
     * @brief 画像ファイルかどうか判定
     * @param mimeType MIMEタイプ
     * @return bool 画像ファイルフラグ
     */
    static bool isImageFile(const String& mimeType);

    /**
     * @brief JavaScriptファイルかどうか判定
     * @param mimeType MIMEタイプ
     * @return bool JavaScriptファイルフラグ
     */
    static bool isJavaScriptFile(const String& mimeType);

    /**
     * @brief CSSファイルかどうか判定
     * @param mimeType MIMEタイプ
     * @return bool CSSファイルフラグ
     */
    static bool isCssFile(const String& mimeType);

    /**
     * @brief 圧縮可能なファイルかどうか判定
     * @param mimeType MIMEタイプ
     * @return bool 圧縮可能フラグ
     */
    static bool isCompressible(const String& mimeType);

    /**
     * @brief デフォルトのMIMEタイプを取得
     * @return String デフォルトMIMEタイプ
     */
    static String getDefaultMimeType();

private:
    /**
     * @brief MIMEタイプマッピング構造体
     */
    struct MimeTypeMapping {
        const char* extension;
        const char* mimeType;
        const char* charset;
        bool isText;
        bool isCompressible;
    };

    /**
     * @brief MIMEタイプマッピングテーブル
     */
    static const MimeTypeMapping mimeTypes_[];

    /**
     * @brief マッピングテーブルサイズ
     */
    static const int mimeTypesCount_;

    /**
     * @brief ファイルパスから拡張子を抽出
     * @param filepath ファイルパス
     * @return String 拡張子（小文字、ピリオド付き）
     */
    static String extractExtension(const String& filepath);

    /**
     * @brief バイナリ検索でMIMEタイプを検索
     * @param extension 拡張子
     * @return const MimeTypeMapping* 見つかったマッピング（なければnullptr）
     */
    static const MimeTypeMapping* findMimeTypeMapping(const String& extension);

    /**
     * @brief ファイルシグネチャーを確認
     * @param content ファイル内容
     * @param size 内容サイズ
     * @param signature チェックするシグネチャー
     * @param sigSize シグネチャーサイズ
     * @return bool シグネチャーマッチフラグ
     */
    static bool checkSignature(const uint8_t* content, size_t size, 
                              const uint8_t* signature, size_t sigSize);
};

#endif
#ifndef FILESYSTEM_HANDLER_H
#define FILESYSTEM_HANDLER_H

#include <Arduino.h>
#include <LittleFS.h>

// Forward declaration
class LoggingService;

/**
 * @brief LittleFS操作専用クラス
 * 
 * ファイルシステムの初期化、ファイル操作、エラーハンドリングを
 * 統一的に処理し、安全なファイル操作を提供します。
 */
class FileSystemHandler {
public:
    /**
     * @brief ファイル情報構造体
     */
    struct FileInfo {
        String name;         // ファイル名
        size_t size;         // ファイルサイズ
        bool isDirectory;    // ディレクトリフラグ
        bool exists;         // 存在フラグ
    };

    /**
     * @brief ファイル操作結果構造体
     */
    struct OperationResult {
        bool success;        // 操作成功フラグ
        String errorMessage; // エラーメッセージ
        size_t bytesRead;    // 読み込みバイト数
        size_t bytesWritten; // 書き込みバイト数
    };

    /**
     * @brief FileSystemHandlerコンストラクター
     */
    FileSystemHandler();

    /**
     * @brief 依存性注入メソッド
     */
    void setLoggingService(LoggingService* loggingService) { loggingService_ = loggingService; }

    /**
     * @brief ファイルシステム初期化
     * @param autoFormat 初期化失敗時の自動フォーマット
     * @return bool 初期化成功フラグ
     */
    bool initialize(bool autoFormat = true);

    /**
     * @brief ファイルシステムマウント状態チェック
     * @return bool マウント状態フラグ
     */
    bool isMounted() const;

    /**
     * @brief ファイル存在チェック
     * @param filepath ファイルパス
     * @return bool 存在フラグ
     */
    bool fileExists(const String& filepath);

    /**
     * @brief ファイル情報取得
     * @param filepath ファイルパス
     * @return FileInfo ファイル情報
     */
    FileInfo getFileInfo(const String& filepath);

    /**
     * @brief ファイル読み込み（全体）
     * @param filepath ファイルパス
     * @param content 読み込み内容の出力先
     * @return OperationResult 操作結果
     */
    OperationResult readFile(const String& filepath, String& content);

    /**
     * @brief ファイル読み込み（バイナリ）
     * @param filepath ファイルパス
     * @param buffer バッファ
     * @param bufferSize バッファサイズ
     * @return OperationResult 操作結果
     */
    OperationResult readFile(const String& filepath, uint8_t* buffer, size_t bufferSize);

    /**
     * @brief ファイル書き込み
     * @param filepath ファイルパス
     * @param content 書き込み内容
     * @param append 追記フラグ
     * @return OperationResult 操作結果
     */
    OperationResult writeFile(const String& filepath, const String& content, bool append = false);

    /**
     * @brief ファイル書き込み（バイナリ）
     * @param filepath ファイルパス
     * @param buffer バッファ
     * @param size 書き込みサイズ
     * @param append 追記フラグ
     * @return OperationResult 操作結果
     */
    OperationResult writeFile(const String& filepath, const uint8_t* buffer, size_t size, bool append = false);

    /**
     * @brief ファイル削除
     * @param filepath ファイルパス
     * @return bool 削除成功フラグ
     */
    bool deleteFile(const String& filepath);

    /**
     * @brief ファイルコピー
     * @param srcPath ソースファイルパス
     * @param destPath 宛先ファイルパス
     * @return OperationResult 操作結果
     */
    OperationResult copyFile(const String& srcPath, const String& destPath);

    /**
     * @brief ディレクトリ作成
     * @param dirPath ディレクトリパス
     * @return bool 作成成功フラグ
     */
    bool createDirectory(const String& dirPath);

    /**
     * @brief ディレクトリ一覧取得
     * @param dirPath ディレクトリパス
     * @param fileList ファイル一覧の出力先
     * @param maxFiles 最大ファイル数
     * @return int 取得したファイル数
     */
    int listDirectory(const String& dirPath, FileInfo* fileList, int maxFiles);

    /**
     * @brief ファイルシステム統計取得
     * @param totalBytes 総バイト数の出力先
     * @param usedBytes 使用バイト数の出力先
     * @return bool 取得成功フラグ
     */
    bool getFilesystemStats(size_t& totalBytes, size_t& usedBytes);

    /**
     * @brief ファイルシステムフォーマット
     * @return bool フォーマット成功フラグ
     */
    bool formatFilesystem();

    /**
     * @brief 安全なファイルオープン
     * @param filepath ファイルパス
     * @param mode オープンモード ("r", "w", "a")
     * @return File ファイルハンドル
     */
    File openFile(const String& filepath, const String& mode);

    /**
     * @brief ファイルクローズと後処理
     * @param file ファイルハンドル
     */
    void closeFile(File& file);

private:
    LoggingService* loggingService_;
    bool isMounted_;

    /**
     * @brief エラーログ出力
     * @param operation 操作名
     * @param filepath ファイルパス
     * @param error エラーメッセージ
     */
    void logError(const String& operation, const String& filepath, const String& error);

    /**
     * @brief 情報ログ出力
     * @param operation 操作名
     * @param filepath ファイルパス
     * @param info 情報メッセージ
     */
    void logInfo(const String& operation, const String& filepath, const String& info);

    /**
     * @brief パスバリデーション
     * @param filepath ファイルパス
     * @return bool バリデーション成功フラグ
     */
    bool validatePath(const String& filepath);

    /**
     * @brief ファイルサイズ制限チェック
     * @param size ファイルサイズ
     * @return bool サイズ許可フラグ
     */
    bool checkSizeLimit(size_t size);

    // 設定定数
    static const size_t MAX_FILE_SIZE = 1024 * 1024; // 1MB制限
    static const size_t BUFFER_SIZE = 512;           // 読み書きバッファサイズ
};

#endif
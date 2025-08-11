#ifndef CACHE_MANAGER_H
#define CACHE_MANAGER_H

#include <Arduino.h>

/**
 * @brief レスポンスキャッシュ管理クラス
 * 
 * 静的ファイルやAPI レスポンスのキャッシュを管理し、
 * メモリ効率の良いキャッシュ戦略を提供します。
 */
class CacheManager {
public:
    /**
     * @brief キャッシュエントリ構造体
     */
    struct CacheEntry {
        String key;              // キャッシュキー（通常はファイルパス）
        String content;          // キャッシュされた内容
        String mimeType;         // MIME タイプ
        unsigned long timestamp; // キャッシュした時刻
        unsigned long expires;   // 有効期限（秒）
        size_t accessCount;      // アクセス回数
        bool isValid;           // 有効フラグ
    };

    /**
     * @brief キャッシュ統計情報
     */
    struct CacheStats {
        int totalEntries;        // 総エントリ数
        int validEntries;        // 有効エントリ数
        size_t totalSize;        // 総キャッシュサイズ
        size_t hitCount;         // ヒット回数
        size_t missCount;        // ミス回数
        float hitRatio;          // ヒット率
    };

    /**
     * @brief CacheManagerコンストラクター
     * @param maxEntries 最大キャッシュエントリ数
     * @param maxTotalSize 最大総キャッシュサイズ（バイト）
     */
    CacheManager(int maxEntries = 10, size_t maxTotalSize = 8192);

    /**
     * @brief キャッシュエントリを追加/更新
     * @param key キャッシュキー
     * @param content コンテンツ
     * @param mimeType MIME タイプ
     * @param ttlSeconds TTL（Time To Live）秒数
     * @return bool 追加成功フラグ
     */
    bool put(const String& key, const String& content, const String& mimeType, unsigned long ttlSeconds);

    /**
     * @brief キャッシュエントリを取得
     * @param key キャッシュキー
     * @return CacheEntry* キャッシュエントリ（見つからない場合はnullptr）
     */
    CacheEntry* get(const String& key);

    /**
     * @brief キャッシュエントリが存在するかチェック
     * @param key キャッシュキー
     * @return bool 存在フラグ
     */
    bool has(const String& key);

    /**
     * @brief キャッシュエントリを削除
     * @param key キャッシュキー
     * @return bool 削除成功フラグ
     */
    bool remove(const String& key);

    /**
     * @brief すべてのキャッシュエントリをクリア
     */
    void clear();

    /**
     * @brief 期限切れエントリを削除
     * @return int 削除されたエントリ数
     */
    int cleanupExpired();

    /**
     * @brief キャッシュ統計情報を取得
     * @return CacheStats 統計情報
     */
    CacheStats getStats();

    /**
     * @brief キャッシュサイズを最適化
     * @param targetSizePercent 目標サイズ（現在サイズの何％）
     * @return int 削除されたエントリ数
     */
    int optimize(int targetSizePercent = 80);

    /**
     * @brief 特定のパターンにマッチするキーのエントリを削除
     * @param pattern パターン（ワイルドカード * 使用可能）
     * @return int 削除されたエントリ数
     */
    int removeByPattern(const String& pattern);

    /**
     * @brief キャッシュヒット率を取得
     * @return float ヒット率（0.0-1.0）
     */
    float getHitRatio();

    /**
     * @brief 現在のキャッシュサイズを取得
     * @return size_t キャッシュサイズ（バイト）
     */
    size_t getCurrentSize();

    /**
     * @brief 使用可能キャッシュ容量を取得
     * @return size_t 使用可能容量（バイト）
     */
    size_t getAvailableSize();

    /**
     * @brief デバッグ情報を出力
     */
    void printDebugInfo();

private:
    CacheEntry* entries_;        // キャッシュエントリ配列
    int maxEntries_;            // 最大エントリ数
    size_t maxTotalSize_;       // 最大総サイズ
    int entryCount_;            // 現在のエントリ数
    size_t currentSize_;        // 現在の総サイズ
    size_t hitCount_;           // ヒット回数
    size_t missCount_;          // ミス回数

    /**
     * @brief キャッシュエントリを検索
     * @param key キャッシュキー
     * @return int エントリインデックス（見つからない場合は-1）
     */
    int findEntry(const String& key);

    /**
     * @brief 空のエントリを検索
     * @return int 空のエントリインデックス（見つからない場合は-1）
     */
    int findEmptyEntry();

    /**
     * @brief LRU（Least Recently Used）エントリを検索
     * @return int LRUエントリインデックス
     */
    int findLRUEntry();

    /**
     * @brief エントリが期限切れかチェック
     * @param entry キャッシュエントリ
     * @return bool 期限切れフラグ
     */
    bool isExpired(const CacheEntry& entry);

    /**
     * @brief エントリサイズを計算
     * @param entry キャッシュエントリ
     * @return size_t エントリサイズ（バイト）
     */
    size_t calculateEntrySize(const CacheEntry& entry);

    /**
     * @brief パターンマッチング
     * @param pattern パターン
     * @param text マッチ対象テキスト
     * @return bool マッチフラグ
     */
    bool matchPattern(const String& pattern, const String& text);

    /**
     * @brief エントリをクリア
     * @param index エントリインデックス
     */
    void clearEntry(int index);

    /**
     * @brief エントリの最終アクセス時刻を更新
     * @param index エントリインデックス
     */
    void touchEntry(int index);
};

#endif
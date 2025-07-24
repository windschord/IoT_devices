#ifndef STORAGE_HAL_H
#define STORAGE_HAL_H

#include <Arduino.h>

// ストレージ設定定数
#define STORAGE_SECTOR_SIZE        4096    // 4KB セクターサイズ
#define STORAGE_CONFIG_OFFSET      0       // 設定データ開始オフセット
#define STORAGE_CONFIG_MAX_SIZE    2048    // 設定データ最大サイズ (2KB)
#define STORAGE_BACKUP_OFFSET      2048    // バックアップ開始オフセット
#define STORAGE_MAGIC_NUMBER       0x47505341  // "GPSA" - GPS NTP Server Config

// CRC32 関連
#define CRC32_POLYNOMIAL           0xEDB88320UL
#define CRC32_INITIAL_VALUE        0xFFFFFFFFUL

// ストレージ操作結果
enum StorageResult {
    STORAGE_SUCCESS,              // 操作成功
    STORAGE_ERROR_INIT,           // 初期化エラー
    STORAGE_ERROR_READ,           // 読み取りエラー
    STORAGE_ERROR_WRITE,          // 書き込みエラー
    STORAGE_ERROR_CRC,            // CRC32エラー
    STORAGE_ERROR_MAGIC,          // マジックナンバーエラー
    STORAGE_ERROR_SIZE,           // サイズエラー
    STORAGE_ERROR_CORRUPTION      // データ破損エラー
};

// 設定データヘッダー構造
struct ConfigHeader {
    uint32_t magic;               // マジックナンバー (STORAGE_MAGIC_NUMBER)
    uint16_t size;                // データサイズ
    uint16_t version;             // 設定バージョン
    uint32_t crc32;               // CRC32チェックサム
    uint32_t timestamp;           // 最終更新タイムスタンプ
    uint32_t reserved[2];         // 将来の拡張用
};

class StorageHAL {
public:
    StorageHAL();
    ~StorageHAL();

    // 初期化と終了処理
    bool initialize();
    void shutdown();

    // データ読み書き
    StorageResult readConfig(void* data, uint16_t size);
    StorageResult writeConfig(const void* data, uint16_t size);

    // データ検証
    bool verifyConfig(const void* data, uint16_t size, uint32_t expected_crc);
    bool isConfigValid();

    // CRC32計算
    static uint32_t calculateCRC32(const void* data, size_t length);
    static uint32_t calculateCRC32(const void* data, size_t length, uint32_t initial_crc);

    // 工場出荷時リセット
    StorageResult factoryReset();

    // 診断機能
    void printStatus() const;
    StorageResult performSelfTest();

    // ストレージ情報取得
    size_t getAvailableSpace() const;
    uint32_t getLastWriteTimestamp() const;
    bool isPowerSafeWrite() const;

private:
    bool initialized;
    uint32_t last_write_timestamp;
    bool power_safe_mode;

    // 内部処理
    StorageResult writeHeader(const ConfigHeader& header, uint32_t offset);
    StorageResult readHeader(ConfigHeader& header, uint32_t offset);
    StorageResult writeData(const void* data, uint16_t size, uint32_t offset);
    StorageResult readData(void* data, uint16_t size, uint32_t offset);

    // 電源断保護
    bool checkPowerStability();
    void enablePowerSafeMode();
    void disablePowerSafeMode();

    // フラッシュメモリアクセス
    bool flashWrite(uint32_t address, const void* data, size_t size);
    bool flashRead(uint32_t address, void* data, size_t size);
    bool flashErase(uint32_t address, size_t size);

    // CRC32テーブル（最適化用）
    static const uint32_t crc32_table[256];
    static bool crc32_table_initialized;
    static void initializeCRC32Table();
};

extern StorageHAL g_storage_hal;

#endif // STORAGE_HAL_H
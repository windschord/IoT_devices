#pragma once

#include <Arduino.h>
#include "../interfaces/IService.h"
#include "../interfaces/IHardwareInterface.h"

// 組み込み環境向けの定数定義
#define MAX_SERVICES 16
#define MAX_HARDWARE 8
#define MAX_NAME_LENGTH 32

/**
 * @brief 依存性注入（DI）コンテナ - 組み込み環境向け軽量版
 * 
 * サービスとハードウェアインスタンスの登録・取得を管理する。
 * std::map等の重いSTLコンテナを使わず、固定配列で実装。
 */
class ServiceContainer {
public:
    /**
     * @brief サービスファクトリ関数型
     */
    typedef IService* (*ServiceFactory)();
    typedef IHardwareInterface* (*HardwareFactory)();
    
    /**
     * @brief シングルトンインスタンス取得
     */
    static ServiceContainer& getInstance();
    
    // コピー・代入禁止
    ServiceContainer(const ServiceContainer&) = delete;
    ServiceContainer& operator=(const ServiceContainer&) = delete;
    
    // ========== サービス登録・取得 ==========
    
    /**
     * @brief サービス登録
     * @param name サービス名
     * @param factory サービス生成ファクトリ
     * @return true 成功, false 失敗
     */
    bool registerService(const char* name, ServiceFactory factory);
    
    /**
     * @brief サービス取得
     * @param name サービス名
     * @return サービスインスタンス（nullptrの場合は未登録）
     */
    IService* getService(const char* name);
    
    /**
     * @brief テンプレート版サービス取得
     * @tparam T サービス型
     * @param name サービス名
     * @return キャストされたサービスインスタンス
     */
    template<typename T>
    T* getService(const char* name) {
        return static_cast<T*>(getService(name));
    }
    
    // ========== ハードウェア登録・取得 ==========
    
    /**
     * @brief ハードウェア登録
     * @param name ハードウェア名
     * @param factory ハードウェア生成ファクトリ
     * @return true 成功, false 失敗
     */
    bool registerHardware(const char* name, HardwareFactory factory);
    
    /**
     * @brief ハードウェア取得
     * @param name ハードウェア名
     * @return ハードウェアインスタンス（nullptrの場合は未登録）
     */
    IHardwareInterface* getHardware(const char* name);
    
    /**
     * @brief テンプレート版ハードウェア取得
     * @tparam T ハードウェア型
     * @param name ハードウェア名
     * @return キャストされたハードウェアインスタンス
     */
    template<typename T>
    T* getHardware(const char* name) {
        return static_cast<T*>(getHardware(name));
    }
    
    // ========== ライフサイクル管理 ==========
    
    /**
     * @brief 全サービス・ハードウェア初期化
     * 依存関係順序を考慮して初期化を実行
     * @return true 全て成功, false 一部または全て失敗
     */
    bool initializeAll();
    
    /**
     * @brief 全サービス開始
     * @return true 全て成功, false 一部または全て失敗
     */
    bool startAll();
    
    /**
     * @brief 全サービス停止
     */
    void stopAll();
    
    /**
     * @brief 登録済みサービス数取得
     */
    int getServiceCount() const { return serviceCount; }
    
    /**
     * @brief 登録済みハードウェア数取得
     */
    int getHardwareCount() const { return hardwareCount; }
    
    /**
     * @brief コンテナクリア（テスト用）
     */
    void clear();
    
    // ========== デバッグ・診断 ==========
    
    /**
     * @brief 登録済みサービス一覧出力
     */
    void listServices() const;
    
    /**
     * @brief 登録済みハードウェア一覧出力
     */
    void listHardware() const;

private:
    ServiceContainer();
    ~ServiceContainer();
    
    // サービス管理（固定配列）
    struct ServiceEntry {
        char name[MAX_NAME_LENGTH];
        ServiceFactory factory;
        IService* instance;
        bool initialized;
        bool started;
    };
    
    // ハードウェア管理（固定配列）
    struct HardwareEntry {
        char name[MAX_NAME_LENGTH];
        HardwareFactory factory;
        IHardwareInterface* instance;
        bool initialized;
    };
    
    ServiceEntry services[MAX_SERVICES];
    HardwareEntry hardware[MAX_HARDWARE];
    int serviceCount;
    int hardwareCount;
    
    // 内部ヘルパー
    int findService(const char* name);
    int findHardware(const char* name);
    IService* createServiceIfNeeded(int index);
    IHardwareInterface* createHardwareIfNeeded(int index);
    void clearServiceEntry(ServiceEntry* entry);
    void clearHardwareEntry(HardwareEntry* entry);
    bool copyString(char* dest, const char* src, size_t maxLen);
};
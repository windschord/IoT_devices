#pragma once

/**
 * @brief サービス基底インターフェース
 * 
 * すべてのサービスクラスが継承すべき基底インターフェース。
 * 依存性注入コンテナでの管理を可能にする。
 */
class IService {
public:
    virtual ~IService() = default;
    
    /**
     * @brief サービス初期化
     * @return true 成功, false 失敗
     */
    virtual bool initialize() = 0;
    
    /**
     * @brief サービス開始
     * @return true 成功, false 失敗
     */
    virtual bool start() = 0;
    
    /**
     * @brief サービス停止
     */
    virtual void stop() = 0;
    
    /**
     * @brief サービス状態確認
     * @return true 正常動作中, false 停止または異常
     */
    virtual bool isRunning() const = 0;
    
    /**
     * @brief サービス名取得
     * @return サービス名
     */
    virtual const char* getName() const = 0;
};
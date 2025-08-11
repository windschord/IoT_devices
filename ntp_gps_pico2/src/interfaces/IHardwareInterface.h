#pragma once

/**
 * @brief ハードウェアインターフェース基底クラス
 * 
 * ハードウェア抽象化レイヤー（HAL）の共通インターフェース。
 * すべてのハードウェアクラスが継承すべき基底インターフェース。
 */
class IHardwareInterface {
public:
    virtual ~IHardwareInterface() = default;
    
    /**
     * @brief ハードウェア初期化
     * @return true 成功, false 失敗
     */
    virtual bool initialize() = 0;
    
    /**
     * @brief ハードウェア状態確認
     * @return true 正常, false 異常
     */
    virtual bool isReady() const = 0;
    
    /**
     * @brief ハードウェアリセット
     * @return true 成功, false 失敗
     */
    virtual bool reset() = 0;
    
    /**
     * @brief ハードウェア名取得
     * @return ハードウェア名
     */
    virtual const char* getHardwareName() const = 0;
    
    /**
     * @brief 最後のエラー情報取得
     * @return エラー情報文字列（エラーがない場合はnullptr）
     */
    virtual const char* getLastError() const = 0;
};

/**
 * @brief GPS インターフェース
 */
class IGpsInterface : public IHardwareInterface {
public:
    virtual bool hasFixedPosition() const = 0;
    virtual bool isPpsSignalActive() const = 0;
    virtual unsigned long getLastPpsTime() const = 0;
    virtual int getSatelliteCount() const = 0;
};

/**
 * @brief ネットワーク インターフェース
 */
class INetworkInterface : public IHardwareInterface {
public:
    virtual bool isConnected() const = 0;
    virtual const char* getIpAddress() const = 0;
    virtual bool dhcpEnabled() const = 0;
    virtual unsigned long getLastPacketTime() const = 0;
};

/**
 * @brief ディスプレイ インターフェース
 */
class IDisplayInterface : public IHardwareInterface {
public:
    virtual void clear() = 0;
    virtual void displayText(const char* text, int line = 0) = 0;
    virtual void setBrightness(uint8_t brightness) = 0;
    virtual bool isDisplayOn() const = 0;
};
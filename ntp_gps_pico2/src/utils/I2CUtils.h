#ifndef I2C_UTILS_H
#define I2C_UTILS_H

#include <Arduino.h>
#include <Wire.h>
#include "LogUtils.h"

/**
 * @brief I2C通信の共通処理を提供するユーティリティクラス
 * 
 * 複数のモジュール（GPS、OLED、RTC）で重複していたI2C通信処理を
 * 統一化し、エラーハンドリングと再試行機能を提供する。
 */
class I2CUtils {
public:
    // I2C通信結果の列挙型
    enum I2CResult {
        I2C_SUCCESS = 0,
        I2C_ERROR_TIMEOUT = 1,
        I2C_ERROR_ADDRESS_NACK = 2,
        I2C_ERROR_DATA_NACK = 3,
        I2C_ERROR_OTHER = 4,
        I2C_ERROR_BUFFER_OVERFLOW = 5
    };

    /**
     * @brief I2Cバスの初期化（統一処理・最適化版）
     * @param wire I2Cバスインスタンス
     * @param sda_pin SDAピン番号
     * @param scl_pin SCLピン番号
     * @param clock_speed クロック速度（デフォルト: 100kHz安定動作）
     * @param enable_pullups プルアップ抵抗有効化（デフォルト: true）
     * @return true: 成功, false: 失敗
     */
    static bool initializeBus(TwoWire& wire, uint8_t sda_pin, uint8_t scl_pin, 
                             uint32_t clock_speed = 100000, bool enable_pullups = true) {
        // ピン設定前の安全化
        wire.end(); // 既存の接続をクリア
        delay(10);
        
        // ピン設定
        wire.setSDA(sda_pin);
        wire.setSCL(scl_pin);
        
        // 改善されたプルアップ抵抗設定
        if (enable_pullups) {
            // 強力なプルアップ抵抗を有効化（Pico2特有の設定）
            pinMode(sda_pin, INPUT_PULLUP);
            pinMode(scl_pin, INPUT_PULLUP);
            
            // 追加の安定化遅延
            delay(20);
        }
        
        // I2C初期化（エラーハンドリング強化）
        wire.begin();
        
        // 段階的クロック設定（安定性向上）
        if (clock_speed > 100000) {
            // 高速モード要求の場合は段階的に上げる
            wire.setClock(50000);   // 50kHz
            delay(50);
            wire.setClock(100000);  // 100kHz
            delay(50);
            wire.setClock(clock_speed);
        } else {
            // 標準100kHzまたはそれ以下
            wire.setClock(clock_speed);
        }
        
        delay(50); // 設定安定化
        
        // バス動作確認（より詳細なテスト）
        return validateBusOperation(wire);
    }
    
    /**
     * @brief I2Cバス動作検証
     * @param wire I2Cバスインスタンス
     * @return true: バス正常, false: バス異常
     */
    static bool validateBusOperation(TwoWire& wire) {
        // バス占有状態チェック（疑似テスト）
        for (int retry = 0; retry < 3; retry++) {
            wire.beginTransmission(0x00); // General Call Address
            uint8_t result = wire.endTransmission();
            
            // エラーコード2（Address NACK）は予想される応答（正常）
            if (result == 0 || result == 2) {
                return true;
            }
            
            // バスリセット試行
            delay(10);
        }
        
        return false; // バス異常
    }

    /**
     * @brief I2Cデバイスの存在確認
     * @param wire I2Cバスインスタンス
     * @param address デバイスアドレス
     * @param retry_count 再試行回数（デフォルト: 3）
     * @return true: デバイス検出, false: デバイス未検出
     */
    static bool testDevice(TwoWire& wire, uint8_t address, uint8_t retry_count = 3) {
        for (uint8_t i = 0; i < retry_count; i++) {
            wire.beginTransmission(address);
            uint8_t error = wire.endTransmission();
            
            if (error == 0) {
                return true; // デバイス検出成功
            }
            
            // 再試行前の短い待機
            if (i < retry_count - 1) {
                delay(10);
            }
        }
        
        return false; // 全ての試行で失敗
    }

    /**
     * @brief I2Cバスのスキャン（デバイス検出）
     * @param wire I2Cバスインスタンス
     * @param found_devices 検出されたデバイスアドレスの格納配列
     * @param max_devices 最大検出デバイス数
     * @param logger ログサービス（オプション）
     * @param component_name ログ用コンポーネント名
     * @return 検出されたデバイス数
     */
    static uint8_t scanBus(TwoWire& wire, uint8_t* found_devices, uint8_t max_devices,
                          LoggingService* logger = nullptr, const char* component_name = "I2C") {
        uint8_t device_count = 0;
        
        LogUtils::logInfo(logger, component_name, "Starting I2C bus scan...");
        
        for (uint8_t address = 1; address < 127 && device_count < max_devices; address++) {
            if (testDevice(wire, address, 1)) {
                found_devices[device_count] = address;
                device_count++;
                
                // ログ出力（見つかったデバイス）
                if (logger) {
                    char msg[64];
                    snprintf(msg, sizeof(msg), "Device found at address 0x%02X", address);
                    LogUtils::logInfo(logger, component_name, msg);
                }
            }
        }
        
        // スキャン結果のサマリー
        if (logger) {
            char summary[64];
            snprintf(summary, sizeof(summary), "I2C scan completed: %d devices found", device_count);
            LogUtils::logInfo(logger, component_name, summary);
        }
        
        return device_count;
    }

    /**
     * @brief I2C通信エラーコードの文字列変換
     * @param error_code エラーコード
     * @return エラー説明文字列
     */
    static const char* getErrorString(uint8_t error_code) {
        switch (error_code) {
            case 0: return "Success";
            case 1: return "Timeout";
            case 2: return "Address NACK";
            case 3: return "Data NACK";
            case 4: return "Other error";
            case 5: return "Buffer overflow";
            default: return "Unknown error";
        }
    }

    /**
     * @brief 安全なI2C読み取り（エラーハンドリング付き）
     * @param wire I2Cバスインスタンス
     * @param address デバイスアドレス
     * @param reg_address レジスタアドレス
     * @param buffer 読み取りバッファ
     * @param length 読み取りバイト数
     * @param retry_count 再試行回数
     * @return I2CResult エラーコード
     */
    static I2CResult safeRead(TwoWire& wire, uint8_t address, uint8_t reg_address,
                             uint8_t* buffer, uint8_t length, uint8_t retry_count = 3) {
        if (!buffer || length == 0) {
            return I2C_ERROR_OTHER;
        }
        
        for (uint8_t retry = 0; retry < retry_count; retry++) {
            // レジスタアドレス送信
            wire.beginTransmission(address);
            wire.write(reg_address);
            uint8_t error = wire.endTransmission(false); // リスタート条件
            
            if (error != 0) {
                if (retry < retry_count - 1) {
                    delay(5);
                    continue;
                }
                return static_cast<I2CResult>(error);
            }
            
            // データ読み取り
            uint8_t received = wire.requestFrom(address, length);
            if (received < length) {
                if (retry < retry_count - 1) {
                    delay(5);
                    continue;
                }
                return I2C_ERROR_TIMEOUT;
            }
            
            // バッファへの読み取り
            for (uint8_t i = 0; i < length; i++) {
                if (wire.available()) {
                    buffer[i] = wire.read();
                } else {
                    return I2C_ERROR_TIMEOUT;
                }
            }
            
            return I2C_SUCCESS;
        }
        
        return I2C_ERROR_OTHER;
    }

    /**
     * @brief 安全なI2C書き込み（エラーハンドリング付き）
     * @param wire I2Cバスインスタンス
     * @param address デバイスアドレス
     * @param reg_address レジスタアドレス
     * @param data 書き込みデータ
     * @param length 書き込みバイト数
     * @param retry_count 再試行回数
     * @return I2CResult エラーコード
     */
    static I2CResult safeWrite(TwoWire& wire, uint8_t address, uint8_t reg_address,
                              const uint8_t* data, uint8_t length, uint8_t retry_count = 3) {
        if (!data || length == 0) {
            return I2C_ERROR_OTHER;
        }
        
        for (uint8_t retry = 0; retry < retry_count; retry++) {
            wire.beginTransmission(address);
            wire.write(reg_address);
            
            for (uint8_t i = 0; i < length; i++) {
                wire.write(data[i]);
            }
            
            uint8_t error = wire.endTransmission();
            if (error == 0) {
                return I2C_SUCCESS;
            }
            
            if (retry < retry_count - 1) {
                delay(5);
            }
        }
        
        return I2C_ERROR_OTHER;
    }
};

#endif // I2C_UTILS_H
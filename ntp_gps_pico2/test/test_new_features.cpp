#include <unity.h>
#include <Arduino.h>

// 新機能のテスト（タスク12-15の実装）
#include "Button_HAL.h"
#include "Storage_HAL.h"
#include "PhysicalReset.h"
#include "ConfigManager.h"
#include "ErrorHandler.h"
#include "DisplayManager.h"

// テスト用のモッククラス
class MockDisplayManager : public DisplayManager {
public:
    int next_mode_calls = 0;
    int trigger_display_calls = 0;
    String error_message = "";
    
    void nextDisplayMode() override { next_mode_calls++; }
    void triggerDisplay() { trigger_display_calls++; }
    void setErrorState(const String& msg) { error_message = msg; }
    void wakeDisplay() override {}
};

class MockConfigManager : public ConfigManager {
public:
    bool reset_called = false;
    
    void resetToDefaults() { reset_called = true; }
};

// テストクラス
class TestNewFeatures {
public:
    static void test_button_hal_initialization() {
        ButtonHAL button_hal;
        
        TEST_ASSERT_TRUE(button_hal.initialize());
        TEST_ASSERT_EQUAL(BUTTON_IDLE, button_hal.getState());
        TEST_ASSERT_FALSE(button_hal.isPressed());
        TEST_ASSERT_EQUAL(0, button_hal.getPressedDuration());
        
        button_hal.shutdown();
    }
    
    static void test_button_hal_state_management() {
        ButtonHAL button_hal;
        button_hal.initialize();
        
        // 初期状態確認
        TEST_ASSERT_EQUAL(BUTTON_IDLE, button_hal.getState());
        
        // 状態変更テスト（シミュレーション）
        // 実際のハードウェアテストでは物理的なボタン操作が必要
        
        button_hal.shutdown();
    }
    
    static void test_storage_hal_initialization() {
        StorageHAL storage_hal;
        
        TEST_ASSERT_TRUE(storage_hal.initialize());
        TEST_ASSERT_TRUE(storage_hal.getAvailableSpace() > 0);
        TEST_ASSERT_TRUE(storage_hal.isPowerSafeWrite());
        
        storage_hal.shutdown();
    }
    
    static void test_storage_hal_crc32_calculation() {
        StorageHAL storage_hal;
        
        // CRC32計算テスト
        const char* test_data = "Hello, World!";
        uint32_t crc1 = StorageHAL::calculateCRC32(test_data, strlen(test_data));
        uint32_t crc2 = StorageHAL::calculateCRC32(test_data, strlen(test_data));
        
        // 同じデータは同じCRC32を生成
        TEST_ASSERT_EQUAL(crc1, crc2);
        
        // 異なるデータは異なるCRC32を生成
        const char* different_data = "Hello, World?";
        uint32_t crc3 = StorageHAL::calculateCRC32(different_data, strlen(different_data));
        TEST_ASSERT_NOT_EQUAL(crc1, crc3);
        
        // 既知の値でのテスト（RFC 3309の例）
        const char* rfc_data = "123456789";
        uint32_t expected_crc = 0xCBF43926; // 既知のCRC32値
        uint32_t actual_crc = StorageHAL::calculateCRC32(rfc_data, strlen(rfc_data));
        TEST_ASSERT_EQUAL(expected_crc, actual_crc);
    }
    
    static void test_storage_hal_read_write() {
        StorageHAL storage_hal;
        storage_hal.initialize();
        
        // テスト用設定データ
        struct TestConfig {
            char name[16];
            uint32_t value;
            bool flag;
        } test_config = {"test", 42, true};
        
        // 書き込みテスト
        StorageResult write_result = storage_hal.writeConfig(&test_config, sizeof(test_config));
        TEST_ASSERT_EQUAL(STORAGE_SUCCESS, write_result);
        
        // 読み込みテスト
        struct TestConfig read_config = {};
        StorageResult read_result = storage_hal.readConfig(&read_config, sizeof(read_config));
        TEST_ASSERT_EQUAL(STORAGE_SUCCESS, read_result);
        
        // データ検証
        TEST_ASSERT_EQUAL_STRING(test_config.name, read_config.name);
        TEST_ASSERT_EQUAL(test_config.value, read_config.value);
        TEST_ASSERT_EQUAL(test_config.flag, read_config.flag);
        
        storage_hal.shutdown();
    }
    
    static void test_storage_hal_factory_reset() {
        StorageHAL storage_hal;
        storage_hal.initialize();
        
        // 何かデータを書き込む
        const char* test_data = "factory_reset_test";
        storage_hal.writeConfig(test_data, strlen(test_data));
        
        // 工場出荷時リセット実行
        StorageResult reset_result = storage_hal.factoryReset();
        TEST_ASSERT_EQUAL(STORAGE_SUCCESS, reset_result);
        
        // 設定が無効になることを確認
        TEST_ASSERT_FALSE(storage_hal.isConfigValid());
        
        storage_hal.shutdown();
    }
    
    static void test_physical_reset_initialization() {
        PhysicalReset physical_reset;
        MockDisplayManager mock_display;
        MockConfigManager mock_config;
        
        // 初期化テスト
        TEST_ASSERT_TRUE(physical_reset.initialize(&mock_display, &mock_config));
        TEST_ASSERT_FALSE(physical_reset.isFactoryResetInProgress());
        TEST_ASSERT_FALSE(physical_reset.wasFactoryResetPerformed());
        
        physical_reset.shutdown();
    }
    
    static void test_simplified_config_manager() {
        ConfigManager config_manager;
        
        // 初期化テスト
        config_manager.init();
        TEST_ASSERT_TRUE(config_manager.isConfigValid());
        
        // 設定取得テスト
        const SystemConfig& config = config_manager.getConfig();
        TEST_ASSERT_TRUE(strlen(config.hostname) > 0);
        TEST_ASSERT_TRUE(config.config_version > 0);
        
        // 基本的な設定変更テスト
        TEST_ASSERT_TRUE(config_manager.setHostname("test-server"));
        TEST_ASSERT_TRUE(config_manager.setLogLevel(2)); // WARN level
        TEST_ASSERT_TRUE(config_manager.setGnssUpdateRate(5));
        
        // 無効な設定のテスト
        TEST_ASSERT_FALSE(config_manager.setHostname("")); // 空のホスト名
        TEST_ASSERT_FALSE(config_manager.setLogLevel(10)); // 無効なログレベル
        TEST_ASSERT_FALSE(config_manager.setGnssUpdateRate(0)); // 無効な更新レート
    }
    
    static void test_simplified_error_handler() {
        ErrorHandler error_handler;
        error_handler.init();
        
        // 基本的なエラー報告テスト
        error_handler.reportConfigurationError("TEST_CONFIG", "Test configuration error");
        TEST_ASSERT_TRUE(error_handler.hasUnresolvedErrors());
        
        // 統計情報テスト
        const ErrorStatistics& stats = error_handler.getStatistics();
        TEST_ASSERT_TRUE(stats.totalErrors > 0);
        
        // エラー解決テスト
        error_handler.resolveError("TEST_CONFIG", ErrorType::CONFIGURATION_ERROR);
        
        // 簡素化された復旧機能テスト
        error_handler.setAutoRecovery(true);
        error_handler.setMaxRetryCount(2);
        
        error_handler.reset();
    }
    
    static void test_integration_button_display() {
        // Button HALとDisplayManagerの統合テスト
        MockDisplayManager mock_display;
        MockConfigManager mock_config;
        
        PhysicalReset physical_reset;
        physical_reset.initialize(&mock_display, &mock_config);
        
        // 短押しシミュレーション（コールバック直接呼び出し）
        physical_reset.onShortPress(BUTTON_SHORT_PRESS);
        
        // DisplayManagerのnextDisplayMode()が呼ばれることを確認
        TEST_ASSERT_EQUAL(1, mock_display.next_mode_calls);
        TEST_ASSERT_EQUAL(1, mock_display.trigger_display_calls);
        
        physical_reset.shutdown();
    }
    
    static void test_integration_long_press_reset() {
        // 長押し工場出荷時リセットの統合テスト
        MockDisplayManager mock_display;
        MockConfigManager mock_config;
        
        PhysicalReset physical_reset;
        physical_reset.initialize(&mock_display, &mock_config);
        
        // 長押しシミュレーション
        physical_reset.onLongPress(BUTTON_LONG_PRESS);
        
        // リセットプロセスが開始されることを確認
        TEST_ASSERT_TRUE(physical_reset.isFactoryResetInProgress());
        TEST_ASSERT_TRUE(mock_display.error_message.indexOf("FACTORY RESET") >= 0);
        
        physical_reset.shutdown();
    }
    
    static void test_integration_storage_config() {
        // Storage HALとConfigManagerの統合テスト
        StorageHAL storage_hal;
        ConfigManager config_manager;
        
        storage_hal.initialize();
        config_manager.init();
        
        // 設定保存・読み込みテスト
        const char* test_hostname = "integration-test";
        TEST_ASSERT_TRUE(config_manager.setHostname(test_hostname));
        
        // 新しいConfigManagerインスタンスで読み込み確認
        ConfigManager config_manager2;
        config_manager2.init();
        const SystemConfig& loaded_config = config_manager2.getConfig();
        TEST_ASSERT_EQUAL_STRING(test_hostname, loaded_config.hostname);
        
        storage_hal.shutdown();
    }
};

// Unity テストランナー
void setUp(void) {
    // 各テスト前の初期化
}

void tearDown(void) {
    // 各テスト後のクリーンアップ
}

// テスト関数の登録
void test_button_hal_functions() {
    RUN_TEST(TestNewFeatures::test_button_hal_initialization);
    RUN_TEST(TestNewFeatures::test_button_hal_state_management);
}

void test_storage_hal_functions() {
    RUN_TEST(TestNewFeatures::test_storage_hal_initialization);
    RUN_TEST(TestNewFeatures::test_storage_hal_crc32_calculation);
    RUN_TEST(TestNewFeatures::test_storage_hal_read_write);
    RUN_TEST(TestNewFeatures::test_storage_hal_factory_reset);
}

void test_physical_reset_functions() {
    RUN_TEST(TestNewFeatures::test_physical_reset_initialization);
}

void test_simplified_systems() {
    RUN_TEST(TestNewFeatures::test_simplified_config_manager);
    RUN_TEST(TestNewFeatures::test_simplified_error_handler);
}

void test_integration_scenarios() {
    RUN_TEST(TestNewFeatures::test_integration_button_display);
    RUN_TEST(TestNewFeatures::test_integration_long_press_reset);
    RUN_TEST(TestNewFeatures::test_integration_storage_config);
}

// メイン関数
int main() {
    UNITY_BEGIN();
    
    // Button HAL テスト
    test_button_hal_functions();
    
    // Storage HAL テスト  
    test_storage_hal_functions();
    
    // Physical Reset テスト
    test_physical_reset_functions();
    
    // 簡素化されたシステムテスト
    test_simplified_systems();
    
    // 統合テスト
    test_integration_scenarios();
    
    return UNITY_END();
}

// Arduino環境用のsetup/loop関数
void setup() {
    delay(2000); // シリアル通信の安定化
    main();
}

void loop() {
    // テスト後は何もしない
}
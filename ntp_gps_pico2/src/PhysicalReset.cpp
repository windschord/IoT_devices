#include "PhysicalReset.h"
#include "DisplayManager.h"
#include "ConfigManager.h"
#include "LoggingService.h"

// グローバルインスタンス
PhysicalReset g_physical_reset;

// 静的メンバ初期化
PhysicalReset* PhysicalReset::instance = nullptr;

PhysicalReset::PhysicalReset() : 
    display_manager(nullptr),
    config_manager(nullptr),
    initialized(false),
    factory_reset_in_progress(false),
    factory_reset_performed(false),
    factory_reset_start_time(0),
    factory_reset_confirmation_time(0) {
    
    // 静的インスタンス参照を設定
    instance = this;
}

PhysicalReset::~PhysicalReset() {
    shutdown();
    instance = nullptr;
}

bool PhysicalReset::initialize(DisplayManager* displayMgr, ConfigManager* configMgr) {
    if (initialized) {
        return true;
    }
    
    if (!displayMgr || !configMgr) {
        LOG_ERR_MSG("RESET", "PhysicalReset: DisplayManager または ConfigManager が null");
        return false;
    }
    
    display_manager = displayMgr;
    config_manager = configMgr;
    
    // Button HAL初期化
    if (!g_button_hal.initialize()) {
        LOG_ERR_MSG("RESET", "PhysicalReset: Button HAL初期化失敗");
        return false;
    }
    
    // コールバック設定
    g_button_hal.setShortPressCallback(onShortPress);
    g_button_hal.setLongPressCallback(onLongPress);
    
    // 状態初期化
    factory_reset_in_progress = false;
    factory_reset_performed = false;
    factory_reset_start_time = 0;
    factory_reset_confirmation_time = 0;
    
    initialized = true;
    
    LOG_INFO_MSG("RESET", "PhysicalReset initialization completed");
    return true;
}

void PhysicalReset::shutdown() {
    if (!initialized) {
        return;
    }
    
    // Button HAL終了処理
    g_button_hal.shutdown();
    
    // 参照をクリア
    display_manager = nullptr;
    config_manager = nullptr;
    
    // 状態リセット
    factory_reset_in_progress = false;
    factory_reset_performed = false;
    
    initialized = false;
    
    LOG_INFO_MSG("RESET", "PhysicalReset: シャットダウン完了");
}

void PhysicalReset::update() {
    if (!initialized) {
        return;
    }
    
    // Button HAL更新
    g_button_hal.update();
    
    // 工場出荷時リセット進行中の処理
    if (factory_reset_in_progress) {
        uint32_t current_time = millis();
        uint32_t elapsed = current_time - factory_reset_start_time;
        
        if (elapsed < 3000) {
            // 最初の3秒間：確認画面表示
            displayFactoryResetConfirmation();
        } else if (elapsed < 8000) {
            // 次の5秒間：リセット進行表示
            displayFactoryResetProgress();
        } else {
            // 8秒経過：リセット実行
            performFactoryReset();
            factory_reset_in_progress = false;
            factory_reset_performed = true;
            
            // 完了画面表示
            displayFactoryResetComplete();
            
            LOG_INFO_MSG("RESET", "PhysicalReset: 工場出荷時リセット完了");
        }
    }
}

bool PhysicalReset::isFactoryResetInProgress() const {
    return factory_reset_in_progress;
}

bool PhysicalReset::wasFactoryResetPerformed() const {
    return factory_reset_performed;
}

void PhysicalReset::printStatus() const {
    LOG_INFO_MSG("RESET", "PhysicalReset Status:");
    LOG_INFO_F("RESET", "  Initialized: %s", initialized ? "Yes" : "No");
    LOG_INFO_F("RESET", "  Factory Reset In Progress: %s", factory_reset_in_progress ? "Yes" : "No");
    LOG_INFO_F("RESET", "  Factory Reset Performed: %s", factory_reset_performed ? "Yes" : "No");
    if (factory_reset_in_progress) {
        uint32_t elapsed = millis() - factory_reset_start_time;
        LOG_INFO_F("RESET", "  Reset Progress: %ums / 8000ms", elapsed);
    }
}

// 静的コールバック関数
void PhysicalReset::onShortPress(ButtonState state) {
    if (instance) {
        instance->handleShortPress();
    }
}

void PhysicalReset::onLongPress(ButtonState state) {
    if (instance) {
        instance->handleLongPress();
    }
}

// インスタンス固有処理
void PhysicalReset::handleShortPress() {
    if (!initialized || factory_reset_in_progress) {
        return;
    }
    
    LOG_INFO_MSG("RESET", "PhysicalReset: 短押し検出 - ディスプレイモード切り替え");
    
    // DisplayManagerのnextDisplayMode()を呼び出し
    if (display_manager) {
        display_manager->wakeDisplay(); // Explicitly wake display on button press
        display_manager->nextDisplayMode();
        display_manager->triggerDisplay();
    }
}

void PhysicalReset::handleLongPress() {
    if (!initialized || factory_reset_in_progress) {
        return;
    }
    
    LOG_WARN_MSG("RESET", "PhysicalReset: 長押し検出 - 工場出荷時リセット開始");
    
    // 工場出荷時リセットプロセス開始
    factory_reset_in_progress = true;
    factory_reset_start_time = millis();
    factory_reset_confirmation_time = 0;
    
    // 確認画面表示開始
    displayFactoryResetConfirmation();
}

void PhysicalReset::performFactoryReset() {
    LOG_WARN_MSG("RESET", "PhysicalReset: 工場出荷時リセット実行中...");
    
    // ConfigManager経由で設定をリセット
    if (config_manager) {
        config_manager->resetToDefaults();
        LOG_INFO_MSG("RESET", "PhysicalReset: 設定をデフォルトにリセット完了");
    }
    
    // その他のリセット処理がある場合はここに追加
    // 例：ログファイルクリア、統計リセットなど
    
    LOG_INFO_MSG("RESET", "PhysicalReset: 工場出荷時リセット実行完了");
}

void PhysicalReset::displayFactoryResetConfirmation() {
    if (!display_manager) {
        return;
    }
    
    // エラー状態として確認画面を表示
    display_manager->setErrorState("FACTORY RESET\nStarting in 3s...\nRelease button\nto cancel");
}

void PhysicalReset::displayFactoryResetProgress() {
    if (!display_manager) {
        return;
    }
    
    uint32_t elapsed = millis() - factory_reset_start_time;
    uint32_t progress_elapsed = elapsed - 3000; // 確認期間（3秒）を除いた経過時間
    uint32_t progress_total = 5000; // 進行期間（5秒）
    int progress_percent = (progress_elapsed * 100) / progress_total;
    
    char progress_msg[64];
    snprintf(progress_msg, sizeof(progress_msg), 
             "FACTORY RESET\nProgress: %d%%\nPlease wait...", 
             progress_percent);
    
    display_manager->setErrorState(progress_msg);
}

void PhysicalReset::displayFactoryResetComplete() {
    if (!display_manager) {
        return;
    }
    
    display_manager->setErrorState("FACTORY RESET\nCOMPLETE\n\nRestarting...");
    
    // 3秒後にシステム再起動
    delay(3000);
    
    LOG_INFO_MSG("RESET", "PhysicalReset: システム再起動実行");
    
    // Raspberry Pi Pico 2の再起動
    // 注意：この方法はハードウェア固有です
    rp2040.reboot();
}
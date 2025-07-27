#ifndef PHYSICAL_RESET_H
#define PHYSICAL_RESET_H

#include <Arduino.h>
#include "../hal/Button_HAL.h"

class DisplayManager;
class ConfigManager;

class PhysicalReset {
public:
    PhysicalReset();
    ~PhysicalReset();

    // 初期化と終了処理
    bool initialize(DisplayManager* displayMgr, ConfigManager* configMgr);
    void shutdown();

    // メイン処理
    void update();

    // 状態取得
    bool isFactoryResetInProgress() const;
    bool wasFactoryResetPerformed() const;

    // デバッグ
    void printStatus() const;

private:
    DisplayManager* display_manager;
    ConfigManager* config_manager;
    bool initialized;
    bool factory_reset_in_progress;
    bool factory_reset_performed;
    uint32_t factory_reset_start_time;
    uint32_t factory_reset_confirmation_time;

    // 静的定数
    static const uint32_t FACTORY_RESET_CONFIRMATION_TIMEOUT = 10000; // 10秒

    // ボタンコールバック関数（静的メンバ）
    static void onShortPress(ButtonState state);
    static void onLongPress(ButtonState state);

    // インスタンス固有の処理
    void handleShortPress();
    void handleLongPress();
    void performFactoryReset();
    void displayFactoryResetConfirmation();
    void displayFactoryResetProgress();
    void displayFactoryResetComplete();

    // 静的インスタンス参照（コールバック用）
    static PhysicalReset* instance;
};

extern PhysicalReset g_physical_reset;

#endif // PHYSICAL_RESET_H
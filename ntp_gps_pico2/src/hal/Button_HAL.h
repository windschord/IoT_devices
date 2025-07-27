#ifndef BUTTON_HAL_H
#define BUTTON_HAL_H

#include <Arduino.h>

enum ButtonState {
    BUTTON_IDLE,              // ボタンが押されていない
    BUTTON_PRESSED,           // ボタンが押されたばかり
    BUTTON_SHORT_PRESS,       // 短押し検出(<2秒)
    BUTTON_LONG_PRESS,        // 長押し検出(>5秒)
    BUTTON_DEBOUNCE          // デバウンス処理中
};

struct ButtonControl {
    ButtonState state;        // 現在のボタン状態
    uint32_t press_start;     // 押下開始時刻
    uint32_t last_read;       // 最後の読み取り時刻
    uint8_t debounce_count;   // デバウンスカウンタ
    bool long_press_triggered; // 長押しアクション実行済みフラグ
    uint32_t cooldown_until;  // クールダウン終了時刻
};

typedef void (*ButtonCallback)(ButtonState state);

class ButtonHAL {
public:
    static const uint8_t BUTTON_PIN = 11;           // GPIO 11
    static const uint32_t DEBOUNCE_DELAY = 20;      // 20ms
    static const uint32_t SHORT_PRESS_THRESHOLD = 100;  // 100ms (大幅短縮)
    static const uint32_t LONG_PRESS_THRESHOLD = 5000;  // 5秒
    static const uint32_t COOLDOWN_PERIOD = 300;    // 300ms (短縮)

    ButtonHAL();
    ~ButtonHAL();

    // 初期化と終了処理
    bool initialize();
    void shutdown();

    // メイン処理
    void update();

    // コールバック設定
    void setShortPressCallback(ButtonCallback callback);
    void setLongPressCallback(ButtonCallback callback);

    // 状態取得
    ButtonState getState() const;
    bool isPressed() const;
    uint32_t getPressedDuration() const;

    // デバッグ情報
    void printStatus() const;

private:
    ButtonControl control;
    ButtonCallback short_press_callback;
    ButtonCallback long_press_callback;
    bool initialized;

    // 内部処理
    bool readButton();
    void handleStateTransition();
    void resetState();
    bool isInCooldown() const;
    void triggerCallback(ButtonState state);
};

extern ButtonHAL g_button_hal;

#endif // BUTTON_HAL_H
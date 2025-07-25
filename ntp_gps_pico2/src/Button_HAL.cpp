#include "Button_HAL.h"
#include "LoggingService.h"

// グローバルインスタンス
ButtonHAL g_button_hal;

ButtonHAL::ButtonHAL() : 
    short_press_callback(nullptr),
    long_press_callback(nullptr),
    initialized(false) {
    
    // 制御構造体の初期化
    resetState();
}

ButtonHAL::~ButtonHAL() {
    shutdown();
}

bool ButtonHAL::initialize() {
    if (initialized) {
        return true;
    }

    // GPIO設定（内部プルアップ、アクティブLow）
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    // 状態初期化
    resetState();
    
    initialized = true;
    
    LOG_INFO_F("BUTTON", "ButtonHAL: 初期化完了 (GPIO %d)", BUTTON_PIN);
    return true;
}

void ButtonHAL::shutdown() {
    if (!initialized) {
        return;
    }
    
    // コールバックをクリア
    short_press_callback = nullptr;
    long_press_callback = nullptr;
    
    // 状態リセット
    resetState();
    
    initialized = false;
    
    LOG_INFO_MSG("BUTTON", "ButtonHAL: シャットダウン完了");
}

void ButtonHAL::update() {
    if (!initialized) {
        return;
    }

    uint32_t current_time = millis();
    
    // クールダウン期間中は処理をスキップ
    if (isInCooldown()) {
        return;
    }

    // デバウンス処理
    if (current_time - control.last_read < DEBOUNCE_DELAY) {
        return;
    }
    
    control.last_read = current_time;
    bool current_pressed = readButton();
    
    handleStateTransition();
    
    // 状態別処理
    switch (control.state) {
        case BUTTON_IDLE:
            if (current_pressed) {
                control.state = BUTTON_PRESSED;
                control.press_start = current_time;
                control.long_press_triggered = false;
                control.debounce_count = 0;
                
                LOG_DEBUG_MSG("BUTTON", "ButtonHAL: ボタン押下検出");
            }
            break;
            
        case BUTTON_PRESSED:
            if (!current_pressed) {
                // 短時間での離脱 - 短押しとして判定
                uint32_t duration = current_time - control.press_start;
                if (duration < SHORT_PRESS_THRESHOLD) {
                    control.state = BUTTON_SHORT_PRESS;
                    triggerCallback(BUTTON_SHORT_PRESS);
                    
                    LOG_INFO_F("BUTTON", "ButtonHAL: 短押し検出 (%ums)", duration);
                }
                resetState();
            } else {
                // 長押し判定
                uint32_t duration = current_time - control.press_start;
                if (duration >= LONG_PRESS_THRESHOLD && !control.long_press_triggered) {
                    control.state = BUTTON_LONG_PRESS;
                    control.long_press_triggered = true;
                    triggerCallback(BUTTON_LONG_PRESS);
                    
                    LOG_WARN_F("BUTTON", "ButtonHAL: 長押し検出 (%ums)", duration);
                }
            }
            break;
            
        case BUTTON_SHORT_PRESS:
        case BUTTON_LONG_PRESS:
            if (!current_pressed) {
                resetState();
            }
            break;
            
        case BUTTON_DEBOUNCE:
            control.debounce_count++;
            if (control.debounce_count >= 3) { // 3回安定したらデバウンス完了
                control.state = current_pressed ? BUTTON_PRESSED : BUTTON_IDLE;
                control.debounce_count = 0;
            }
            break;
    }
}

void ButtonHAL::setShortPressCallback(ButtonCallback callback) {
    short_press_callback = callback;
    LOG_DEBUG_MSG("BUTTON", "ButtonHAL: 短押しコールバック設定");
}

void ButtonHAL::setLongPressCallback(ButtonCallback callback) {
    long_press_callback = callback;
    LOG_DEBUG_MSG("BUTTON", "ButtonHAL: 長押しコールバック設定");
}

ButtonState ButtonHAL::getState() const {
    return control.state;
}

bool ButtonHAL::isPressed() const {
    return control.state == BUTTON_PRESSED || 
           control.state == BUTTON_SHORT_PRESS || 
           control.state == BUTTON_LONG_PRESS;
}

uint32_t ButtonHAL::getPressedDuration() const {
    if (control.state == BUTTON_IDLE) {
        return 0;
    }
    return millis() - control.press_start;
}

void ButtonHAL::printStatus() const {
    const char* state_names[] = {
        "IDLE", "PRESSED", "SHORT_PRESS", "LONG_PRESS", "DEBOUNCE"
    };
    
    LOG_INFO_MSG("BUTTON", "ButtonHAL Status:");
    LOG_INFO_F("BUTTON", "  State: %s", state_names[control.state]);
    LOG_INFO_F("BUTTON", "  Pressed Duration: %ums", getPressedDuration());
    LOG_INFO_F("BUTTON", "  Debounce Count: %d", control.debounce_count);
    LOG_INFO_F("BUTTON", "  Long Press Triggered: %s", control.long_press_triggered ? "Yes" : "No");
    LOG_INFO_F("BUTTON", "  Cooldown: %s", isInCooldown() ? "Active" : "Inactive");
}

bool ButtonHAL::readButton() {
    // アクティブLow（押下時LOW）
    return digitalRead(BUTTON_PIN) == LOW;
}

void ButtonHAL::handleStateTransition() {
    // 現在の実装では追加の状態遷移処理は不要
    // 将来的に複雑な状態遷移が必要になった場合に使用
}

void ButtonHAL::resetState() {
    control.state = BUTTON_IDLE;
    control.press_start = 0;
    control.last_read = 0;
    control.debounce_count = 0;
    control.long_press_triggered = false;
    control.cooldown_until = millis() + COOLDOWN_PERIOD;
}

bool ButtonHAL::isInCooldown() const {
    return millis() < control.cooldown_until;
}

void ButtonHAL::triggerCallback(ButtonState state) {
    switch (state) {
        case BUTTON_SHORT_PRESS:
            if (short_press_callback) {
                short_press_callback(state);
            }
            break;
            
        case BUTTON_LONG_PRESS:
            if (long_press_callback) {
                long_press_callback(state);
            }
            break;
            
        default:
            break;
    }
}
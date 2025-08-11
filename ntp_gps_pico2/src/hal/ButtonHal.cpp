#include "ButtonHal.h"
#include "../config/LoggingService.h"

// グローバルインスタンス
ButtonHAL g_button_hal;

ButtonHAL::ButtonHAL() : 
    short_press_callback(nullptr),
    long_press_callback(nullptr),
    initialized(false),
    lastError(nullptr) {
    
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
    
    LOG_INFO_F("BUTTON", "ButtonHAL initialization completed (GPIO %d)", BUTTON_PIN);
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
    
    LOG_INFO_MSG("BUTTON", "ButtonHAL shutdown completed");
}

void ButtonHAL::update() {
    if (!initialized) {
        return;
    }

    uint32_t current_time = millis();
    
    // 定期的なデバッグ出力（5秒ごと）
    static uint32_t last_debug = 0;
    static uint32_t update_call_count = 0;
    update_call_count++;
    
    if (current_time - last_debug > 5000) {
        bool raw_state = digitalRead(BUTTON_PIN);
        LOG_DEBUG_F("BUTTON", "GPIO %d = %s (RAW), cooldown=%s, state=%d, update_calls=%d", 
                    BUTTON_PIN, raw_state ? "HIGH" : "LOW", 
                    isInCooldown() ? "YES" : "NO", control.state, update_call_count);
        last_debug = current_time;
        update_call_count = 0; // Reset counter
    }
    
    // クールダウン期間中は処理をスキップ
    if (isInCooldown()) {
        static uint32_t last_cooldown_debug = 0;
        if (current_time - last_cooldown_debug > 1000) {
            uint32_t remaining = control.cooldown_until - current_time;
            LOG_DEBUG_F("BUTTON", "Still in cooldown, %ums remaining", remaining);
            last_cooldown_debug = current_time;
        }
        return;
    }

    // デバウンス処理
    if (current_time - control.last_read < DEBOUNCE_DELAY) {
        return;
    }
    
    control.last_read = current_time;
    bool current_pressed = readButton();
    
    // ボタン状態変化のデバッグ
    static bool last_pressed_debug = false;
    static uint32_t last_state_change = 0;
    if (current_pressed != last_pressed_debug) {
        uint32_t time_since_last = current_time - last_state_change;
        LOG_DEBUG_F("BUTTON", "Button state changed from %s to %s (after %ums)", 
                    last_pressed_debug ? "PRESSED" : "RELEASED",
                    current_pressed ? "PRESSED" : "RELEASED", time_since_last);
        last_pressed_debug = current_pressed;
        last_state_change = current_time;
    }
    
    handleStateTransition();
    
    // 状態別処理
    switch (control.state) {
        case BUTTON_IDLE:
            if (current_pressed) {
                control.state = BUTTON_PRESSED;
                control.press_start = current_time;
                control.long_press_triggered = false;
                control.debounce_count = 0;
                
                LOG_DEBUG_MSG("BUTTON", "Button press detected");
            }
            break;
            
        case BUTTON_PRESSED:
            if (!current_pressed) {
                // ボタンが離された - 短押しとして判定
                uint32_t duration = current_time - control.press_start;
                if (duration >= SHORT_PRESS_THRESHOLD) {
                    control.state = BUTTON_SHORT_PRESS;
                    triggerCallback(BUTTON_SHORT_PRESS);
                    
                    LOG_INFO_F("BUTTON", "Short press detected (%ums)", duration);
                } else {
                    LOG_DEBUG_F("BUTTON", "Press duration too short (%ums < %ums)", duration, SHORT_PRESS_THRESHOLD);
                }
                resetState();
            } else {
                // 長押し判定
                uint32_t duration = current_time - control.press_start;
                if (duration >= LONG_PRESS_THRESHOLD && !control.long_press_triggered) {
                    control.state = BUTTON_LONG_PRESS;
                    control.long_press_triggered = true;
                    triggerCallback(BUTTON_LONG_PRESS);
                    
                    LOG_WARN_F("BUTTON", "Long press detected (%ums)", duration);
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
    LOG_DEBUG_MSG("BUTTON", "Short press callback configured");
}

void ButtonHAL::setLongPressCallback(ButtonCallback callback) {
    long_press_callback = callback;
    LOG_DEBUG_MSG("BUTTON", "Long press callback configured");
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

bool ButtonHAL::reset() {
    lastError = nullptr;
    
    if (initialized) {
        shutdown();
    }
    
    bool result = initialize();
    if (!result) {
        lastError = "Button reset failed";
    }
    
    return result;
}
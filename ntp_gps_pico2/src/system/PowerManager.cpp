#include "PowerManager.h"
#include "../config/LoggingService.h"
#include "../hal/HardwareConfig.h"
#include <pico/stdlib.h>
#include <hardware/adc.h>
#include <hardware/watchdog.h>

PowerManager::PowerManager() : loggingService(nullptr), currentPowerState(POWER_NORMAL) {
    // 電圧監視初期化
    voltageMonitor = {
        .currentVoltage = 0.0f,
        .minVoltage = 4.5f,      // 5V供給の90%
        .maxVoltage = 5.5f,      // 5V供給の110%
        .warningThreshold = 4.7f, // 94%
        .criticalThreshold = 4.3f, // 86%
        .lastCheck = 0,
        .checkInterval = 30,      // 30秒間隔
        .voltageStable = true
    };
    
    // ウォッチドッグ初期化
    watchdog = {
        .enabled = false,
        .timeout = 8000,          // 8秒デフォルト
        .lastFeed = 0,
        .feedInterval = 4000,     // 4秒間隔でフィード
        .maxMissedFeeds = 2,
        .missedFeedCount = 0
    };
    
    // システム安定性監視初期化
    stability = {
        .uptimeSeconds = 0,
        .rebootCount = 0,
        .lastReboot = 0,
        .cpuTemperature = 0.0f,
        .thermalThrottling = false,
        .freeHeapMemory = 0,
        .minFreeHeap = 0xFFFFFFFF
    };
}

void PowerManager::init() {
    if (loggingService) {
        loggingService->info("POWER", "Initializing Power Management System...");
    }
    
    initializeVoltageMonitor();
    initializeWatchdog();
    initializeStabilityMonitor();
    
    if (loggingService) {
        loggingService->info("POWER", "Power Management System initialized successfully");
    }
}

void PowerManager::initializeVoltageMonitor() {
    // ADC初期化（内部電圧リファレンス用）
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4); // 内部温度センサー
    
    if (loggingService) {
        loggingService->info("POWER", "Voltage monitoring initialized");
    }
}

void PowerManager::initializeWatchdog() {
    // Pico2ウォッチドッグの初期化準備
    // 実際の有効化は enableWatchdog() で行う
    
    if (loggingService) {
        loggingService->info("POWER", "Watchdog system prepared (not enabled by default)");
    }
}

void PowerManager::initializeStabilityMonitor() {
    // 初回メトリクス取得
    stability.freeHeapMemory = getFreeHeapMemory();
    stability.minFreeHeap = stability.freeHeapMemory;
    stability.cpuTemperature = getCpuTemperature();
    stability.uptimeSeconds = millis() / 1000;
    
    if (loggingService) {
        loggingService->infof("POWER", "System metrics initialized - Free heap: %lu bytes, CPU temp: %.1f°C", 
                            stability.freeHeapMemory, stability.cpuTemperature);
    }
}

void PowerManager::update() {
    unsigned long now = millis();
    
    // 定期的な電圧チェック
    if (now - voltageMonitor.lastCheck > (voltageMonitor.checkInterval * 1000)) {
        checkVoltage();
    }
    
    // ウォッチドッグフィード
    if (watchdog.enabled) {
        if (now - watchdog.lastFeed > watchdog.feedInterval) {
            feedWatchdog();
        }
        checkWatchdogTimeout();
    }
    
    // システムメトリクス更新（低優先度・1分間隔）
    static unsigned long lastMetricsUpdate = 0;
    if (now - lastMetricsUpdate > 60000) { // 1分間隔
        updateSystemMetrics();
        lastMetricsUpdate = now;
    }
}

void PowerManager::checkVoltage() {
    voltageMonitor.lastCheck = millis();
    voltageMonitor.currentVoltage = readInternalVoltage();
    
    PowerState previousState = currentPowerState;
    updatePowerState();
    
    // 電圧安定性評価
    voltageMonitor.voltageStable = (voltageMonitor.currentVoltage >= voltageMonitor.warningThreshold);
    
    // ステート変化時のログ
    if (previousState != currentPowerState) {
        const char* stateNames[] = {"NORMAL", "WARNING", "CRITICAL", "EMERGENCY"};
        if (loggingService) {
            loggingService->warningf("POWER", "Power state changed from %s to %s - Voltage: %.2fV", 
                                   stateNames[previousState], stateNames[currentPowerState], 
                                   voltageMonitor.currentVoltage);
        }
        
        // ステート別対応
        switch (currentPowerState) {
            case POWER_WARNING:
                handleVoltageWarning();
                break;
            case POWER_CRITICAL:
                handleVoltageCritical();
                break;
            case POWER_EMERGENCY:
                handlePowerEmergency();
                break;
            default:
                break;
        }
    }
}

void PowerManager::updatePowerState() {
    float voltage = voltageMonitor.currentVoltage;
    
    if (voltage < voltageMonitor.criticalThreshold) {
        currentPowerState = POWER_EMERGENCY;
    } else if (voltage < voltageMonitor.warningThreshold) {
        if (voltage < (voltageMonitor.criticalThreshold + 0.1f)) {
            currentPowerState = POWER_CRITICAL;
        } else {
            currentPowerState = POWER_WARNING;
        }
    } else {
        currentPowerState = POWER_NORMAL;
    }
}

void PowerManager::handleVoltageWarning() {
    if (loggingService) {
        loggingService->warning("POWER", "Low voltage detected - reducing power consumption");
    }
    
    // 非必須機能の電力削減
    // 例：OLED表示を暗くする、WiFi スキャン頻度を下げる等
}

void PowerManager::handleVoltageCritical() {
    if (loggingService) {
        loggingService->error("POWER", "Critical voltage level - preparing for emergency shutdown");
    }
    
    // 重要データの保存準備
    // 非必須サービスの停止
}

void PowerManager::handlePowerEmergency() {
    if (loggingService) {
        loggingService->error("POWER", "EMERGENCY: Voltage too low - initiating controlled shutdown");
    }
    
    // 緊急シャットダウン処理
    // 1. 重要な設定を保存
    // 2. ネットワーク接続を適切に閉じる
    // 3. 再起動またはシャットダウン
    performControlledReboot("Emergency low voltage");
}

void PowerManager::enableWatchdog(unsigned long timeoutMs) {
    if (timeoutMs < 1000 || timeoutMs > 8388) { // Pico2の制限
        if (loggingService) {
            loggingService->error("POWER", "Invalid watchdog timeout - must be 1-8388ms");
        }
        return;
    }
    
    watchdog.timeout = timeoutMs;
    watchdog.enabled = true;
    watchdog.lastFeed = millis();
    watchdog.missedFeedCount = 0;
    
    // Pico2ウォッチドッグを有効化
    watchdog_enable(timeoutMs, true);
    
    if (loggingService) {
        loggingService->infof("POWER", "Watchdog enabled with %lu ms timeout", timeoutMs);
    }
}

void PowerManager::disableWatchdog() {
    if (!watchdog.enabled) return;
    
    watchdog.enabled = false;
    
    if (loggingService) {
        loggingService->info("POWER", "Watchdog disabled");
    }
}

void PowerManager::feedWatchdog() {
    if (!watchdog.enabled) return;
    
    watchdog_update();
    watchdog.lastFeed = millis();
    watchdog.missedFeedCount = 0;
    
    // デバッグレベルでのみログ出力（頻度が高いため）
    if (loggingService) {
        loggingService->debug("POWER", "Watchdog fed");
    }
}

void PowerManager::checkWatchdogTimeout() {
    if (!watchdog.enabled) return;
    
    unsigned long now = millis();
    if (now - watchdog.lastFeed > watchdog.feedInterval) {
        watchdog.missedFeedCount++;
        
        if (watchdog.missedFeedCount >= watchdog.maxMissedFeeds) {
            if (loggingService) {
                loggingService->error("POWER", "Watchdog feed timeout - system may be unresponsive");
            }
            // 自動的なウォッチドッグリセットに委ねる
        }
    }
}

void PowerManager::updateSystemMetrics() {
    stability.uptimeSeconds = millis() / 1000;
    stability.cpuTemperature = getCpuTemperature();
    stability.freeHeapMemory = getFreeHeapMemory();
    
    // 最小フリーヒープ記録
    if (stability.freeHeapMemory < stability.minFreeHeap) {
        stability.minFreeHeap = stability.freeHeapMemory;
    }
    
    // 熱スロットリング検出
    if (stability.cpuTemperature > 70.0f) { // Pico2の推奨動作温度上限
        stability.thermalThrottling = true;
        if (loggingService) {
            loggingService->warningf("POWER", "High CPU temperature detected: %.1f°C", stability.cpuTemperature);
        }
    } else {
        stability.thermalThrottling = false;
    }
    
    // メモリ不足警告
    if (stability.freeHeapMemory < 10240) { // 10KB未満
        if (loggingService) {
            loggingService->warningf("POWER", "Low memory warning - Free heap: %lu bytes", stability.freeHeapMemory);
        }
    }
}

float PowerManager::getCpuTemperature() {
    return readCpuTemperatureInternal();
}

unsigned long PowerManager::getFreeHeapMemory() {
    // Pico2のフリーヒープ取得（概算）
    extern char __HeapLimit, __StackLimit;
    return &__StackLimit - &__HeapLimit;
}

void PowerManager::performControlledReboot(const char* reason) {
    if (loggingService) {
        loggingService->errorf("POWER", "Performing controlled reboot - Reason: %s", reason);
    }
    
    // 重要な状態を保存
    stability.rebootCount++;
    stability.lastReboot = millis();
    
    // 少し待ってから再起動
    delay(1000);
    
    // ウォッチドッグを使った再起動
    watchdog_enable(100, true);
    while(1) {
        // ウォッチドッグタイムアウトを待つ
    }
}

// プライベート関数実装

float PowerManager::readInternalVoltage() {
    // Pico2の内部ADCを使用した電圧測定の概算実装
    // 実際の実装では、外部電圧分割回路が必要
    
    adc_select_input(3); // VSYS測定用
    uint16_t raw = adc_read();
    
    // 概算計算（実際の回路に応じて調整が必要）
    float voltage = raw * 3.3f * 3.0f / 4096.0f; // ADC基準電圧 * 分圧比 / ADC分解能
    
    return voltage;
}

float PowerManager::readCpuTemperatureInternal() {
    // Pico2内蔵温度センサーから温度を読取
    adc_select_input(4); // 温度センサー
    uint16_t raw = adc_read();
    
    // ADC値から温度への変換（Pico2データシート参照）
    const float conversionFactor = 3.3f / 4096.0f;
    float voltage = raw * conversionFactor;
    float temperature = 27.0f - (voltage - 0.706f) / 0.001721f;
    
    return temperature;
}
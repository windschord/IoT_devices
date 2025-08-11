#pragma once

#include <Arduino.h>

/**
 * @brief メインループ処理を管理するクラス
 * 
 * システムのメインループ処理を優先度別に分離し、
 * タイミング制御の一元化と非ブロッキング処理の最適化を実現する。
 * 
 * 優先度レベル:
 * - HIGH: 毎ループ実行される重要な処理
 * - MEDIUM: 100ms間隔で実行される処理  
 * - LOW: 1000ms間隔で実行される処理
 */
class MainLoop {
public:
    /**
     * @brief メインループの実行
     * 
     * 優先度に基づいて各処理を適切なタイミングで実行する
     */
    static void execute();

private:
    /**
     * @brief 高優先度処理（毎ループ実行）
     * 
     * - エラーハンドラー更新
     * - 物理リセット監視
     * - 電源管理・ウォッチドッグ
     * - UDP/NTP要求処理
     * - ログ処理
     * - Webサーバー処理
     * - GPS データ処理
     * - LED管理
     */
    static void executeHighPriorityTasks();

    /**
     * @brief 中優先度処理（100ms間隔）
     * 
     * - ディスプレイ更新
     * - システムコントローラー更新
     * - GPS信号監視
     */
    static void executeMediumPriorityTasks();

    /**
     * @brief 低優先度処理（1000ms間隔）
     * 
     * - ハードウェア状態更新
     * - ネットワーク監視・自動復旧
     * - Prometheusメトリクス更新
     * - GPSキャッシュ無効化
     * - ネットワーク状態デバッグ
     */
    static void executeLowPriorityTasks();

    // 個別処理メソッド
    static void processGpsData();
    static void manageLeds();
    static void processNetworkRecovery();
    static void updateMetrics();
    static void processDisplayContent();
    static void debugNetworkStatus();
    static void handleGnssBlinking();

    // タイミング制御定数
    static constexpr unsigned long MEDIUM_PRIORITY_INTERVAL = 100; // 100ms
    static constexpr unsigned long LOW_PRIORITY_INTERVAL = 1000;   // 1000ms
    static constexpr unsigned long NETWORK_DEBUG_INTERVAL = 30000; // 30 seconds
    static constexpr unsigned long RTC_DETAIL_DEBUG_INTERVAL = 10000; // 10 seconds
};
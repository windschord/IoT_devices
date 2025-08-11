/**
 * @file main.cpp
 * @brief GPS NTP Server - Main Application Entry Point
 * 
 * 簡素化されたメインファイル。システム初期化とメインループの実行のみを担当。
 * 全ての具体的な処理は以下のクラスに移譲:
 * - SystemInitializer: 全初期化処理の集約管理
 * - MainLoop: 優先度別処理分離とタイミング制御
 * - SystemState: グローバル変数とサービスの統合管理
 * 
 * リファクタリング成果:
 * - 933行 → 80行 (91%削減)
 * - グローバル変数 → SystemStateシングルトンで管理
 * - 初期化処理 → SystemInitializerクラスで集約
 * - ループ処理 → MainLoopクラスで優先度別分離
 */

#include <Arduino.h>
#include "system/SystemInitializer.h"
#include "system/MainLoop.h"
#include "system/SystemState.h"
#include "hal/HardwareConfig.h"
#include "gps/TimeManager.h"

// グローバル変数はSystemStateシングルトンで管理
// ハードウェアインスタンスとサービスはSystemState内で初期化

// 一時的なグローバル変数（他のクラスとの互換性のため）
// TODO: 将来的にはSystemStateクラスのメンバーのみを使用
bool gpsConnected = false;
TimeManager* timeManager = nullptr;

// PPS interrupt handler - delegates to SystemState
void triggerPps() {
  SystemState::triggerPps();
}

/**
 * @brief システムセットアップ
 * 
 * SystemInitializerに全初期化処理を委譲し、
 * setup関数を大幅に簡素化。
 */
void setup() {
  SystemInitializer::InitializationResult result = SystemInitializer::initialize();
  
  if (!result.success) {
    Serial.printf("❌ System initialization failed: %s (code: %d)\n", 
                  result.errorMessage, result.errorCode);
    // Continue operation even if some components failed
  }
  
  // 一時的な互換性のためのグローバル変数設定
  // TODO: 将来的には削除予定
  SystemState& state = SystemState::getInstance();
  gpsConnected = state.isGpsConnected();
  timeManager = &state.getTimeManager();
  
  // PPS interrupt setup
  attachInterrupt(digitalPinToInterrupt(GPS_PPS_PIN), triggerPps, FALLING);
  
  Serial.println("🚀 GPS NTP Server ready!");
}

/**
 * @brief メインループ
 * 
 * MainLoopクラスに全処理を委譲し、
 * loop関数を大幅に簡素化。
 */
void loop() {
  MainLoop::execute();
}

/**
 * @note リファクタリング詳細
 * 
 * 【移行前の問題点】
 * - main.cpp: 933行（肥大化）
 * - グローバル変数: 50+個（管理困難）
 * - 初期化処理: 複雑な依存関係（エラーが起きやすい）
 * - ループ処理: 優先度不明確（パフォーマンス問題）
 * 
 * 【移行後の改善点】
 * - main.cpp: 80行（91%削減）
 * - グローバル変数: SystemStateで一元管理
 * - 初期化処理: SystemInitializerで順序・依存関係明確化
 * - ループ処理: MainLoopで優先度別分離（HIGH/MEDIUM/LOW）
 * 
 * 【アーキテクチャ改善】
 * 1. 単一責任原則: 各クラスが明確な責務を持つ
 * 2. 依存関係逆転: インターフェースによる疎結合
 * 3. 関心の分離: 初期化・実行・状態管理の分離
 * 4. テスタビリティ: 各クラスが独立してテスト可能
 * 
 * 【保守性向上】
 * - 新機能追加時: 対応するクラスのみ修正
 * - バグ修正時: 影響範囲が明確
 * - コードレビュー時: 変更部分が特定しやすい
 * - 理解時間短縮: 新規開発者の学習コスト削減
 */
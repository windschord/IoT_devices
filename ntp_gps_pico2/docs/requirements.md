# Requirements Document

## Introduction

このプロジェクトは、Raspberry Pi Pico 2とGPS受信機（SparkFun GNSS Timing Breakout ZED-F9T）を使用して、高精度なNTPサーバーを構築することを目的としています。GPSのPPS（Pulse Per Second）信号を利用して正確な時刻同期を実現し、ネットワーク経由でNTPクライアントに時刻情報を提供します。また、W5500イーサネットモジュールでネットワーク接続を行い、SH1106 OLEDディスプレイでステータス表示を行います。

## Requirements

### Requirement 1

**User Story:** システム管理者として、GPS衛星から正確な時刻情報を取得したいので、ネットワーク内のデバイスが高精度な時刻同期を行えるようになる

#### Acceptance Criteria

1. WHEN GPS受信機がGPS衛星からの信号を受信 THEN システム SHALL GPS時刻情報を取得する
2. WHEN GPS受信機がPPS信号を出力 THEN システム SHALL PPS信号をRaspberry Pi Pico 2で検出する
3. WHEN GPS信号が利用可能 THEN システム SHALL GPS時刻とPPS信号を同期させる
4. IF GPS信号が30秒以上受信できない THEN システム SHALL 内部クロックにフォールバックする

### Requirement 2

**User Story:** ネットワーク管理者として、NTPプロトコル経由で時刻同期サービスを提供したいので、クライアントデバイスが正確な時刻を取得できるようになる

#### Acceptance Criteria

1. WHEN NTPクライアントがNTP要求を送信 THEN システム SHALL NTPプロトコルに準拠した応答を返す
2. WHEN システムがGPS時刻と同期している THEN NTP応答 SHALL stratum 1レベルの精度を示す
3. WHEN システムがGPS信号を失っている THEN NTP応答 SHALL stratum 2以上のレベルを示す
4. WHEN 複数のNTPクライアントが同時に要求 THEN システム SHALL 各要求に適切に応答する

### Requirement 3

**User Story:** システム運用者として、デバイスの動作状況を視覚的に確認したいので、現在の状態を一目で把握できるようになる

#### Acceptance Criteria

1. WHEN システムが起動 THEN OLEDディスプレイ SHALL 起動メッセージを表示する
2. WHEN GPS信号を受信中 THEN ディスプレイ SHALL GPS受信状態（衛星数、信号強度）を表示する
3. WHEN NTPサーバーが動作中 THEN ディスプレイ SHALL 現在時刻とNTP統計情報を表示する
4. WHEN エラーが発生 THEN ディスプレイ SHALL エラー状態とメッセージを表示する

### Requirement 4

**User Story:** ネットワーク管理者として、イーサネット経由でNTPサービスにアクセスしたいので、Wi-Fiに依存しない安定した接続を確保できる

#### Acceptance Criteria

1. WHEN システムが起動 THEN W5500モジュール SHALL イーサネット接続を確立する
2. WHEN ネットワーク設定が完了 THEN システム SHALL 固定IPアドレスまたはDHCPでIPを取得する
3. WHEN イーサネット接続が確立 THEN システム SHALL NTPポート（123）でリッスンを開始する
4. IF イーサネット接続が切断 THEN システム SHALL 再接続を試行する

### Requirement 5

**User Story:** システム管理者として、デバイスの設定を変更したいので、設定ファイルやWebインターフェース経由で設定を調整できるようになる

#### Acceptance Criteria

1. WHEN システムが起動 THEN システム SHALL 設定ファイルから初期設定を読み込む
2. WHEN 設定変更要求を受信 THEN システム SHALL 新しい設定を検証して適用する
3. WHEN 無効な設定が検出 THEN システム SHALL デフォルト設定にフォールバックする
4. WHEN 設定が変更 THEN システム SHALL 変更内容を不揮発性メモリに保存する

### Requirement 6

**User Story:** システム運用者として、システムの精度と性能を監視したいので、時刻同期の品質とNTPサービスの統計情報を確認できるようになる

#### Acceptance Criteria

1. WHEN システムが動作中 THEN システム SHALL GPS時刻とシステム時刻の差分を記録する
2. WHEN NTP要求を処理 THEN システム SHALL 応答時間と要求数を記録する
3. WHEN Prometheusから統計情報が要求 THEN システム SHALL メトリクス形式で精度統計とサービス統計を提供する
4. WHEN 異常な精度低下を検出 THEN システム SHALL アラート状態を表示する

### Requirement 7

**User Story:** システム管理者として、ログ情報を中央集約したいので、システムログをSyslogサーバーに転送して一元管理できるようになる

#### Acceptance Criteria

1. WHEN システムイベントが発生 THEN システム SHALL 適切なログレベルでログを生成する
2. WHEN ログが生成 THEN システム SHALL 設定されたSyslogサーバーにログを転送する
3. WHEN Syslogサーバーが利用不可 THEN システム SHALL ローカルログバッファに一時保存する
4. IF Syslogサーバーが復旧 THEN システム SHALL バッファされたログを送信する
# GPS NTP Server テストスイート

このディレクトリには、GPS NTPサーバーのRFC 5905準拠性と時刻同期の正確性を検証するテストスイートが含まれています。

## テストファイル

### test_ntp_rfc5905.cpp
RFC 5905 (NTPv4) 準拠性の総合テスト
- NTPパケット構造とフィールドの検証
- タイムスタンプ形式とバイトオーダーの確認
- Stratum、Precision、Reference IDの妥当性チェック
- エッジケースとエラーケースの処理

### test_time_synchronization.cpp  
時刻同期問題の詳細検証テスト
- GPS時刻→Unixタイムスタンプ変換の精度テスト
- Unix→NTPタイムスタンプ変換の検証
- TimeManagerの高精度時刻取得機能テスト
- 55年オフセット問題の再現と分析

## テスト実行方法

### 1. 基本テスト実行
```bash
# すべてのテストを実行
pio test -e test

# 特定のテストファイルのみ実行
pio test -e test --filter test_ntp_rfc5905
pio test -e test --filter test_time_synchronization
```

### 2. 詳細デバッグ出力付きテスト
```bash
# デバッグ出力を有効にしてテスト実行
pio test -e test -v
```

### 3. シリアルモニター経由でのテスト結果確認
```bash
# テスト実行後、シリアル出力を確認
pio device monitor -e test -b 9600
```

## 検証項目

### RFC 5905準拠性
- ✅ NTPパケットサイズ: 48バイト
- ✅ NTPv4 ヘッダーフィールド
- ✅ Leap Indicator、Version、Mode フィールド
- ✅ Stratum レベル (GPS: 1, RTC: 3+)
- ✅ Reference Identifier ("GPS\0", "RTC\0")
- ✅ Precision 値 (GPS: 2^-20, RTC: 2^-10)
- ✅ タイムスタンプのネットワークバイトオーダー

### 時刻変換精度
- ✅ GPS UTC時刻 → Unixタイムスタンプ変換
- ✅ Unix → NTPタイムスタンプ変換 (+2208988800秒)
- ✅ マイクロ秒精度のフラクション部計算
- ✅ エンディアン変換の正確性
- ✅ うるう年の処理

### 問題再現・分析
- ❌ 55年オフセット問題の根本原因特定
- ❌ TimeManager::getHighPrecisionTime() の異常値問題
- ❌ GPS同期データの正しい保存・取得

## 期待されるテスト結果

### 正常ケース
```
GPS Time Debug - timeSync->gpsTime: 1753179057, elapsed: 12345, result: 1753179057123
NTP Timestamp Debug - Unix: 1753179057, NTP: 3962167857 (0xEC29E271), Expected: 3962167857
```

### 現在の問題ケース  
```
GPS Time Debug - timeSync->gpsTime: 832400, elapsed: 12345, result: 832412
NTP Timestamp Debug - Unix: 832400, NTP: 2209821200 (0x83B73210), Expected: 3962167857
```

## トラブルシューティング

### テストが失敗する場合
1. **time_t型のサイズ確認**: 32bit vs 64bit
2. **タイムゾーン設定**: UTCでの動作確認
3. **メモリ制約**: テスト用データ構造のサイズ確認

### 時刻変換エラーの場合
1. **gpsTimeToUnixTimestamp()関数の動作確認**
2. **mktime()とUTC計算の差分確認**
3. **TimeSync構造体の初期化状態確認**

### NTPタイムスタンプ異常の場合
1. **NTP_TIMESTAMP_DELTA (2208988800) の確認**
2. **htonl/ntohl変換の動作確認**
3. **フラクション部の計算精度確認**

## 参考資料

- [RFC 5905 - Network Time Protocol Version 4](https://tools.ietf.org/html/rfc5905)
- [NTP Timestamp Format](https://tools.ietf.org/html/rfc5905#section-6)
- [GPS Time vs UTC](https://www.nist.gov/pml/time-and-frequency-division/time-realization/coordinated-universal-time-utc)
- [Unity Test Framework](https://github.com/ThrowTheSwitch/Unity)

## バグレポート

テストで問題が発見された場合は、以下の情報と共に報告してください：

1. テスト実行環境 (PlatformIO版、ボード種類)
2. 失敗したテストケース名
3. 期待値と実際の値
4. シリアル出力ログ
5. GPS受信状況 (衛星数、信号強度)

現在の主要問題：
- **GPS時刻同期後のTimeManager::getHighPrecisionTime()が異常に小さい値を返す**
- **結果としてNTPクライアントに55年古い時刻が送信される**
- **根本原因：timeSync->gpsTimeの設定または取得に問題がある可能性**
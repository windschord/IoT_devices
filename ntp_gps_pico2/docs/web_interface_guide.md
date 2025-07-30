# Web設定インターフェース操作ガイド

## 概要

GPS NTP ServerのWeb設定インターフェースは、ブラウザから簡単にシステムを設定・監視できる包括的な管理画面です。直感的なタブ形式で各機能が整理されており、リアルタイムでシステム状態を確認できます。

## アクセス方法

### 1. IPアドレスの確認

**方法1: OLEDディスプレイ**
1. デバイスのリセットボタンを短押し（<2秒）
2. ディスプレイモードを「System」に切り替え
3. IPアドレスが表示される

**方法2: ルーター管理画面**
1. ルーターの管理画面にアクセス
2. 接続機器リストで「gps-ntp-server」を検索
3. 割り当てられたIPアドレスを確認

**方法3: ネットワークスキャン**
```bash
# Linux/macOS
nmap -sn 192.168.1.0/24 | grep -A1 "gps-ntp-server"

# Windows
arp -a | findstr "gps-ntp-server"
```

### 2. ブラウザアクセス

1. Webブラウザを開く（Chrome、Firefox、Safari、Edge対応）
2. アドレスバーに`http://[IPアドレス]`を入力
3. Enter キーを押してアクセス

**例**: `http://192.168.1.100`

## インターフェース構成

### メイン画面レイアウト

```
┌─────────────────────────────────────────┐
│ GPS NTP Server Configuration           │
├─────────────────────────────────────────┤
│ [Network] [GNSS] [NTP] [System] [Status]│ ← タブナビゲーション
├─────────────────────────────────────────┤
│                                         │
│          設定コンテンツエリア              │
│                                         │
├─────────────────────────────────────────┤
│ [Save Settings] [Reset to Defaults]    │ ← アクションボタン
├─────────────────────────────────────────┤
│ Status: Settings saved successfully ✓   │ ← メッセージエリア
└─────────────────────────────────────────┘
```

### ナビゲーション操作

- **タブクリック**: 設定カテゴリの切り替え
- **フォーム入力**: 各設定項目の変更
- **保存ボタン**: 変更内容の適用
- **リセットボタン**: カテゴリ別デフォルト復帰
- **自動更新**: Status画面は5秒間隔で更新

## タブ別操作ガイド

### Network タブ

ネットワーク関連の設定を管理します。

#### Hostname（ホスト名）設定

**目的**: ネットワーク上でのデバイス識別名を設定

**操作手順**:
1. Networkタブをクリック
2. 「Hostname」フィールドに名前を入力
3. 制限: 英数字とハイフンのみ、63文字以内
4. 「Save Network Settings」をクリック

**入力例**:
- ✅ `my-ntp-server`
- ✅ `office-gps-01`
- ❌ `server_with_underscore`（アンダースコア不可）
- ❌ `very-long-hostname-that-exceeds-the-maximum...`（長すぎ）

#### IP Configuration（IP設定）

**DHCP設定（推奨）**:
1. 「DHCP」ラジオボタンを選択
2. 静的IP設定フィールドが自動的に非表示
3. 「Save Network Settings」をクリック
4. システムが自動的にIPアドレスを取得

**Static IP設定**:
1. 「Static IP」ラジオボタンを選択
2. 静的IP設定フィールドが表示される
3. 各フィールドに適切な値を入力:
   - **IP Address**: `192.168.1.100`
   - **Netmask**: `255.255.255.0`
   - **Gateway**: `192.168.1.1`
4. 「Save Network Settings」をクリック

**⚠️ 注意**: Static IP設定後は新しいIPアドレスでアクセスしてください。

#### MAC Address表示

**表示場所**: Network タブ下部
**情報**: 読み取り専用、ネットワーク管理で使用
**形式**: `00:11:22:33:44:55`

### GNSS タブ

GPS/GNSS関連の設定と衛星システムの管理を行います。

#### Constellation Settings（衛星システム設定）

各衛星システムのチェックボックスで有効/無効を切り替え：

**GPS（Global Positioning System）**:
- ☑️ **推奨**: 常に有効
- **理由**: 最も安定した衛星システム
- **カバレッジ**: 全世界

**GLONASS（ロシア）**:
- ☑️ **推奨**: 有効
- **利点**: 高緯度地域での精度向上
- **併用効果**: GPS併用で衛星数増加

**Galileo（ヨーロッパ）**:
- ☑️ **推奨**: 有効
- **特徴**: 高精度、民間主導
- **将来性**: 拡張中のシステム

**BeiDou（中国）**:
- ☑️ **地域により推奨**: アジア太平洋で有効
- **注意**: 消費電力増加の可能性
- **カバレッジ**: アジア太平洋中心

**QZSS（日本準天頂衛星）**:
- ☑️ **日本では推奨**: 有効
- **特徴**: 日本上空の高仰角
- **追加機能**: 災害警報受信

#### Update Rate（更新レート設定）

**1Hz（推奨）**:
- **用途**: 標準的なNTP用途
- **消費電力**: 最小
- **精度**: 十分

**5Hz**:
- **用途**: 高頻度更新が必要な場合
- **消費電力**: 中程度
- **精度**: 向上

**10Hz**:
- **用途**: リアルタイム性が重要な場合
- **消費電力**: 最大
- **精度**: 最高

#### Disaster Alert Settings（災害警報設定）

**QZSS L1S災害警報**:
- **対象**: 日本国内のみ
- **機能**: 地震、津波、気象警報の受信
- **表示**: OLEDディスプレイに警報内容表示

**Alert Priority（警報優先度）**:
- **High**: 全ての警報を表示
- **Medium**: 重要な警報のみ
- **Low**: 緊急警報のみ

### NTP タブ

NTPサーバー機能の設定を行います。

#### NTP Server Settings

**Enable NTP Server**:
- ☑️ **チェック**: NTPサーバー機能有効
- ☐ **チェック外し**: NTPサーバー機能無効
- **注意**: 無効にするとNTPサービスが停止

**NTP Port**:
- **デフォルト**: `123`（標準NTPポート）
- **変更理由**: セキュリティポリシー、ファイアウォール設定
- **範囲**: 1-65535
- **⚠️ 注意**: 変更後はクライアント設定も更新必要

**Stratum Level**:
- **1**: GPS同期時（最高精度）
- **2**: セカンダリ参照時
- **3**: 内部クロック使用時
- **自動**: システムが自動選択（推奨）

#### 設定例

**標準設定**:
```
☑️ Enable NTP Server
Port: 123
Stratum: 1 (Auto)
```

**セキュリティ強化設定**:
```
☑️ Enable NTP Server  
Port: 1123
Stratum: 1 (Auto)
```

### System タブ

システム全体の動作設定を管理します。

#### Auto Restart Settings

**Enable Auto Restart**:
- ☐ **家庭利用推奨**: 無効
- ☑️ **長期運用**: 有効
- **目的**: メモリクリーンアップ、安定性向上

**Restart Interval**:
- **デフォルト**: 24時間
- **範囲**: 1-168時間（1週間）
- **推奨**: 24-72時間

#### Debug Settings

**Enable Debug Mode**:
- ☐ **通常運用**: 無効
- ☑️ **問題診断時**: 有効
- **影響**: ログ出力増加、若干の性能低下

#### 設定例

**家庭利用設定**:
```
☐ Auto Restart Enabled
Restart Interval: 24 hours
☐ Debug Mode Enabled
```

**商用環境設定**:
```
☑️ Auto Restart Enabled
Restart Interval: 72 hours  
☐ Debug Mode Enabled
```

### Status タブ

システムの現在状態をリアルタイムで監視します。

#### System Information

**表示項目**:
- **Uptime**: システム稼働時間
- **Free Memory**: 使用可能メモリ量
- **Network Status**: ネットワーク接続状態

**正常値の例**:
```
Uptime: 2h 35m
Free Memory: 485 KB
Network Status: Connected ✓
```

#### GPS Status

**表示項目**:
- **Fix Status**: GPS測位状態
  - ✅ `3D Fix`: 正常（推奨）
  - ⚠️ `2D Fix`: 2次元測位
  - ❌ `No Fix`: 測位不可
- **Satellites**: 衛星数（全体/使用中）
- **Position**: 現在位置（緯度・経度）
- **PPS Status**: PPS信号状態

**正常値の例**:
```
Fix Status: 3D Fix ✓
Satellites: 12 / 8
Position: 35.681382, 139.766084
PPS Status: Active ✓
```

#### NTP Status

**表示項目**:
- **Current Stratum**: 現在の階層レベル
- **NTP Requests**: 処理したNTP要求数
- **Active Clients**: 接続中クライアント数

**正常値の例**:
```
Current Stratum: 1 ✓
NTP Requests: 1,247
Active Clients: 5
```

#### 自動更新機能

- **更新間隔**: 5秒
- **更新対象**: Status タブの全情報
- **停止条件**: 他のタブを表示中は更新停止
- **手動更新**: 「Refresh Status」ボタンで即座更新

## 高度な操作

### 設定の一括管理

#### 設定のエクスポート（手動）
現在は手動記録のみ対応：
1. 各タブの設定値を記録
2. 重要設定を優先的に保存
3. テキストファイルで管理

#### 設定のインポート（手動）
1. 記録した設定値を参照
2. 各タブで手動入力
3. 順番に保存実行

### バックアップ・復旧

#### 個別設定のリセット
各タブの「Reset to Defaults」ボタン：
1. 該当カテゴリのみデフォルト値に復帰
2. 他のカテゴリ設定は影響なし
3. 即座に保存される

#### 全設定のリセット（工場出荷時）
物理ボタンまたはWeb画面から実行：

**物理ボタン**:
1. リセットボタンを5秒以上長押し
2. OLEDディスプレイで進行状況確認
3. 自動再起動

**Web画面**:
1. System タブの「Factory Reset」
2. 確認ダイアログで「OK」
3. 再起動待機

### 診断機能

#### ネットワーク診断
Status タブで以下を確認：
1. Network Status表示
2. IP Address情報
3. Connection状態

#### GPS診断
Status タブで以下を確認：
1. Fix Status（3D Fixが理想）
2. Satellite数（8個以上推奨）
3. PPS Status（Activeが必要）

#### システム診断
Status タブで以下を確認：
1. Free Memory（400KB以上推奨）
2. Uptime（安定性指標）
3. Error表示の有無

## エラー処理と対応

### 表示されるメッセージ

#### 成功メッセージ（緑色）
- `Settings saved successfully ✓`
- `Configuration updated ✓`
- `Network connection established ✓`

#### 警告メッセージ（黄色）
- `GPS signal weak - check antenna ⚠️`
- `High memory usage detected ⚠️`
- `Some satellites unavailable ⚠️`

#### エラーメッセージ（赤色）
- `Invalid IP address format ❌`
- `Port number out of range ❌`
- `Configuration save failed ❌`

### 一般的なエラーと対処

#### 設定保存エラー
**エラー**: `Configuration save failed`
**原因**: 
- 不正な入力値
- システムリソース不足
- ハードウェア問題

**対処**:
1. 入力値の妥当性確認
2. ページ再読み込み
3. システム再起動

#### ネットワーク接続エラー
**エラー**: Webページにアクセスできない
**原因**:
- IPアドレス変更
- ネットワーク設定ミス
- デバイス故障

**対処**:
1. IPアドレス再確認
2. ケーブル接続確認
3. 工場出荷時リセット

#### GPS関連エラー
**エラー**: `GPS Fix unavailable`
**原因**:
- アンテナ配置不良
- 信号遮蔽
- ハードウェア故障

**対処**:
1. アンテナ位置変更
2. 屋外での動作確認
3. 15分以上待機

## パフォーマンス最適化

### 推奨設定（家庭利用）

```
Network:
  Hostname: gps-ntp-server
  IP: DHCP

GNSS:
  GPS: ✓ Enabled
  GLONASS: ✓ Enabled  
  Galileo: ✓ Enabled
  BeiDou: ☐ Disabled (省電力)
  QZSS: ✓ Enabled (日本)
  Update Rate: 1Hz

NTP:
  Server: ✓ Enabled
  Port: 123
  Stratum: Auto

System:
  Auto Restart: ☐ Disabled
  Debug: ☐ Disabled
```

### 推奨設定（商用環境）

```
Network:
  Hostname: prod-ntp-01
  IP: Static IP

GNSS:
  All Systems: ✓ Enabled
  Update Rate: 5Hz

NTP:
  Server: ✓ Enabled
  Port: 123
  Stratum: Auto

System:
  Auto Restart: ✓ Enabled (72h)
  Debug: ☐ Disabled
```

## ブラウザ互換性

### 対応ブラウザ

**デスクトップ**:
- Chrome 70+
- Firefox 65+
- Safari 12+
- Edge 79+

**モバイル**:
- Chrome Mobile 70+
- Safari Mobile 12+
- Samsung Internet 10+

### 機能要件

**必須機能**:
- JavaScript有効
- Cookies有効
- ES6対応

**推奨機能**:
- CSS Grid/Flexbox対応
- Fetch API対応
- FormData API対応

## セキュリティ考慮事項

### アクセス制御

**現状**:
- IP制限なし（LAN内からのアクセス）
- 認証なし（設定変更自由）

**推奨対策**:
1. ファイアウォールでWAN側アクセス制限
2. 管理用VLANでの運用
3. 物理的なアクセス制御

### データ保護

**実装済み**:
- 入力値サニタイゼーション
- HTMLエスケープ処理
- レート制限（30req/min）

**ユーザー側対策**:
1. 信頼できるネットワークからのアクセス
2. 定期的な設定確認
3. 不審なアクセスログ監視

---

**バージョン**: 1.0  
**最終更新**: 2025-07-30  
**対象ファームウェア**: v1.0.0以降
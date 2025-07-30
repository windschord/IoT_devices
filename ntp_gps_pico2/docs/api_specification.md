# GPS NTP Server - API仕様書

## 概要

GPS NTP ServerのREST API仕様を定義します。本APIは、システム設定、状態監視、診断機能へのプログラマティックアクセスを提供します。

## 基本情報

- **Base URL**: `http://[device_ip_address]/`
- **API Version**: v1.0
- **Content-Type**: `application/json`
- **Character Encoding**: UTF-8
- **Rate Limiting**: 30 requests/minute per IP address

## 認証

現在のバージョンでは認証は実装されていません。将来のバージョンで以下の認証方式の実装を予定しています：

- Basic Authentication
- API Key Authentication
- Session-based Authentication

## HTTP ステータスコード

| Code | Status | Description |
|------|--------|-------------|
| 200 | OK | リクエスト成功 |
| 201 | Created | リソース作成成功 |
| 400 | Bad Request | 不正なリクエスト |
| 404 | Not Found | リソースが存在しない |
| 429 | Too Many Requests | レート制限に達した |
| 500 | Internal Server Error | サーバー内部エラー |
| 503 | Service Unavailable | サービス利用不可 |

## 共通レスポンス形式

### 成功レスポンス

```json
{
  "status": "success",
  "message": "Operation completed successfully",
  "data": {
    // レスポンスデータ
  },
  "timestamp": "2025-07-30T12:34:56Z"
}
```

### エラーレスポンス

```json
{
  "status": "error",
  "error_code": "INVALID_REQUEST",
  "message": "Invalid request parameters",
  "details": {
    "field": "hostname",
    "reason": "Hostname must be alphanumeric"
  },
  "timestamp": "2025-07-30T12:34:56Z"
}
```

## API エンドポイント

### 1. システム情報

#### GET /api/info

システムの基本情報を取得します。

**レスポンス例:**
```json
{
  "status": "success",
  "data": {
    "system": {
      "name": "GPS NTP Server",
      "version": "1.0.0", 
      "build_date": "2025-07-30T10:15:30Z",
      "hardware": "Raspberry Pi Pico 2",
      "cpu": "RP2350 150MHz Dual-Core"
    },
    "capabilities": {
      "ntp_server": true,
      "web_interface": true,
      "prometheus_metrics": true,
      "syslog_client": true,
      "qzss_l1s": true
    }
  }
}
```

### 2. 設定管理 API

#### GET /api/config

全設定を取得します。

**レスポンス例:**
```json
{
  "status": "success",
  "data": {
    "network": {
      "hostname": "gps-ntp-server",
      "dhcp_enabled": true,
      "static_ip": null,
      "netmask": null,
      "gateway": null,
      "mac_address": "02:00:00:12:34:56"
    },
    "gnss": {
      "gps_enabled": true,
      "glonass_enabled": true,
      "galileo_enabled": true,
      "beidou_enabled": false,
      "qzss_enabled": true,
      "update_rate": 1,
      "l1s_enabled": true,
      "alert_priority": 1
    },
    "ntp": {
      "server_enabled": true,
      "port": 123,
      "stratum": 1
    },
    "system": {
      "auto_restart_enabled": false,
      "restart_interval_hours": 24,
      "debug_mode_enabled": false
    },
    "log": {
      "syslog_server": "192.168.1.10",
      "syslog_port": 514,
      "log_level": 1
    }
  }
}
```

#### POST /api/config

設定を更新します。

**リクエスト例:**
```json
{
  "network": {
    "hostname": "my-ntp-server",
    "dhcp_enabled": false,
    "static_ip": "192.168.1.100",
    "netmask": "255.255.255.0",
    "gateway": "192.168.1.1"
  }
}
```

**レスポンス例:**
```json
{
  "status": "success",
  "message": "Configuration updated successfully",
  "data": {
    "updated_sections": ["network"],
    "restart_required": false
  }
}
```

#### GET /api/config/network

ネットワーク設定のみを取得します。

**レスポンス例:**
```json
{
  "status": "success",
  "data": {
    "hostname": "gps-ntp-server",
    "dhcp_enabled": true,
    "static_ip": null,
    "netmask": null,
    "gateway": null,
    "mac_address": "02:00:00:12:34:56",
    "current_ip": "192.168.1.150",
    "connection_status": "connected"
  }
}
```

#### POST /api/config/network

ネットワーク設定を更新します。

**リクエスト例:**
```json
{
  "hostname": "office-ntp-01",
  "dhcp_enabled": false,
  "static_ip": "192.168.1.100",
  "netmask": "255.255.255.0", 
  "gateway": "192.168.1.1"
}
```

#### GET /api/config/gnss

GNSS設定のみを取得します。

**レスポンス例:**
```json
{
  "status": "success",
  "data": {
    "gps_enabled": true,
    "glonass_enabled": true,
    "galileo_enabled": true,
    "beidou_enabled": false,
    "qzss_enabled": true,
    "update_rate": 1,
    "l1s_enabled": true,
    "alert_priority": 1,
    "supported_constellations": ["GPS", "GLONASS", "Galileo", "BeiDou", "QZSS"]
  }
}
```

#### POST /api/config/gnss

GNSS設定を更新します。

**リクエスト例:**
```json
{
  "gps_enabled": true,
  "glonass_enabled": true,
  "galileo_enabled": true,
  "beidou_enabled": true,
  "qzss_enabled": true,
  "update_rate": 5,
  "l1s_enabled": true,
  "alert_priority": 0
}
```

#### GET /api/config/ntp

NTP設定のみを取得します。

**レスポンス例:**
```json
{
  "status": "success",
  "data": {
    "server_enabled": true,
    "port": 123,
    "stratum": 1,
    "current_clients": 5,
    "total_requests": 1247
  }
}
```

#### POST /api/config/ntp

NTP設定を更新します。

**リクエスト例:**
```json
{
  "server_enabled": true,
  "port": 1123,
  "stratum": 2
}
```

#### GET /api/config/system

システム設定のみを取得します。

**レスポンス例:**
```json
{
  "status": "success", 
  "data": {
    "auto_restart_enabled": false,
    "restart_interval_hours": 24,
    "debug_mode_enabled": false,
    "uptime_seconds": 7200,
    "free_memory_bytes": 485320
  }
}
```

#### POST /api/config/system

システム設定を更新します。

**リクエスト例:**
```json
{
  "auto_restart_enabled": true,
  "restart_interval_hours": 72,
  "debug_mode_enabled": false
}
```

#### GET /api/config/log

ログ設定のみを取得します。

**レスポンス例:**
```json
{
  "status": "success",
  "data": {
    "syslog_server": "192.168.1.10",
    "syslog_port": 514,
    "log_level": 1,
    "syslog_enabled": true,
    "local_buffer_size": 4096,
    "messages_sent": 156
  }
}
```

#### POST /api/config/log

ログ設定を更新します。

**リクエスト例:**
```json
{
  "syslog_server": "10.0.1.100", 
  "syslog_port": 514,
  "log_level": 2
}
```

### 3. システム状態 API

#### GET /api/status

システム全体の状態を取得します。

**レスポンス例:**
```json
{
  "status": "success",
  "data": {
    "system": {
      "state": "RUNNING",
      "uptime_seconds": 7200,
      "free_memory_bytes": 485320,
      "health_score": 98
    },
    "network": {
      "connected": true,
      "ip_address": "192.168.1.150",
      "mac_address": "02:00:00:12:34:56",
      "link_speed": "100Mbps"
    },
    "gps": {
      "fix_status": "3D Fix",
      "satellites_total": 26,
      "satellites_used": 12,
      "pps_active": true,
      "last_fix_time": "2025-07-30T12:34:56Z"
    },
    "ntp": {
      "server_running": true,
      "current_stratum": 1,
      "requests_total": 1247,
      "active_clients": 5
    }
  }
}
```

#### GET /api/status/gps

GPS状態の詳細を取得します。

**レスポンス例:**
```json
{
  "status": "success",
  "data": {
    "fix_status": "3D Fix",
    "fix_type": 3,
    "position": {
      "latitude": 35.681382,
      "longitude": 139.766084,
      "altitude": 25.5
    },
    "accuracy": {
      "horizontal": 2.1,
      "vertical": 3.8,
      "time": 50
    },
    "satellites": {
      "gps": {"total": 12, "used": 8},
      "glonass": {"total": 8, "used": 3},
      "galileo": {"total": 6, "used": 1},
      "beidou": {"total": 0, "used": 0},
      "qzss": {"total": 2, "used": 1}
    },
    "pps": {
      "active": true,
      "last_pulse": "2025-07-30T12:34:56.123Z",
      "pulse_count": 7200
    },
    "qzss_l1s": {
      "signal_detected": false,
      "last_message": null,
      "message_count": 0
    }
  }
}
```

#### GET /api/status/ntp

NTP統計情報を取得します。

**レスポンス例:**
```json
{
  "status": "success",
  "data": {
    "server_running": true,
    "current_stratum": 1,
    "reference_time": "2025-07-30T12:34:56.123456Z",
    "statistics": {
      "requests_total": 1247,
      "requests_per_minute": 15,
      "active_clients": 5,
      "average_response_time_ms": 2.3
    },
    "clients": [
      {
        "ip": "192.168.1.10",
        "last_request": "2025-07-30T12:34:55Z",
        "request_count": 156
      },
      {
        "ip": "192.168.1.20", 
        "last_request": "2025-07-30T12:34:52Z",
        "request_count": 89
      }
    ]
  }
}
```

#### GET /api/status/network

ネットワーク状態を取得します。

**レスポンス例:**
```json
{
  "status": "success",
  "data": {
    "connection": {
      "connected": true,
      "link_status": "up",
      "speed": "100Mbps",
      "duplex": "full"
    },
    "addressing": {
      "ip_address": "192.168.1.150",
      "netmask": "255.255.255.0",
      "gateway": "192.168.1.1",
      "mac_address": "02:00:00:12:34:56"
    },
    "statistics": {
      "bytes_sent": 1048576,
      "bytes_received": 2097152,
      "packets_sent": 1024,
      "packets_received": 2048,
      "errors": 0
    }
  }
}
```

#### GET /api/status/health

ヘルスチェック情報を取得します。

**レスポンス例:**
```json
{
  "status": "success",
  "data": {
    "overall_health": "healthy",
    "health_score": 98,
    "services": {
      "gps_client": {"status": "healthy", "score": 100},
      "ntp_server": {"status": "healthy", "score": 98},
      "web_server": {"status": "healthy", "score": 95},
      "config_manager": {"status": "healthy", "score": 100},
      "logging_service": {"status": "healthy", "score": 90},
      "display_manager": {"status": "healthy", "score": 85},
      "network_manager": {"status": "healthy", "score": 100},
      "prometheus_metrics": {"status": "healthy", "score": 95}
    },
    "alerts": [],
    "last_check": "2025-07-30T12:34:56Z"
  }
}
```

### 4. 診断・制御 API

#### POST /api/diagnostic/network

ネットワーク診断を実行します。

**リクエスト例:**
```json
{
  "tests": ["connectivity", "dns", "gateway"],
  "target_host": "8.8.8.8"
}
```

**レスポンス例:**
```json
{
  "status": "success",
  "data": {
    "test_results": {
      "connectivity": {
        "status": "pass",
        "details": "Network interface is up and configured"
      },
      "dns": {
        "status": "pass", 
        "details": "DNS resolution working",
        "resolver": "192.168.1.1"
      },
      "gateway": {
        "status": "pass",
        "details": "Gateway reachable",
        "response_time_ms": 5
      }
    },
    "overall_result": "pass",
    "execution_time_ms": 1250
  }
}
```

#### POST /api/diagnostic/gps

GPS診断を実行します。

**レスポンス例:**
```json
{
  "status": "success",
  "data": {
    "test_results": {
      "hardware": {
        "status": "pass",
        "details": "ZED-F9T responding on I2C"
      },
      "signal": {
        "status": "pass",
        "details": "Satellites visible: 26, Fix available"
      },
      "pps": {
        "status": "pass",
        "details": "PPS pulses detected, frequency: 1.000Hz"
      },
      "timing": {
        "status": "pass",
        "details": "GPS time synchronized, accuracy: 50ns"
      }
    },
    "overall_result": "pass",
    "execution_time_ms": 5000
  }
}
```

#### POST /api/reset

工場出荷時設定リセットを実行します。

**リクエスト例:**
```json
{
  "confirm": true,
  "reset_type": "factory"
}
```

**レスポンス例:**
```json
{
  "status": "success",
  "message": "Factory reset initiated",
  "data": {
    "reset_type": "factory",
    "estimated_completion_time": "30 seconds",
    "auto_restart": true
  }
}
```

#### POST /api/restart

システム再起動を実行します。

**リクエスト例:**
```json
{
  "confirm": true,
  "restart_type": "soft"
}
```

**レスポンス例:**
```json
{
  "status": "success",
  "message": "System restart initiated",
  "data": {
    "restart_type": "soft",
    "estimated_downtime": "60 seconds"
  }
}
```

### 5. メトリクス API

#### GET /metrics

Prometheus形式のメトリクスを取得します。

**レスポンス例:**
```
# HELP gps_satellites_total Number of visible satellites by constellation
# TYPE gps_satellites_total gauge
gps_satellites_total{constellation="GPS"} 12
gps_satellites_total{constellation="GLONASS"} 8
gps_satellites_total{constellation="Galileo"} 6
gps_satellites_total{constellation="BeiDou"} 0
gps_satellites_total{constellation="QZSS"} 2

# HELP ntp_requests_total Total number of NTP requests processed
# TYPE ntp_requests_total counter
ntp_requests_total 1247

# HELP system_uptime_seconds System uptime in seconds
# TYPE system_uptime_seconds counter
system_uptime_seconds 7200

# HELP memory_free_bytes Available memory in bytes
# TYPE memory_free_bytes gauge
memory_free_bytes 485320
```

#### GET /api/metrics

JSON形式のメトリクスを取得します。

**レスポンス例:**
```json
{
  "status": "success",
  "data": {
    "gps": {
      "satellites_gps": 12,
      "satellites_glonass": 8,
      "satellites_galileo": 6,
      "satellites_beidou": 0,
      "satellites_qzss": 2,
      "satellites_used": 12,
      "fix_type": 3,
      "pps_pulses": 7200
    },
    "ntp": {
      "requests_total": 1247,
      "requests_per_minute": 15,
      "active_clients": 5,
      "stratum": 1
    },
    "system": {
      "uptime_seconds": 7200,
      "free_memory_bytes": 485320,
      "health_score": 98,
      "cpu_usage_percent": 70
    },
    "timestamp": "2025-07-30T12:34:56Z"
  }
}
```

## エラーコード

### 設定関連エラー

| Code | Message | Description |
|------|---------|-------------|
| CONFIG_INVALID_FORMAT | Invalid configuration format | 設定形式が不正 |
| CONFIG_VALIDATION_FAILED | Configuration validation failed | 設定値の検証に失敗 |
| CONFIG_SAVE_FAILED | Failed to save configuration | 設定の保存に失敗 |
| CONFIG_LOAD_FAILED | Failed to load configuration | 設定の読み込みに失敗 |

### ネットワーク関連エラー

| Code | Message | Description |
|------|---------|-------------|
| NETWORK_INVALID_IP | Invalid IP address format | 不正なIPアドレス形式 |
| NETWORK_INVALID_HOSTNAME | Invalid hostname format | 不正なホスト名形式 |
| NETWORK_CONNECTION_FAILED | Network connection failed | ネットワーク接続に失敗 |

### GPS関連エラー

| Code | Message | Description |
|------|---------|-------------|
| GPS_NO_FIX | GPS fix not available | GPS測位不可 |
| GPS_HARDWARE_ERROR | GPS hardware error | GPSハードウェアエラー |
| GPS_COMMUNICATION_ERROR | GPS communication error | GPS通信エラー |

### システム関連エラー

| Code | Message | Description |
|------|---------|-------------|
| SYSTEM_MEMORY_LOW | System memory low | システムメモリ不足 |
| SYSTEM_OVERLOAD | System overload | システム過負荷 |
| SYSTEM_HARDWARE_ERROR | System hardware error | システムハードウェアエラー |

## レート制限

各IPアドレスからのリクエストは1分間に30回まで制限されています。制限に達した場合、以下のレスポンスが返されます：

```json
{
  "status": "error",
  "error_code": "RATE_LIMIT_EXCEEDED",
  "message": "Rate limit exceeded. Please try again later.",
  "details": {
    "limit": 30,
    "window": "1 minute",
    "retry_after": 45
  }
}
```

## WebSocket API (将来実装予定)

リアルタイム状態更新のためのWebSocket APIを将来実装予定です。

```javascript
// 接続例
const ws = new WebSocket('ws://192.168.1.100/ws');

// リアルタイム状態受信
ws.onmessage = function(event) {
  const data = JSON.parse(event.data);
  console.log('Status update:', data);
};
```

## SDKとライブラリ

### JavaScript/Node.js

```javascript
class GpsNtpClient {
  constructor(baseUrl) {
    this.baseUrl = baseUrl;
  }
  
  async getStatus() {
    const response = await fetch(`${this.baseUrl}/api/status`);
    return await response.json();
  }
  
  async updateConfig(config) {
    const response = await fetch(`${this.baseUrl}/api/config`, {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify(config)
    });
    return await response.json();
  }
}
```

### Python

```python
import requests

class GpsNtpClient:
    def __init__(self, base_url):
        self.base_url = base_url
    
    def get_status(self):
        response = requests.get(f"{self.base_url}/api/status")
        return response.json()
    
    def update_config(self, config):
        response = requests.post(
            f"{self.base_url}/api/config",
            json=config
        )
        return response.json()
```

### cURL使用例

```bash
# システム状態取得
curl -X GET http://192.168.1.100/api/status

# 設定更新
curl -X POST http://192.168.1.100/api/config \
  -H "Content-Type: application/json" \
  -d '{"network":{"hostname":"my-ntp-server"}}'

# GPS診断実行
curl -X POST http://192.168.1.100/api/diagnostic/gps

# メトリクス取得（Prometheus形式）
curl -X GET http://192.168.1.100/metrics
```

## バージョニング

- **Version**: v1.0
- **Breaking Changes**: メジャーバージョンアップ時
- **Backward Compatibility**: マイナーバージョンアップでは維持
- **Deprecation Policy**: 廃止予定機能は2バージョン前に告知

---

**Document Version**: 1.0  
**Last Updated**: 2025-07-30  
**API Version**: v1.0  
**Target Firmware**: v1.0.0+
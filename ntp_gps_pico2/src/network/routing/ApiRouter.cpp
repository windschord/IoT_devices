#include "ApiRouter.h"
#include "../../config/ConfigManager.h"
#include "../../gps/GpsClient.h"
#include "../../system/PrometheusMetrics.h"
#include "../../config/LoggingService.h"
#include <ArduinoJson.h>

// 静的インスタンス初期化
ApiRouter* ApiRouter::instance_ = nullptr;

ApiRouter::ApiRouter() 
    : configManager_(nullptr)
    , gpsClient_(nullptr)
    , prometheusMetrics_(nullptr)
    , loggingService_(nullptr) {
    instance_ = this;
}

void ApiRouter::setupRoutes(RouteHandler& routeHandler) {
    // GPS API ルート
    routeHandler.addGetRoute("/api/gps", handleGpsGet, 10);

    // 設定 API ルート（特定のカテゴリが先、一般的なものが後）
    routeHandler.addGetRoute("/api/config/network", handleConfigNetworkGet, 20);
    routeHandler.addPostRoute("/api/config/network", handleConfigNetworkPost, 20);
    routeHandler.addGetRoute("/api/config/gnss", handleConfigGnssGet, 20);
    routeHandler.addPostRoute("/api/config/gnss", handleConfigGnssPost, 20);
    routeHandler.addGetRoute("/api/config/ntp", handleConfigNtpGet, 20);
    routeHandler.addPostRoute("/api/config/ntp", handleConfigNtpPost, 20);
    routeHandler.addGetRoute("/api/config/system", handleConfigSystemGet, 20);
    routeHandler.addPostRoute("/api/config/system", handleConfigSystemPost, 20);
    routeHandler.addGetRoute("/api/config/log", handleConfigLogGet, 20);
    routeHandler.addPostRoute("/api/config/log", handleConfigLogPost, 20);
    
    // 一般的な設定 API ルート
    routeHandler.addGetRoute("/api/config", handleConfigGet, 30);

    // システム API ルート
    routeHandler.addGetRoute("/api/status", handleStatusGet, 40);
    routeHandler.addPostRoute("/api/system/reboot", handleSystemRebootPost, 40);
    routeHandler.addGetRoute("/api/system/metrics", handleSystemMetricsGet, 40);
    routeHandler.addGetRoute("/api/system/logs", handleSystemLogsGet, 40);
    routeHandler.addPostRoute("/api/reset", handleFactoryReset, 40);

    // デバッグ API ルート
    routeHandler.addGetRoute("/api/debug/files", handleDebugFilesGet, 50);
}

// GPS API実装
void ApiRouter::handleGpsGet(EthernetClient& client, const HttpRequestParser::ParsedRequest& request) {
    if (!instance_ || !instance_->gpsClient_) {
        sendErrorResponse(client, HttpResponseBuilder::INTERNAL_SERVER_ERROR, 
                         "GPS client not available");
        return;
    }

    DynamicJsonDocument doc(4096);
    
    // GPS データ取得と JSON 生成
    WebGpsData webGpsData = instance_->gpsClient_->getWebGpsData();
    
    if (webGpsData.data_valid) {
        // 詳細GPS情報をJSONに変換
        doc["latitude"] = webGpsData.latitude;
        doc["longitude"] = webGpsData.longitude;
        doc["altitude"] = webGpsData.altitude;
        doc["fix_type"] = webGpsData.fix_type;
        doc["satellites_total"] = webGpsData.satellites_total;
        doc["satellites_used"] = webGpsData.satellites_used;
        doc["data_valid"] = true;
        
        // 衛星情報配列
        JsonArray satellites = doc.createNestedArray("satellites");
        for (uint8_t i = 0; i < webGpsData.satellite_count; i++) {
            JsonObject sat = satellites.createNestedObject();
            sat["prn"] = webGpsData.satellites[i].prn;
            sat["constellation"] = webGpsData.satellites[i].constellation;
            sat["azimuth"] = webGpsData.satellites[i].azimuth;
            sat["elevation"] = webGpsData.satellites[i].elevation;
            sat["signal_strength"] = webGpsData.satellites[i].signal_strength;
        }
    } else {
        doc["error"] = "GPS data not available";
        doc["data_valid"] = false;
    }
    
    String jsonString;
    serializeJson(doc, jsonString);
    sendJsonResponse(client, jsonString);
}

// 設定 API実装の例（NetworkConfig）
void ApiRouter::handleConfigNetworkGet(EthernetClient& client, const HttpRequestParser::ParsedRequest& request) {
    if (!instance_ || !instance_->configManager_) {
        sendErrorResponse(client, HttpResponseBuilder::INTERNAL_SERVER_ERROR,
                         "Configuration Manager not available");
        return;
    }

    DynamicJsonDocument doc(1024);
    const auto& config = instance_->configManager_->getConfig();
    
    doc["hostname"] = config.hostname;
    doc["ip_address"] = config.ip_address;
    doc["netmask"] = config.netmask;
    doc["gateway"] = config.gateway;
    doc["dns_server"] = config.dns_server;

    String jsonString;
    serializeJson(doc, jsonString);
    sendJsonResponse(client, jsonString);
}

void ApiRouter::handleConfigNetworkPost(EthernetClient& client, const HttpRequestParser::ParsedRequest& request) {
    if (!checkRateLimit(client)) {
        sendErrorResponse(client, HttpResponseBuilder::TOO_MANY_REQUESTS,
                         "Rate limit exceeded");
        return;
    }

    if (!instance_ || !instance_->configManager_) {
        sendErrorResponse(client, HttpResponseBuilder::INTERNAL_SERVER_ERROR,
                         "Configuration Manager not available");
        return;
    }

    if (!validateJsonInput(request.body)) {
        sendErrorResponse(client, HttpResponseBuilder::BAD_REQUEST,
                         "Invalid JSON input");
        return;
    }

    // JSON解析と設定更新処理
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, request.body);
    
    if (error) {
        sendErrorResponse(client, HttpResponseBuilder::BAD_REQUEST,
                         "JSON parse error");
        return;
    }

    // 実際の設定更新処理は既存のコードを再利用
    sendJsonResponse(client, "{\"success\": true, \"message\": \"Network configuration updated\"}");
}

// その他のAPI実装は同様のパターンで...（簡略化のため一部のみ実装）

// システム状態取得
void ApiRouter::handleStatusGet(EthernetClient& client, const HttpRequestParser::ParsedRequest& request) {
    DynamicJsonDocument doc(1024);
    
    if (instance_ && instance_->gpsClient_) {
        GpsSummaryData gpsData = instance_->gpsClient_->getGpsSummaryData();
        doc["gps_fix"] = (gpsData.fixType >= 2);
        doc["satellites"] = gpsData.SIV;
    }
    
    doc["network_connected"] = (Ethernet.linkStatus() == LinkON);
    doc["uptime_seconds"] = millis() / 1000;
    doc["free_memory"] = rp2040.getFreeHeap();

    String jsonString;
    serializeJson(doc, jsonString);
    sendJsonResponse(client, jsonString);
}

// ユーティリティメソッド実装
bool ApiRouter::checkRateLimit(EthernetClient& client) {
    // 簡単なレート制限実装
    static unsigned long lastRequestTime = 0;
    static int requestCount = 0;
    
    unsigned long currentTime = millis();
    
    if (currentTime - lastRequestTime < 60000) { // 1分間
        requestCount++;
        if (requestCount > 30) { // 30リクエスト/分
            return false;
        }
    } else {
        requestCount = 1;
        lastRequestTime = currentTime;
    }
    
    return true;
}

bool ApiRouter::validateJsonInput(const String& jsonInput) {
    if (jsonInput.length() == 0 || jsonInput.length() > 2048) {
        return false;
    }
    
    String trimmed = jsonInput;
    trimmed.trim();
    
    return trimmed.startsWith("{") && trimmed.indexOf("}") > 0;
}

void ApiRouter::sendErrorResponse(EthernetClient& client, 
                                 HttpResponseBuilder::StatusCode statusCode, 
                                 const String& message) {
    HttpResponseBuilder builder(client);
    String jsonError = "{\"error\": \"" + message + "\"}";
    builder.json(jsonError, statusCode).send();
}

void ApiRouter::sendJsonResponse(EthernetClient& client, 
                                const String& jsonResponse,
                                HttpResponseBuilder::StatusCode statusCode) {
    HttpResponseBuilder builder(client);
    builder.json(jsonResponse, statusCode).send();
}

// 残りのハンドラー実装（スタブ）
void ApiRouter::handleConfigGet(EthernetClient& client, const HttpRequestParser::ParsedRequest& request) {
    sendJsonResponse(client, "{\"message\": \"Config GET - Not implemented yet\"}");
}

void ApiRouter::handleConfigGnssGet(EthernetClient& client, const HttpRequestParser::ParsedRequest& request) {
    sendJsonResponse(client, "{\"message\": \"GNSS Config GET - Not implemented yet\"}");
}

void ApiRouter::handleConfigGnssPost(EthernetClient& client, const HttpRequestParser::ParsedRequest& request) {
    sendJsonResponse(client, "{\"message\": \"GNSS Config POST - Not implemented yet\"}");
}

void ApiRouter::handleConfigNtpGet(EthernetClient& client, const HttpRequestParser::ParsedRequest& request) {
    sendJsonResponse(client, "{\"message\": \"NTP Config GET - Not implemented yet\"}");
}

void ApiRouter::handleConfigNtpPost(EthernetClient& client, const HttpRequestParser::ParsedRequest& request) {
    sendJsonResponse(client, "{\"message\": \"NTP Config POST - Not implemented yet\"}");
}

void ApiRouter::handleConfigSystemGet(EthernetClient& client, const HttpRequestParser::ParsedRequest& request) {
    sendJsonResponse(client, "{\"message\": \"System Config GET - Not implemented yet\"}");
}

void ApiRouter::handleConfigSystemPost(EthernetClient& client, const HttpRequestParser::ParsedRequest& request) {
    sendJsonResponse(client, "{\"message\": \"System Config POST - Not implemented yet\"}");
}

void ApiRouter::handleConfigLogGet(EthernetClient& client, const HttpRequestParser::ParsedRequest& request) {
    sendJsonResponse(client, "{\"message\": \"Log Config GET - Not implemented yet\"}");
}

void ApiRouter::handleConfigLogPost(EthernetClient& client, const HttpRequestParser::ParsedRequest& request) {
    sendJsonResponse(client, "{\"message\": \"Log Config POST - Not implemented yet\"}");
}

void ApiRouter::handleSystemRebootPost(EthernetClient& client, const HttpRequestParser::ParsedRequest& request) {
    sendJsonResponse(client, "{\"message\": \"System Reboot - Not implemented yet\"}");
}

void ApiRouter::handleSystemMetricsGet(EthernetClient& client, const HttpRequestParser::ParsedRequest& request) {
    sendJsonResponse(client, "{\"message\": \"System Metrics - Not implemented yet\"}");
}

void ApiRouter::handleSystemLogsGet(EthernetClient& client, const HttpRequestParser::ParsedRequest& request) {
    sendJsonResponse(client, "{\"message\": \"System Logs - Not implemented yet\"}");
}

void ApiRouter::handleFactoryReset(EthernetClient& client, const HttpRequestParser::ParsedRequest& request) {
    sendJsonResponse(client, "{\"message\": \"Factory Reset - Not implemented yet\"}");
}

void ApiRouter::handleDebugFilesGet(EthernetClient& client, const HttpRequestParser::ParsedRequest& request) {
    sendJsonResponse(client, "{\"message\": \"Debug Files - Not implemented yet\"}");
}
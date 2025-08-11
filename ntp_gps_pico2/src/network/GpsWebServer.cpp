#include "GpsWebServer.h"
#include "NtpServer.h"
#include "../config/ConfigManager.h"
#include "../system/PrometheusMetrics.h"
#include "../config/LoggingService.h"
#include "../gps/GpsClient.h"

ModernGpsWebServer::ModernGpsWebServer()
    : ntpServer_(nullptr)
    , configManager_(nullptr)
    , prometheusMetrics_(nullptr)
    , loggingService_(nullptr)
    , gpsClient_(nullptr)
    , cacheManager_(nullptr)
    , initialized_(false) {
    
    // 統計を初期化
    statistics_.requestCount = 0;
    statistics_.errorCount = 0;
    statistics_.averageResponseTime = 0;
    statistics_.cacheHitRatio = 0.0f;
    
    // デフォルト設定を適用
    config_ = getDefaultConfig();
}

ModernGpsWebServer::~ModernGpsWebServer() {
    if (cacheManager_) {
        delete cacheManager_;
    }
}

void ModernGpsWebServer::setConfigManager(ConfigManager* configManagerInstance) {
    configManager_ = configManagerInstance;
    apiRouter_.setConfigManager(configManagerInstance);
}

void ModernGpsWebServer::setPrometheusMetrics(PrometheusMetrics* prometheusMetricsInstance) {
    prometheusMetrics_ = prometheusMetricsInstance;
    apiRouter_.setPrometheusMetrics(prometheusMetricsInstance);
}

void ModernGpsWebServer::setLoggingService(LoggingService* loggingServiceInstance) {
    loggingService_ = loggingServiceInstance;
    apiRouter_.setLoggingService(loggingServiceInstance);
    fileRouter_.setLoggingService(loggingServiceInstance);
    fileSystemHandler_.setLoggingService(loggingServiceInstance);
}

void ModernGpsWebServer::setGpsClient(GpsClient* gpsClientInstance) {
    gpsClient_ = gpsClientInstance;
    apiRouter_.setGpsClient(gpsClientInstance);
}

void ModernGpsWebServer::configure(const ServerConfig& config) {
    config_ = config;
    
    // キャッシュマネージャーの再初期化
    if (cacheManager_) {
        delete cacheManager_;
    }
    
    if (config_.enableCaching) {
        cacheManager_ = new CacheManager(config_.maxCacheEntries, config_.maxCacheSize);
    }
}

bool ModernGpsWebServer::initialize() {
    if (initialized_) {
        return true;
    }

    log("INFO", "Initializing GPS Web Server");

    // ファイルシステムハンドラーの初期化
    if (!fileSystemHandler_.initialize()) {
        log("ERROR", "Failed to initialize filesystem handler");
        return false;
    }

    // キャッシュマネージャーの初期化
    if (config_.enableCaching && !cacheManager_) {
        cacheManager_ = new CacheManager(config_.maxCacheEntries, config_.maxCacheSize);
    }

    // ルーティングテーブルの設定
    setupRoutes();

    initialized_ = true;
    log("INFO", "GPS Web Server initialized successfully");
    return true;
}

void ModernGpsWebServer::handleClient(Stream& stream, EthernetServer& server,
                               UBX_NAV_SAT_data_t* ubxNavSatData_t,
                               GpsSummaryData gpsSummaryData) {
    // 従来のインターフェース互換性のため、新しい処理に転送
    processRequests(server);
}

void ModernGpsWebServer::processRequests(EthernetServer& server) {
    if (!initialized_) {
        if (!initialize()) {
            return;
        }
    }

    EthernetClient client = server.available();
    if (client) {
        handleSingleRequest(client);
        client.stop();
    }
}

void ModernGpsWebServer::setupRoutes() {
    log("INFO", "Setting up routing table");

    // APIルートの設定（高優先度）
    apiRouter_.setupRoutes(routeHandler_);

    // ファイルルートの設定（低優先度）
    fileRouter_.setupRoutes(routeHandler_);

    log("INFO", "Routing table configured with " + String(routeHandler_.getRouteCount()) + " routes");
}

void ModernGpsWebServer::handleSingleRequest(EthernetClient& client) {
    unsigned long startTime = millis();
    bool hasError = false;

    // 例外処理は Arduino 環境では使用できないため、エラーチェックベースに変更
    log("INFO", "New HTTP client connected from " + client.remoteIP().toString());

    // リクエストを読み込み
    String rawRequest = readRequest(client);
    if (rawRequest.length() == 0) {
        log("WARNING", "Empty request received");
        HttpResponseBuilder::send404(client);
        hasError = true;
        return;
    }

    // リクエストを解析
    HttpRequestParser::ParsedRequest request = HttpRequestParser::parseRequest(rawRequest);
    if (!request.isValid) {
        log("ERROR", "Invalid HTTP request format");
        HttpResponseBuilder::send404(client);
        hasError = true;
        return;
    }

    log("INFO", "HTTP Request: " + request.method + " " + request.path);

    // ルーターに処理を委譲
    if (!routeHandler_.route(client, request)) {
        log("WARNING", "No route found for: " + request.path);
        hasError = true;
    }

    // 統計を更新
    unsigned long responseTime = millis() - startTime;
    updateStatistics(responseTime, hasError);
    
    log("INFO", "Request processed in " + String(responseTime) + "ms");
}

String ModernGpsWebServer::readRequest(EthernetClient& client) {
    String request = "";
    boolean currentLineIsBlank = true;
    boolean headerComplete = false;
    int contentLength = 0;
    
    unsigned long requestStartTime = millis();
    const unsigned long timeout = config_.requestTimeout * 1000;
    
    while (client.connected() && (millis() - requestStartTime < timeout)) {
        if (client.available()) {
            char c = client.read();
            request += c;
            
            if (!headerComplete) {
                if (c == '\n' && currentLineIsBlank) {
                    headerComplete = true;
                    
                    // Content-Lengthを解析
                    contentLength = HttpRequestParser::parseContentLength(request);
                    
                    // ボディがない場合は終了
                    if (contentLength == 0) {
                        break;
                    }
                }
                
                if (c == '\n') {
                    currentLineIsBlank = true;
                } else if (c != '\r') {
                    currentLineIsBlank = false;
                }
            } else {
                // ボディの読み込み
                if (request.length() >= request.indexOf("\r\n\r\n") + 4 + contentLength) {
                    break;
                }
            }
        } else {
            // データが利用できない場合は少し待つ
            delay(1);
        }
    }
    
    return request;
}

ModernGpsWebServer::Statistics ModernGpsWebServer::getStatistics() const {
    Statistics stats = statistics_;
    
    // キャッシュヒット率を更新
    if (cacheManager_) {
        stats.cacheHitRatio = cacheManager_->getHitRatio();
    }
    
    return stats;
}

void ModernGpsWebServer::invalidateGpsCache() {
    if (cacheManager_) {
        cacheManager_->removeByPattern("/api/gps*");
    }
}

// プライベートメソッド実装
void ModernGpsWebServer::log(const String& level, const String& message) {
    if (loggingService_) {
        if (level == "ERROR") {
            loggingService_->error("WEB", message.c_str());
        } else if (level == "WARNING") {
            loggingService_->warning("WEB", message.c_str());
        } else {
            loggingService_->info("WEB", message.c_str());
        }
    }
}

void ModernGpsWebServer::updateStatistics(unsigned long responseTime, bool isError) {
    statistics_.requestCount++;
    
    if (isError) {
        statistics_.errorCount++;
    }
    
    // 移動平均でレスポンス時間を更新
    if (statistics_.requestCount == 1) {
        statistics_.averageResponseTime = responseTime;
    } else {
        statistics_.averageResponseTime = 
            (statistics_.averageResponseTime * 9 + responseTime) / 10;
    }
}

ModernGpsWebServer::ServerConfig ModernGpsWebServer::getDefaultConfig() const {
    ServerConfig config;
    config.enableCaching = true;
    config.enableCompression = false;  // 組み込みシステムでは無効
    config.maxCacheEntries = 10;
    config.maxCacheSize = 8192;       // 8KB
    config.requestTimeout = 10;       // 10秒
    config.enableAccessLog = true;
    config.enableDebugApi = false;
    return config;
}
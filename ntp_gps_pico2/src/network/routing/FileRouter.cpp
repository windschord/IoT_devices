#include "FileRouter.h"
#include "../../config/LoggingService.h"
#include "../../gps/GpsClient.h"
#include "../../system/PrometheusMetrics.h"
#include <LittleFS.h>

// 静的インスタンス初期化
FileRouter* FileRouter::instance_ = nullptr;

FileRouter::FileRouter() 
    : routeCount_(0)
    , loggingService_(nullptr) {
    instance_ = this;
    
    // ルート配列初期化
    for (int i = 0; i < MAX_FILE_ROUTES; i++) {
        fileRoutes_[i].enabled = false;
    }
}

void FileRouter::setupRoutes(RouteHandler& routeHandler) {
    // 静的ファイルルートを設定
    setupDefaultRoutes();
    
    // ルートハンドラーにファイルルートを登録
    for (int i = 0; i < routeCount_; i++) {
        if (fileRoutes_[i].enabled) {
            // 各ファイルタイプに応じたハンドラーを設定
            RouteHandler::HandlerFunction handler = nullptr;
            
            if (fileRoutes_[i].urlPath == "/" || fileRoutes_[i].urlPath == "/index") {
                handler = handleMainPage;
            } else if (fileRoutes_[i].urlPath == "/gps") {
                handler = handleGpsPage;
            } else if (fileRoutes_[i].urlPath == "/gps.js") {
                handler = handleGpsScript;
            } else if (fileRoutes_[i].urlPath == "/config") {
                handler = handleConfigPage;
            } else if (fileRoutes_[i].urlPath == "/config.js") {
                handler = handleConfigScript;
            } else if (fileRoutes_[i].urlPath == "/metrics") {
                handler = handleMetricsPage;
            }
            
            if (handler != nullptr) {
                routeHandler.addGetRoute(fileRoutes_[i].urlPath, handler, 80);
            }
        }
    }
}

bool FileRouter::addFileRoute(const String& urlPath, 
                             const String& filePath,
                             const String& mimeType,
                             bool cacheEnabled,
                             int cacheDuration) {
    if (routeCount_ >= MAX_FILE_ROUTES) {
        return false;
    }

    fileRoutes_[routeCount_].urlPath = urlPath;
    fileRoutes_[routeCount_].filePath = filePath;
    fileRoutes_[routeCount_].mimeType = mimeType.length() > 0 ? mimeType : getMimeType(filePath);
    fileRoutes_[routeCount_].cacheEnabled = cacheEnabled;
    fileRoutes_[routeCount_].cacheDuration = cacheDuration;
    fileRoutes_[routeCount_].enabled = true;
    
    routeCount_++;
    return true;
}

void FileRouter::setupDefaultRoutes() {
    // デフォルトのファイルルート設定
    addFileRoute("/", "/index.html", "text/html", true, 300);
    addFileRoute("/gps", "/gps.html", "text/html", false, 0);
    addFileRoute("/gps.js", "/gps.js", "text/javascript", true, 3600);
    addFileRoute("/config", "/config.html", "text/html", false, 0);
    addFileRoute("/config.js", "/config.js", "text/javascript", true, 3600);
    addFileRoute("/metrics", "", "text/plain", false, 0); // 特別処理
}

// ハンドラー実装
void FileRouter::handleMainPage(EthernetClient& client, const HttpRequestParser::ParsedRequest& request) {
    // 簡単なメインページを動的生成（実際のプロジェクトではindex.htmlを配信）
    String htmlContent = "<!DOCTYPE HTML>\n"
                        "<html>\n"
                        "<head><title>GPS NTP Server</title></head>\n"
                        "<body>\n"
                        "<h1>GPS NTP Server</h1>\n"
                        "<p>Status: Running</p>\n"
                        "<p><a href=\"/gps\">GPS Status</a> | "
                        "<a href=\"/config\">Configuration</a> | "
                        "<a href=\"/metrics\">Metrics</a></p>\n"
                        "</body>\n"
                        "</html>";
    
    HttpResponseBuilder builder(client);
    builder.html(htmlContent).send();
}

void FileRouter::handleGpsPage(EthernetClient& client, const HttpRequestParser::ParsedRequest& request) {
    handleFileRequest(client, "/gps.html", "text/html", false, 0);
}

void FileRouter::handleGpsScript(EthernetClient& client, const HttpRequestParser::ParsedRequest& request) {
    handleFileRequest(client, "/gps.js", "text/javascript", true, 3600);
}

void FileRouter::handleConfigPage(EthernetClient& client, const HttpRequestParser::ParsedRequest& request) {
    handleFileRequest(client, "/config.html", "text/html", false, 0);
}

void FileRouter::handleConfigScript(EthernetClient& client, const HttpRequestParser::ParsedRequest& request) {
    handleFileRequest(client, "/config.js", "text/javascript", true, 3600);
}

void FileRouter::handleMetricsPage(EthernetClient& client, const HttpRequestParser::ParsedRequest& request) {
    // PrometheusMetricsからメトリクスを取得（実装されている場合）
    HttpResponseBuilder builder(client);
    builder.setStatus(HttpResponseBuilder::OK)
           .setContentType("text/plain; version=0.0.4; charset=utf-8")
           .addHeader("Connection", "close")
           .addHeader("Cache-Control", "no-cache")
           .setBody("# GPS NTP Server Metrics\n# TYPE gps_satellites gauge\ngps_satellites 0\n")
           .send();
}

void FileRouter::handleFileRequest(EthernetClient& client, 
                                  const String& filepath,
                                  const String& mimeType,
                                  bool cacheEnabled,
                                  int cacheDuration) {
    // LittleFSの初期化
    if (!LittleFS.begin()) {
        if (instance_ && instance_->loggingService_) {
            instance_->loggingService_->error("FILE", "Failed to mount LittleFS");
        }
        HttpResponseBuilder::send404(client);
        return;
    }

    // ファイル存在チェック
    if (!fileExists(filepath)) {
        if (instance_ && instance_->loggingService_) {
            instance_->loggingService_->error("FILE", ("File not found: " + filepath).c_str());
        }
        HttpResponseBuilder::send404(client);
        return;
    }

    // ファイルオープン
    File file = LittleFS.open(filepath, "r");
    if (!file) {
        HttpResponseBuilder::send404(client);
        return;
    }

    // レスポンスビルダー作成
    HttpResponseBuilder builder(client);
    builder.setStatus(HttpResponseBuilder::OK)
           .setContentType(mimeType)
           .addHeader("Connection", "close")
           .addSecurityHeaders();

    // キャッシュヘッダー追加
    if (cacheEnabled && cacheDuration > 0) {
        String cacheHeaders = generateCacheHeaders(cacheDuration);
        builder.addHeader("Cache-Control", "public, max-age=" + String(cacheDuration));
    } else {
        builder.addNoCacheHeaders();
    }

    // ヘッダー送信
    builder.send();

    // ファイル内容を分割送信
    const size_t bufferSize = 512;
    uint8_t buffer[bufferSize];
    while (file.available()) {
        size_t bytesRead = file.read(buffer, bufferSize);
        client.write(buffer, bytesRead);
    }

    file.close();

    if (instance_ && instance_->loggingService_) {
        instance_->loggingService_->info("FILE", ("Served file: " + filepath).c_str());
    }
}

String FileRouter::getMimeType(const String& filepath) {
    if (filepath.endsWith(".html")) return "text/html";
    if (filepath.endsWith(".css")) return "text/css";
    if (filepath.endsWith(".js")) return "text/javascript";
    if (filepath.endsWith(".json")) return "application/json";
    if (filepath.endsWith(".png")) return "image/png";
    if (filepath.endsWith(".jpg") || filepath.endsWith(".jpeg")) return "image/jpeg";
    if (filepath.endsWith(".gif")) return "image/gif";
    if (filepath.endsWith(".ico")) return "image/x-icon";
    if (filepath.endsWith(".svg")) return "image/svg+xml";
    if (filepath.endsWith(".txt")) return "text/plain";
    if (filepath.endsWith(".xml")) return "text/xml";
    return "text/plain";
}

size_t FileRouter::getFileSize(const String& filepath) {
    if (!LittleFS.begin()) return 0;
    
    File file = LittleFS.open(filepath, "r");
    if (!file) return 0;
    
    size_t size = file.size();
    file.close();
    return size;
}

bool FileRouter::fileExists(const String& filepath) {
    if (!LittleFS.begin()) return false;
    return LittleFS.exists(filepath);
}

String FileRouter::generateCacheHeaders(int cacheDuration) {
    return "public, max-age=" + String(cacheDuration);
}
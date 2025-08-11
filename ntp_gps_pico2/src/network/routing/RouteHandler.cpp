#include "RouteHandler.h"

RouteHandler::RouteHandler() : routeCount_(0) {
    // 初期化
    for (int i = 0; i < MAX_ROUTES; i++) {
        routes_[i].pattern = "";
        routes_[i].method = GET;
        routes_[i].handler = nullptr;
        routes_[i].priority = 100;
        routes_[i].enabled = false;
    }
}

bool RouteHandler::addRoute(const String& pattern, HttpMethod method, HandlerFunction handler, int priority) {
    if (routeCount_ >= MAX_ROUTES || handler == nullptr) {
        return false;
    }

    routes_[routeCount_].pattern = pattern;
    routes_[routeCount_].method = method;
    routes_[routeCount_].handler = handler;
    routes_[routeCount_].priority = priority;
    routes_[routeCount_].enabled = true;
    
    routeCount_++;
    sortRoutesByPriority();
    
    return true;
}

bool RouteHandler::addGetRoute(const String& pattern, HandlerFunction handler, int priority) {
    return addRoute(pattern, GET, handler, priority);
}

bool RouteHandler::addPostRoute(const String& pattern, HandlerFunction handler, int priority) {
    return addRoute(pattern, POST, handler, priority);
}

bool RouteHandler::disableRoute(const String& pattern, HttpMethod method) {
    for (int i = 0; i < routeCount_; i++) {
        if (routes_[i].pattern.equals(pattern) && 
            (routes_[i].method == method || method == ANY)) {
            routes_[i].enabled = false;
            return true;
        }
    }
    return false;
}

bool RouteHandler::enableRoute(const String& pattern, HttpMethod method) {
    for (int i = 0; i < routeCount_; i++) {
        if (routes_[i].pattern.equals(pattern) && 
            (routes_[i].method == method || method == ANY)) {
            routes_[i].enabled = true;
            return true;
        }
    }
    return false;
}

bool RouteHandler::route(EthernetClient& client, const HttpRequestParser::ParsedRequest& request) {
    if (!request.isValid) {
        HttpResponseBuilder::send404(client);
        return false;
    }

    HttpMethod requestMethod = stringToMethod(request.method);
    
    // 優先度順にルートをチェック
    for (int i = 0; i < routeCount_; i++) {
        const Route& route = routes_[i];
        
        if (!route.enabled || route.handler == nullptr) {
            continue;
        }

        // メソッドマッチング
        if (route.method != ANY && route.method != requestMethod) {
            continue;
        }

        // パターンマッチング
        if (matchPattern(route.pattern, request.path)) {
            route.handler(client, request);
            return true;
        }
    }

    // マッチするルートが見つからない場合は404を返す
    HttpResponseBuilder::send404(client);
    return false;
}

int RouteHandler::getRouteCount() const {
    return routeCount_;
}

void RouteHandler::clearRoutes() {
    routeCount_ = 0;
    for (int i = 0; i < MAX_ROUTES; i++) {
        routes_[i].pattern = "";
        routes_[i].handler = nullptr;
        routes_[i].enabled = false;
    }
}

RouteHandler::HttpMethod RouteHandler::stringToMethod(const String& methodStr) {
    if (methodStr.equalsIgnoreCase("GET")) return GET;
    if (methodStr.equalsIgnoreCase("POST")) return POST;
    if (methodStr.equalsIgnoreCase("PUT")) return PUT;
    if (methodStr.equalsIgnoreCase("DELETE")) return DELETE;
    if (methodStr.equalsIgnoreCase("OPTIONS")) return OPTIONS;
    return GET; // デフォルト
}

bool RouteHandler::matchPattern(const String& pattern, const String& path) {
    // 完全一致
    if (pattern.equals(path)) {
        return true;
    }

    // ワイルドカード処理
    if (pattern.endsWith("*")) {
        String prefix = pattern.substring(0, pattern.length() - 1);
        return path.startsWith(prefix);
    }

    // パス区切りを考慮したワイルドカード
    if (pattern.endsWith("/*")) {
        String prefix = pattern.substring(0, pattern.length() - 2);
        return path.startsWith(prefix) && 
               (path.length() == prefix.length() || path.charAt(prefix.length()) == '/');
    }

    return false;
}

void RouteHandler::sortRoutesByPriority() {
    // 単純なバブルソート（ルート数が少ないため）
    for (int i = 0; i < routeCount_ - 1; i++) {
        for (int j = 0; j < routeCount_ - 1 - i; j++) {
            if (routes_[j].priority > routes_[j + 1].priority) {
                // スワップ
                Route temp = routes_[j];
                routes_[j] = routes_[j + 1];
                routes_[j + 1] = temp;
            }
        }
    }
}
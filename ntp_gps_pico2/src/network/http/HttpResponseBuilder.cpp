#include "HttpResponseBuilder.h"

HttpResponseBuilder::HttpResponseBuilder(EthernetClient& client) 
    : client_(client)
    , statusCode_(OK)
    , contentType_("text/html")
    , securityHeaders_(false)
    , noCacheHeaders_(false) {
}

HttpResponseBuilder& HttpResponseBuilder::setStatus(StatusCode statusCode) {
    statusCode_ = statusCode;
    return *this;
}

HttpResponseBuilder& HttpResponseBuilder::setContentType(const String& contentType) {
    contentType_ = contentType;
    return *this;
}

HttpResponseBuilder& HttpResponseBuilder::addHeader(const String& name, const String& value) {
    customHeaders_ += name + ": " + value + "\r\n";
    return *this;
}

HttpResponseBuilder& HttpResponseBuilder::addSecurityHeaders() {
    securityHeaders_ = true;
    return *this;
}

HttpResponseBuilder& HttpResponseBuilder::addNoCacheHeaders() {
    noCacheHeaders_ = true;
    return *this;
}

HttpResponseBuilder& HttpResponseBuilder::setBody(const String& body) {
    body_ = body;
    return *this;
}

HttpResponseBuilder& HttpResponseBuilder::json(const String& jsonBody, StatusCode statusCode) {
    return setStatus(statusCode)
           .setContentType("application/json")
           .addSecurityHeaders()
           .addNoCacheHeaders()
           .setBody(jsonBody);
}

HttpResponseBuilder& HttpResponseBuilder::html(const String& htmlBody, StatusCode statusCode) {
    return setStatus(statusCode)
           .setContentType("text/html")
           .addSecurityHeaders()
           .addNoCacheHeaders()
           .setBody(htmlBody);
}

void HttpResponseBuilder::send() {
    // HTTPレスポンスライン
    client_.print("HTTP/1.1 ");
    client_.print(static_cast<int>(statusCode_));
    client_.print(" ");
    client_.println(getStatusText(statusCode_));

    // 基本ヘッダー
    client_.println("Content-Type: " + contentType_);
    client_.println("Connection: close");

    // セキュリティヘッダー
    if (securityHeaders_) {
        client_.println("X-Content-Type-Options: nosniff");
        client_.println("X-Frame-Options: DENY");
        client_.println("X-XSS-Protection: 1; mode=block");
    }

    // キャッシュ無効化ヘッダー
    if (noCacheHeaders_) {
        client_.println("Cache-Control: no-cache, no-store, must-revalidate");
        client_.println("Pragma: no-cache");
        client_.println("Expires: 0");
    }

    // カスタムヘッダー
    if (customHeaders_.length() > 0) {
        client_.print(customHeaders_);
    }

    // ヘッダーとボディの区切り
    client_.println();

    // ボディ
    if (body_.length() > 0) {
        client_.print(body_);
    }
}

void HttpResponseBuilder::send404(EthernetClient& client) {
    HttpResponseBuilder builder(client);
    builder.setStatus(NOT_FOUND)
           .setContentType("text/html")
           .setBody("<!DOCTYPE HTML>\n"
                   "<html><body>\n"
                   "<h1>404 Not Found</h1>\n"
                   "<p>The requested resource could not be found on this server.</p>\n"
                   "</body></html>")
           .send();
}

void HttpResponseBuilder::sendError(EthernetClient& client, StatusCode statusCode, const String& message) {
    HttpResponseBuilder builder(client);
    
    // HTMLエラーページの生成
    String errorBody = "<!DOCTYPE HTML>\n<html><body>\n<h1>" + 
                      String(static_cast<int>(statusCode)) + " " + 
                      getStatusText(statusCode) + "</h1>\n<p>" + 
                      message + "</p>\n</body></html>";

    builder.setStatus(statusCode)
           .setContentType("text/html")
           .setBody(errorBody)
           .send();
}

String HttpResponseBuilder::getStatusText(StatusCode statusCode) {
    switch (statusCode) {
        case OK: return "OK";
        case BAD_REQUEST: return "Bad Request";
        case NOT_FOUND: return "Not Found";
        case TOO_MANY_REQUESTS: return "Too Many Requests";
        case INTERNAL_SERVER_ERROR: return "Internal Server Error";
        default: return "Unknown";
    }
}
#include "HttpHeaders.h"

// HttpHeaders::Container implementation
String HttpHeaders::Container::get(const String& name) const {
    String normalizedName = normalizeName(name);
    
    for (int i = 0; i < headerCount_; i++) {
        if (headers_[i].used && normalizeName(headers_[i].name).equals(normalizedName)) {
            return headers_[i].value;
        }
    }
    
    return "";
}

void HttpHeaders::Container::set(const String& name, const String& value) {
    String normalizedName = normalizeName(name);
    
    // 既存のヘッダーを更新
    for (int i = 0; i < headerCount_; i++) {
        if (headers_[i].used && normalizeName(headers_[i].name).equals(normalizedName)) {
            headers_[i].value = value;
            return;
        }
    }
    
    // 新しいヘッダーを追加
    if (headerCount_ < MAX_HEADERS) {
        headers_[headerCount_].name = name;
        headers_[headerCount_].value = value;
        headers_[headerCount_].used = true;
        headerCount_++;
    }
}

bool HttpHeaders::Container::has(const String& name) const {
    String normalizedName = normalizeName(name);
    
    for (int i = 0; i < headerCount_; i++) {
        if (headers_[i].used && normalizeName(headers_[i].name).equals(normalizedName)) {
            return true;
        }
    }
    
    return false;
}

void HttpHeaders::Container::clear() {
    for (int i = 0; i < MAX_HEADERS; i++) {
        headers_[i].name = "";
        headers_[i].value = "";
        headers_[i].used = false;
    }
    headerCount_ = 0;
}

int HttpHeaders::Container::count() const {
    return headerCount_;
}

String HttpHeaders::Container::normalizeName(const String& name) const {
    String normalized = name;
    normalized.toLowerCase();
    return normalized;
}

// HttpHeaders static methods
HttpHeaders::Container HttpHeaders::parse(const String& rawHeaders) {
    Container headers;
    headers.clear();

    int startIndex = 0;
    int endIndex;

    while ((endIndex = rawHeaders.indexOf('\n', startIndex)) != -1) {
        String headerLine = rawHeaders.substring(startIndex, endIndex);
        headerLine.trim();

        if (headerLine.length() > 0) {
            String name, value;
            if (parseHeaderLine(headerLine, name, value)) {
                headers.set(name, value);
            }
        }

        startIndex = endIndex + 1;
    }

    // 最後の行を処理
    if (startIndex < rawHeaders.length()) {
        String headerLine = rawHeaders.substring(startIndex);
        headerLine.trim();
        
        if (headerLine.length() > 0) {
            String name, value;
            if (parseHeaderLine(headerLine, name, value)) {
                headers.set(name, value);
            }
        }
    }

    return headers;
}

String HttpHeaders::generateSecurityHeaders() {
    return "X-Content-Type-Options: nosniff\r\n"
           "X-Frame-Options: DENY\r\n"
           "X-XSS-Protection: 1; mode=block\r\n";
}

String HttpHeaders::generateNoCacheHeaders() {
    return "Cache-Control: no-cache, no-store, must-revalidate\r\n"
           "Pragma: no-cache\r\n"
           "Expires: 0\r\n";
}

String HttpHeaders::generateJsonHeaders() {
    return "Content-Type: application/json\r\n"
           "Connection: close\r\n" +
           generateSecurityHeaders() +
           generateNoCacheHeaders();
}

String HttpHeaders::generateHtmlHeaders() {
    return "Content-Type: text/html\r\n"
           "Connection: close\r\n" +
           generateSecurityHeaders() +
           generateNoCacheHeaders();
}

String HttpHeaders::generateTextHeaders(const String& contentType) {
    return "Content-Type: " + contentType + "\r\n"
           "Connection: close\r\n" +
           generateSecurityHeaders() +
           generateNoCacheHeaders();
}

String HttpHeaders::generateCorsHeaders(const String& allowedOrigins, const String& allowedMethods) {
    return "Access-Control-Allow-Origin: " + allowedOrigins + "\r\n"
           "Access-Control-Allow-Methods: " + allowedMethods + "\r\n"
           "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
           "Access-Control-Max-Age: 3600\r\n";
}

bool HttpHeaders::parseHeaderLine(const String& headerLine, String& name, String& value) {
    int colonIndex = headerLine.indexOf(':');
    
    if (colonIndex <= 0 || colonIndex >= headerLine.length() - 1) {
        return false;
    }

    name = headerLine.substring(0, colonIndex);
    name.trim();
    
    value = headerLine.substring(colonIndex + 1);
    value.trim();

    return name.length() > 0;
}
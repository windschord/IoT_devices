#include "HttpRequestParser.h"

HttpRequestParser::ParsedRequest HttpRequestParser::parseRequest(const String& rawRequest) {
    ParsedRequest request;
    request.isValid = false;
    request.contentLength = 0;

    if (rawRequest.length() == 0) {
        return request;
    }

    // リクエストラインを抽出
    int firstNewline = rawRequest.indexOf('\n');
    if (firstNewline <= 0) {
        return request;
    }

    String requestLine = rawRequest.substring(0, firstNewline);
    requestLine.trim();

    // リクエストラインを解析
    if (!parseRequestLine(requestLine, request.method, request.path, 
                         request.queryString, request.httpVersion)) {
        return request;
    }

    // Content-Lengthを解析
    request.contentLength = parseContentLength(rawRequest);

    // ボディを抽出
    if (request.contentLength > 0) {
        request.body = extractBody(rawRequest, request.contentLength);
    }

    request.isValid = true;
    return request;
}

bool HttpRequestParser::parseRequestLine(const String& requestLine,
                                        String& method,
                                        String& path,
                                        String& queryString,
                                        String& httpVersion) {
    // "GET /path HTTP/1.1" 形式の解析
    int firstSpace = requestLine.indexOf(' ');
    int lastSpace = requestLine.lastIndexOf(' ');

    if (firstSpace <= 0 || lastSpace <= firstSpace) {
        return false;
    }

    method = requestLine.substring(0, firstSpace);
    String url = requestLine.substring(firstSpace + 1, lastSpace);
    httpVersion = requestLine.substring(lastSpace + 1);

    // URLをパスとクエリ文字列に分割
    splitUrl(url, path, queryString);

    return true;
}

int HttpRequestParser::parseContentLength(const String& headers) {
    int contentLength = 0;

    // "Content-Length: " を検索（複数の書式に対応）
    int contentLengthIndex = headers.indexOf("Content-Length:");
    if (contentLengthIndex < 0) {
        contentLengthIndex = headers.indexOf("content-length:");
    }

    if (contentLengthIndex >= 0) {
        int lineEnd = headers.indexOf('\r', contentLengthIndex);
        if (lineEnd < 0) {
            lineEnd = headers.indexOf('\n', contentLengthIndex);
        }
        
        if (lineEnd > contentLengthIndex) {
            String lengthStr = headers.substring(contentLengthIndex + 15, lineEnd); // "Content-Length:" is 15 chars
            lengthStr.trim();
            contentLength = lengthStr.toInt();
        }
    }

    return contentLength;
}

String HttpRequestParser::extractBody(const String& rawRequest, int contentLength) {
    // ヘッダーとボディの境界を検索（複数パターンに対応）
    int contentStart = rawRequest.indexOf("\r\n\r\n");
    if (contentStart >= 0) {
        contentStart += 4; // "\r\n\r\n" は 4 文字
    } else {
        contentStart = rawRequest.indexOf("\n\n");
        if (contentStart >= 0) {
            contentStart += 2; // "\n\n" は 2 文字
        }
    }

    if (contentStart < 0 || contentLength <= 0) {
        return "";
    }

    // 指定された長さのボディを抽出
    int availableLength = rawRequest.length() - contentStart;
    int extractLength = (contentLength <= availableLength) ? contentLength : availableLength;

    return rawRequest.substring(contentStart, contentStart + extractLength);
}

void HttpRequestParser::splitUrl(const String& url, String& path, String& queryString) {
    int queryStart = url.indexOf('?');
    
    if (queryStart >= 0) {
        path = url.substring(0, queryStart);
        queryString = url.substring(queryStart + 1);
    } else {
        path = url;
        queryString = "";
    }
}
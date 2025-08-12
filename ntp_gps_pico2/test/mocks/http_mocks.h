#pragma once

#include "../arduino_mock.h"
#include "../../src/network/http/HttpRequestParser.h"
#include "../../src/network/http/HttpResponseBuilder.h"
#include "../../src/network/http/HttpHeaders.h"
#include "../../src/network/routing/RouteHandler.h"
#include "../../src/network/routing/ApiRouter.h"
#include "../../src/network/routing/FileRouter.h"
#include "../../src/network/filesystem/FileSystemHandler.h"
#include "../../src/network/filesystem/MimeTypeResolver.h"
#include "../../src/network/filesystem/CacheManager.h"
#include "../../src/system/Result.h"

/**
 * @file http_mocks.h
 * @brief Mock classes for the HTTP processing refactored classes
 * 
 * This file provides mock implementations for testing the HTTP processing
 * classes created during webserver.cpp refactoring.
 */

// ========== Mock HTTP Request Parser ==========

class MockHttpRequestParser {
public:
    mutable bool parseCalled = false;
    mutable bool isValidRequest = true;
    mutable const char* mockMethod = "GET";
    mutable const char* mockPath = "/";
    mutable const char* mockVersion = "HTTP/1.1";
    mutable const char* mockHeaders = "Host: localhost\r\n";
    mutable const char* mockBody = "";
    mutable size_t mockContentLength = 0;

    struct MockHttpRequest {
        const char* method = "GET";
        const char* path = "/";
        const char* version = "HTTP/1.1";
        const char* headers = "Host: localhost\r\n";
        const char* body = "";
        size_t contentLength = 0;
        bool valid = true;

        bool isValid() const { return valid; }
        const char* getMethod() const { return method; }
        const char* getPath() const { return path; }
        const char* getVersion() const { return version; }
        const char* getHeader(const char* name) const { 
            if (strcmp(name, "Host") == 0) return "localhost";
            if (strcmp(name, "Content-Length") == 0) return "0";
            return nullptr;
        }
        const char* getBody() const { return body; }
        size_t getContentLength() const { return contentLength; }
    };

    MockHttpRequest parse(const String& requestData) {
        parseCalled = true;
        MockHttpRequest request;
        
        request.method = mockMethod;
        request.path = mockPath;
        request.version = mockVersion;
        request.headers = mockHeaders;
        request.body = mockBody;
        request.contentLength = mockContentLength;
        request.valid = isValidRequest;
        
        return request;
    }

    void setMockRequest(const char* method, const char* path, const char* body = "", size_t contentLen = 0) {
        mockMethod = method;
        mockPath = path;
        mockBody = body;
        mockContentLength = contentLen;
    }

    void reset() {
        parseCalled = false;
        isValidRequest = true;
        mockMethod = "GET";
        mockPath = "/";
        mockBody = "";
        mockContentLength = 0;
    }
};

// ========== Mock HTTP Response Builder ==========

class MockHttpResponseBuilder {
public:
    mutable bool buildResponseCalled = false;
    mutable bool setStatusCalled = false;
    mutable bool setHeaderCalled = false;
    mutable bool setBodyCalled = false;
    mutable int mockStatusCode = 200;
    mutable const char* mockStatusMessage = "OK";
    mutable const char* mockHeaders = "Content-Type: text/html\r\n";
    mutable const char* mockBody = "<html><body>Mock Response</body></html>";

    struct MockHttpResponse {
        int statusCode = 200;
        const char* statusMessage = "OK";
        const char* headers = "Content-Type: text/html\r\n";
        const char* body = "<html><body>Mock Response</body></html>";
        size_t contentLength = 0;

        MockHttpResponse(int code = 200, const char* message = "OK", 
                        const char* h = "", const char* b = "") 
            : statusCode(code), statusMessage(message), headers(h), body(b) {
            contentLength = strlen(body);
        }

        String toString() const {
            String response;
            response += "HTTP/1.1 ";
            response += String(statusCode);
            response += " ";
            response += statusMessage;
            response += "\r\n";
            response += headers;
            response += "Content-Length: ";
            response += String(contentLength);
            response += "\r\n\r\n";
            response += body;
            return response;
        }
    };

    MockHttpResponse buildResponse(int statusCode, const char* body, const char* contentType = "text/html") {
        buildResponseCalled = true;
        return MockHttpResponse(statusCode, getStatusMessage(statusCode), 
                              buildHeaders(contentType).c_str(), body);
    }

    MockHttpResponse buildJsonResponse(const char* json) {
        return buildResponse(200, json, "application/json");
    }

    MockHttpResponse buildErrorResponse(int statusCode, const char* message) {
        return buildResponse(statusCode, message, "text/plain");
    }

    void setStatus(int code, const char* message) {
        setStatusCalled = true;
        mockStatusCode = code;
        mockStatusMessage = message;
    }

    void setHeader(const char* name, const char* value) {
        setHeaderCalled = true;
    }

    void setBody(const char* body) {
        setBodyCalled = true;
        mockBody = body;
    }

    void reset() {
        buildResponseCalled = false;
        setStatusCalled = false;
        setHeaderCalled = false;
        setBodyCalled = false;
        mockStatusCode = 200;
        mockStatusMessage = "OK";
        mockBody = "<html><body>Mock Response</body></html>";
    }

private:
    const char* getStatusMessage(int code) const {
        switch (code) {
            case 200: return "OK";
            case 404: return "Not Found";
            case 500: return "Internal Server Error";
            case 400: return "Bad Request";
            default: return "Unknown";
        }
    }

    String buildHeaders(const char* contentType) const {
        String headers;
        headers += "Content-Type: ";
        headers += contentType;
        headers += "\r\n";
        headers += "Cache-Control: no-cache\r\n";
        headers += "Connection: close\r\n";
        return headers;
    }
};

// ========== Mock Route Handler ==========

class MockRouteHandler {
public:
    mutable bool addRouteCalled = false;
    mutable bool handleRequestCalled = false;
    mutable bool routeMatched = true;
    mutable const char* matchedRoute = "/test";
    mutable int routeCount = 0;

    struct MockRoute {
        const char* pattern;
        const char* method;
        int priority;
        bool (*handler)(const char* path);
    };

    static bool mockHandler(const char* path) { return true; }

    bool addRoute(const char* pattern, const char* method, int priority, 
                 bool (*handler)(const char* path)) {
        addRouteCalled = true;
        routeCount++;
        return true;
    }

    MockHttpResponseBuilder::MockHttpResponse handleRequest(const MockHttpRequestParser::MockHttpRequest& request) {
        handleRequestCalled = true;
        
        if (routeMatched) {
            return MockHttpResponseBuilder::MockHttpResponse(200, "OK", "", "Route handled successfully");
        } else {
            return MockHttpResponseBuilder::MockHttpResponse(404, "Not Found", "", "Route not found");
        }
    }

    bool matchesRoute(const char* path, const char* method) const {
        return routeMatched && strcmp(path, matchedRoute) == 0;
    }

    int getRouteCount() const { return routeCount; }

    void reset() {
        addRouteCalled = false;
        handleRequestCalled = false;
        routeMatched = true;
        routeCount = 0;
    }
};

// ========== Mock API Router ==========

class MockApiRouter {
public:
    mutable bool setupApiRoutesCalled = false;
    mutable bool handleApiRequestCalled = false;
    mutable bool isApiPath = true;
    mutable const char* mockApiResponse = "{\"status\": \"ok\", \"message\": \"Mock API response\"}";

    void setupApiRoutes() {
        setupApiRoutesCalled = true;
    }

    MockHttpResponseBuilder::MockHttpResponse handleApiRequest(const MockHttpRequestParser::MockHttpRequest& request) {
        handleApiRequestCalled = true;
        
        if (isApiPath) {
            return MockHttpResponseBuilder::MockHttpResponse(200, "OK", 
                "Content-Type: application/json\r\n", mockApiResponse);
        } else {
            return MockHttpResponseBuilder::MockHttpResponse(404, "Not Found", "", "API endpoint not found");
        }
    }

    bool isApiRequest(const char* path) const {
        return isApiPath && strncmp(path, "/api/", 5) == 0;
    }

    void setMockApiResponse(const char* response) {
        mockApiResponse = response;
    }

    void reset() {
        setupApiRoutesCalled = false;
        handleApiRequestCalled = false;
        isApiPath = true;
        mockApiResponse = "{\"status\": \"ok\", \"message\": \"Mock API response\"}";
    }
};

// ========== Mock File Router ==========

class MockFileRouter {
public:
    mutable bool setupFileRoutesCalled = false;
    mutable bool handleFileRequestCalled = false;
    mutable bool fileExists = true;
    mutable const char* mockFileContent = "<html><body>Mock File Content</body></html>";
    mutable const char* mockMimeType = "text/html";

    void setupFileRoutes() {
        setupFileRoutesCalled = true;
    }

    MockHttpResponseBuilder::MockHttpResponse handleFileRequest(const MockHttpRequestParser::MockHttpRequest& request) {
        handleFileRequestCalled = true;
        
        if (fileExists) {
            String headers;
            headers += "Content-Type: ";
            headers += mockMimeType;
            headers += "\r\n";
            return MockHttpResponseBuilder::MockHttpResponse(200, "OK", 
                headers.c_str(), mockFileContent);
        } else {
            return MockHttpResponseBuilder::MockHttpResponse(404, "Not Found", "", "File not found");
        }
    }

    bool isStaticFile(const char* path) const {
        return strstr(path, ".html") != nullptr || 
               strstr(path, ".css") != nullptr || 
               strstr(path, ".js") != nullptr;
    }

    void setMockFile(const char* content, const char* mimeType, bool exists = true) {
        mockFileContent = content;
        mockMimeType = mimeType;
        fileExists = exists;
    }

    void reset() {
        setupFileRoutesCalled = false;
        handleFileRequestCalled = false;
        fileExists = true;
        mockFileContent = "<html><body>Mock File Content</body></html>";
        mockMimeType = "text/html";
    }
};

// ========== Mock File System Handler ==========

class MockFileSystemHandler {
public:
    mutable bool readFileCalled = false;
    mutable bool fileExistsCalled = false;
    mutable bool getFileSizeCalled = false;
    mutable bool mockFileExists = true;
    mutable size_t mockFileSize = 1024;
    mutable const char* mockFileContent = "Mock file content";

    Result<String, ErrorType> readFile(const char* path) {
        readFileCalled = true;
        
        if (mockFileExists) {
            return Result<String, ErrorType>::ok(String(mockFileContent));
        } else {
            return Result<String, ErrorType>::error(ErrorType::SYSTEM_ERROR);
        }
    }

    bool fileExists(const char* path) {
        fileExistsCalled = true;
        return mockFileExists;
    }

    size_t getFileSize(const char* path) {
        getFileSizeCalled = true;
        return mockFileExists ? mockFileSize : 0;
    }

    void setMockFile(const char* content, size_t size, bool exists = true) {
        mockFileContent = content;
        mockFileSize = size;
        mockFileExists = exists;
    }

    void reset() {
        readFileCalled = false;
        fileExistsCalled = false;
        getFileSizeCalled = false;
        mockFileExists = true;
        mockFileSize = 1024;
        mockFileContent = "Mock file content";
    }
};

// ========== Mock MIME Type Resolver ==========

class MockMimeTypeResolver {
public:
    mutable bool getMimeTypeCalled = false;
    mutable const char* mockMimeType = "text/html";

    const char* getMimeType(const char* filename) {
        getMimeTypeCalled = true;
        
        if (strstr(filename, ".html")) return "text/html";
        if (strstr(filename, ".css")) return "text/css";
        if (strstr(filename, ".js")) return "application/javascript";
        if (strstr(filename, ".json")) return "application/json";
        if (strstr(filename, ".png")) return "image/png";
        if (strstr(filename, ".jpg") || strstr(filename, ".jpeg")) return "image/jpeg";
        
        return mockMimeType;
    }

    void setMockMimeType(const char* mimeType) {
        mockMimeType = mimeType;
    }

    void reset() {
        getMimeTypeCalled = false;
        mockMimeType = "text/html";
    }
};

// ========== Mock Cache Manager ==========

class MockCacheManager {
public:
    mutable bool getCachedResponseCalled = false;
    mutable bool cacheResponseCalled = false;
    mutable bool clearCacheCalled = false;
    mutable bool hasCachedResponse = false;
    mutable const char* cachedContent = "Cached response";

    struct MockCacheEntry {
        const char* content;
        size_t size;
        unsigned long timestamp;
        const char* etag;
        
        MockCacheEntry(const char* c = "", size_t s = 0, unsigned long t = 0, const char* e = "")
            : content(c), size(s), timestamp(t), etag(e) {}
    };

    Result<MockCacheEntry, ErrorType> getCachedResponse(const char* path) {
        getCachedResponseCalled = true;
        
        if (hasCachedResponse) {
            return Result<MockCacheEntry, ErrorType>::ok(
                MockCacheEntry(cachedContent, strlen(cachedContent), millis(), "mock-etag"));
        } else {
            return Result<MockCacheEntry, ErrorType>::error(ErrorType::SYSTEM_ERROR);
        }
    }

    bool cacheResponse(const char* path, const char* content, const char* etag) {
        cacheResponseCalled = true;
        cachedContent = content;
        hasCachedResponse = true;
        return true;
    }

    void clearCache() {
        clearCacheCalled = true;
        hasCachedResponse = false;
        cachedContent = "";
    }

    bool isCached(const char* path) const {
        return hasCachedResponse;
    }

    void setCachedResponse(const char* content, bool cached = true) {
        cachedContent = content;
        hasCachedResponse = cached;
    }

    void reset() {
        getCachedResponseCalled = false;
        cacheResponseCalled = false;
        clearCacheCalled = false;
        hasCachedResponse = false;
        cachedContent = "Cached response";
    }
};

// ========== HTTP Test Data Manager ==========

class HttpTestDataManager {
public:
    // Common HTTP test requests
    static const char* GET_ROOT_REQUEST;
    static const char* GET_API_STATUS_REQUEST;
    static const char* POST_CONFIG_REQUEST;
    static const char* GET_NONEXISTENT_REQUEST;
    static const char* MALFORMED_REQUEST;

    // Common HTTP test responses
    static const char* OK_RESPONSE;
    static const char* NOT_FOUND_RESPONSE;
    static const char* JSON_API_RESPONSE;
    static const char* ERROR_RESPONSE;

    static HttpTestDataManager& getInstance() {
        static HttpTestDataManager instance;
        return instance;
    }

    void reset() {
        // Reset any internal state if needed
    }
};

// Static test data definitions
const char* HttpTestDataManager::GET_ROOT_REQUEST = 
    "GET / HTTP/1.1\r\nHost: localhost\r\nUser-Agent: Test\r\n\r\n";

const char* HttpTestDataManager::GET_API_STATUS_REQUEST = 
    "GET /api/status HTTP/1.1\r\nHost: localhost\r\nAccept: application/json\r\n\r\n";

const char* HttpTestDataManager::POST_CONFIG_REQUEST = 
    "POST /api/config HTTP/1.1\r\nHost: localhost\r\nContent-Type: application/json\r\nContent-Length: 25\r\n\r\n{\"setting\":\"test_value\"}";

const char* HttpTestDataManager::GET_NONEXISTENT_REQUEST = 
    "GET /nonexistent HTTP/1.1\r\nHost: localhost\r\n\r\n";

const char* HttpTestDataManager::MALFORMED_REQUEST = 
    "INVALID REQUEST FORMAT";

const char* HttpTestDataManager::OK_RESPONSE = 
    "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 13\r\n\r\nTest response";

const char* HttpTestDataManager::NOT_FOUND_RESPONSE = 
    "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: 9\r\n\r\nNot found";

const char* HttpTestDataManager::JSON_API_RESPONSE = 
    "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: 25\r\n\r\n{\"status\":\"ok\",\"data\":{}}";

const char* HttpTestDataManager::ERROR_RESPONSE = 
    "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\nContent-Length: 21\r\n\r\nInternal server error";

// ========== HTTP Mock Test Helper ==========

class HttpMockTestHelper {
public:
    static void setupHttpMocks() {
        // Initialize all HTTP mocks to default state
    }

    static void teardownHttpMocks() {
        HttpTestDataManager::getInstance().reset();
    }

    static MockHttpRequestParser::MockHttpRequest createMockRequest(
        const char* method = "GET", 
        const char* path = "/", 
        const char* body = "",
        size_t contentLength = 0) {
        
        MockHttpRequestParser::MockHttpRequest request;
        request.method = method;
        request.path = path;
        request.body = body;
        request.contentLength = contentLength;
        request.valid = true;
        return request;
    }

    static MockHttpResponseBuilder::MockHttpResponse createMockResponse(
        int statusCode = 200, 
        const char* body = "Test response", 
        const char* contentType = "text/html") {
        
        return MockHttpResponseBuilder::MockHttpResponse(
            statusCode, 
            statusCode == 200 ? "OK" : "Error", 
            "", 
            body
        );
    }
};

// Global HTTP mock instances for dependency injection
extern MockHttpRequestParser* g_mockHttpRequestParser;
extern MockHttpResponseBuilder* g_mockHttpResponseBuilder;
extern MockRouteHandler* g_mockRouteHandler;
extern MockApiRouter* g_mockApiRouter;
extern MockFileRouter* g_mockFileRouter;
extern MockFileSystemHandler* g_mockFileSystemHandler;
extern MockMimeTypeResolver* g_mockMimeTypeResolver;
extern MockCacheManager* g_mockCacheManager;

inline void initializeHttpMocks() {
    g_mockHttpRequestParser = new MockHttpRequestParser();
    g_mockHttpResponseBuilder = new MockHttpResponseBuilder();
    g_mockRouteHandler = new MockRouteHandler();
    g_mockApiRouter = new MockApiRouter();
    g_mockFileRouter = new MockFileRouter();
    g_mockFileSystemHandler = new MockFileSystemHandler();
    g_mockMimeTypeResolver = new MockMimeTypeResolver();
    g_mockCacheManager = new MockCacheManager();
}

inline void cleanupHttpMocks() {
    delete g_mockHttpRequestParser;
    delete g_mockHttpResponseBuilder;
    delete g_mockRouteHandler;
    delete g_mockApiRouter;
    delete g_mockFileRouter;
    delete g_mockFileSystemHandler;
    delete g_mockMimeTypeResolver;
    delete g_mockCacheManager;
    
    g_mockHttpRequestParser = nullptr;
    g_mockHttpResponseBuilder = nullptr;
    g_mockRouteHandler = nullptr;
    g_mockApiRouter = nullptr;
    g_mockFileRouter = nullptr;
    g_mockFileSystemHandler = nullptr;
    g_mockMimeTypeResolver = nullptr;
    g_mockCacheManager = nullptr;
}
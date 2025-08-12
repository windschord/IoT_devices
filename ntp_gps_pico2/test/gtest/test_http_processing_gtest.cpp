#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Include necessary mocks before the actual headers
#include "../mocks/system_mocks.h"
#include "../mocks/http_mocks.h"

// Mock the Arduino environment
#include "../arduino_mock.h"
#include "../test_common.h"

using ::testing::_;
using ::testing::Return;
using ::testing::StrEq;
using ::testing::InSequence;

/**
 * @file test_http_processing_gtest.cpp
 * @brief GoogleTest tests for HTTP processing classes
 * 
 * Tests the HTTP processing classes created during the webserver.cpp refactoring:
 * - HttpRequestParser
 * - HttpResponseBuilder
 * - RouteHandler
 * - ApiRouter
 * - FileRouter
 * - FileSystemHandler
 * - MimeTypeResolver
 * - CacheManager
 */

class HttpProcessingTest : public ::testing::Test {
protected:
    void SetUp() override {
        HttpMockTestHelper::setupHttpMocks();
        
        // Reset all HTTP mocks
        mockRequestParser.reset();
        mockResponseBuilder.reset();
        mockRouteHandler.reset();
        mockApiRouter.reset();
        mockFileRouter.reset();
        mockFileSystemHandler.reset();
        mockMimeTypeResolver.reset();
        mockCacheManager.reset();
    }

    void TearDown() override {
        HttpMockTestHelper::teardownHttpMocks();
    }

    // Mock instances for testing
    MockHttpRequestParser mockRequestParser;
    MockHttpResponseBuilder mockResponseBuilder;
    MockRouteHandler mockRouteHandler;
    MockApiRouter mockApiRouter;
    MockFileRouter mockFileRouter;
    MockFileSystemHandler mockFileSystemHandler;
    MockMimeTypeResolver mockMimeTypeResolver;
    MockCacheManager mockCacheManager;
};

// ========== HTTP Request Parser Tests ==========

TEST_F(HttpProcessingTest, HttpRequestParserBasicParsing) {
    // Test basic HTTP request parsing
    String testRequest = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
    
    mockRequestParser.setMockRequest("GET", "/", "", 0);
    auto request = mockRequestParser.parse(testRequest);
    
    EXPECT_TRUE(mockRequestParser.parseCalled);
    EXPECT_TRUE(request.isValid());
    EXPECT_STREQ(request.getMethod(), "GET");
    EXPECT_STREQ(request.getPath(), "/");
    EXPECT_STREQ(request.getVersion(), "HTTP/1.1");
}

TEST_F(HttpProcessingTest, HttpRequestParserPostWithBody) {
    // Test POST request parsing with body
    String testRequest = "POST /api/config HTTP/1.1\r\nContent-Length: 25\r\n\r\n{\"setting\":\"test_value\"}";
    
    mockRequestParser.setMockRequest("POST", "/api/config", "{\"setting\":\"test_value\"}", 25);
    auto request = mockRequestParser.parse(testRequest);
    
    EXPECT_TRUE(mockRequestParser.parseCalled);
    EXPECT_TRUE(request.isValid());
    EXPECT_STREQ(request.getMethod(), "POST");
    EXPECT_STREQ(request.getPath(), "/api/config");
    EXPECT_EQ(request.getContentLength(), 25);
    EXPECT_STREQ(request.getBody(), "{\"setting\":\"test_value\"}");
}

TEST_F(HttpProcessingTest, HttpRequestParserInvalidRequest) {
    // Test handling of invalid HTTP request
    String invalidRequest = "INVALID REQUEST FORMAT";
    
    mockRequestParser.isValidRequest = false;
    auto request = mockRequestParser.parse(invalidRequest);
    
    EXPECT_TRUE(mockRequestParser.parseCalled);
    EXPECT_FALSE(request.isValid());
}

// ========== HTTP Response Builder Tests ==========

TEST_F(HttpProcessingTest, HttpResponseBuilderBasicResponse) {
    // Test basic HTTP response building
    auto response = mockResponseBuilder.buildResponse(200, "Hello World", "text/plain");
    
    EXPECT_TRUE(mockResponseBuilder.buildResponseCalled);
    EXPECT_EQ(response.statusCode, 200);
    EXPECT_STREQ(response.statusMessage, "OK");
    EXPECT_STREQ(response.body, "Hello World");
}

TEST_F(HttpProcessingTest, HttpResponseBuilderJsonResponse) {
    // Test JSON response building
    const char* jsonData = "{\"status\":\"success\",\"data\":{\"value\":42}}";
    auto response = mockResponseBuilder.buildJsonResponse(jsonData);
    
    EXPECT_TRUE(mockResponseBuilder.buildResponseCalled);
    EXPECT_EQ(response.statusCode, 200);
    EXPECT_STREQ(response.body, jsonData);
}

TEST_F(HttpProcessingTest, HttpResponseBuilderErrorResponse) {
    // Test error response building
    auto response = mockResponseBuilder.buildErrorResponse(404, "Not Found");
    
    EXPECT_TRUE(mockResponseBuilder.buildResponseCalled);
    EXPECT_EQ(response.statusCode, 404);
    EXPECT_STREQ(response.statusMessage, "Not Found");
    EXPECT_STREQ(response.body, "Not Found");
}

TEST_F(HttpProcessingTest, HttpResponseBuilderToString) {
    // Test response serialization to string
    auto response = mockResponseBuilder.buildResponse(200, "Test", "text/html");
    String responseString = response.toString();
    
    EXPECT_TRUE(responseString.indexOf("HTTP/1.1 200 OK") >= 0);
    EXPECT_TRUE(responseString.indexOf("Content-Length:") >= 0);
    EXPECT_TRUE(responseString.indexOf("Test") >= 0);
}

// ========== Route Handler Tests ==========

TEST_F(HttpProcessingTest, RouteHandlerAddRoute) {
    // Test route registration
    bool result = mockRouteHandler.addRoute("/test", "GET", 1, MockRouteHandler::mockHandler);
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(mockRouteHandler.addRouteCalled);
    EXPECT_EQ(mockRouteHandler.getRouteCount(), 1);
}

TEST_F(HttpProcessingTest, RouteHandlerMatchRoute) {
    // Test route matching
    mockRouteHandler.routeMatched = true;
    mockRouteHandler.matchedRoute = "/test";
    
    bool matches = mockRouteHandler.matchesRoute("/test", "GET");
    
    EXPECT_TRUE(matches);
}

TEST_F(HttpProcessingTest, RouteHandlerHandleRequest) {
    // Test request handling through route handler
    auto request = HttpMockTestHelper::createMockRequest("GET", "/test");
    mockRouteHandler.routeMatched = true;
    
    auto response = mockRouteHandler.handleRequest(request);
    
    EXPECT_TRUE(mockRouteHandler.handleRequestCalled);
    EXPECT_EQ(response.statusCode, 200);
    EXPECT_STREQ(response.body, "Route handled successfully");
}

TEST_F(HttpProcessingTest, RouteHandlerRouteNotFound) {
    // Test handling when route is not found
    auto request = HttpMockTestHelper::createMockRequest("GET", "/nonexistent");
    mockRouteHandler.routeMatched = false;
    
    auto response = mockRouteHandler.handleRequest(request);
    
    EXPECT_TRUE(mockRouteHandler.handleRequestCalled);
    EXPECT_EQ(response.statusCode, 404);
    EXPECT_STREQ(response.body, "Route not found");
}

// ========== API Router Tests ==========

TEST_F(HttpProcessingTest, ApiRouterSetup) {
    // Test API routes setup
    mockApiRouter.setupApiRoutes();
    
    EXPECT_TRUE(mockApiRouter.setupApiRoutesCalled);
}

TEST_F(HttpProcessingTest, ApiRouterHandleApiRequest) {
    // Test API request handling
    auto request = HttpMockTestHelper::createMockRequest("GET", "/api/status");
    mockApiRouter.isApiPath = true;
    
    auto response = mockApiRouter.handleApiRequest(request);
    
    EXPECT_TRUE(mockApiRouter.handleApiRequestCalled);
    EXPECT_EQ(response.statusCode, 200);
    EXPECT_TRUE(strstr(response.body, "Mock API response") != nullptr);
}

TEST_F(HttpProcessingTest, ApiRouterIsApiRequest) {
    // Test API path detection
    mockApiRouter.isApiPath = true;
    bool isApi = mockApiRouter.isApiRequest("/api/status");
    
    EXPECT_TRUE(isApi);
    
    mockApiRouter.isApiPath = false;
    bool isNotApi = mockApiRouter.isApiRequest("/index.html");
    
    EXPECT_FALSE(isNotApi);
}

TEST_F(HttpProcessingTest, ApiRouterCustomResponse) {
    // Test custom API response
    const char* customResponse = "{\"custom\":\"data\",\"test\":true}";
    mockApiRouter.setMockApiResponse(customResponse);
    
    auto request = HttpMockTestHelper::createMockRequest("GET", "/api/test");
    auto response = mockApiRouter.handleApiRequest(request);
    
    EXPECT_STREQ(response.body, customResponse);
}

// ========== File Router Tests ==========

TEST_F(HttpProcessingTest, FileRouterSetup) {
    // Test file routes setup
    mockFileRouter.setupFileRoutes();
    
    EXPECT_TRUE(mockFileRouter.setupFileRoutesCalled);
}

TEST_F(HttpProcessingTest, FileRouterHandleFileRequest) {
    // Test file request handling
    auto request = HttpMockTestHelper::createMockRequest("GET", "/index.html");
    mockFileRouter.fileExists = true;
    
    auto response = mockFileRouter.handleFileRequest(request);
    
    EXPECT_TRUE(mockFileRouter.handleFileRequestCalled);
    EXPECT_EQ(response.statusCode, 200);
    EXPECT_STREQ(response.body, "<html><body>Mock File Content</body></html>");
}

TEST_F(HttpProcessingTest, FileRouterFileNotFound) {
    // Test file not found handling
    auto request = HttpMockTestHelper::createMockRequest("GET", "/nonexistent.html");
    mockFileRouter.fileExists = false;
    
    auto response = mockFileRouter.handleFileRequest(request);
    
    EXPECT_TRUE(mockFileRouter.handleFileRequestCalled);
    EXPECT_EQ(response.statusCode, 404);
    EXPECT_STREQ(response.body, "File not found");
}

TEST_F(HttpProcessingTest, FileRouterStaticFileDetection) {
    // Test static file detection
    EXPECT_TRUE(mockFileRouter.isStaticFile("index.html"));
    EXPECT_TRUE(mockFileRouter.isStaticFile("style.css"));
    EXPECT_TRUE(mockFileRouter.isStaticFile("script.js"));
    EXPECT_FALSE(mockFileRouter.isStaticFile("/api/status"));
}

// ========== File System Handler Tests ==========

TEST_F(HttpProcessingTest, FileSystemHandlerReadFile) {
    // Test file reading
    const char* testContent = "Test file content";
    mockFileSystemHandler.setMockFile(testContent, strlen(testContent), true);
    
    auto result = mockFileSystemHandler.readFile("/test.txt");
    
    EXPECT_TRUE(mockFileSystemHandler.readFileCalled);
    EXPECT_TRUE(result.isOk());
    EXPECT_STREQ(result.value().c_str(), testContent);
}

TEST_F(HttpProcessingTest, FileSystemHandlerFileNotExists) {
    // Test reading non-existent file
    mockFileSystemHandler.setMockFile("", 0, false);
    
    auto result = mockFileSystemHandler.readFile("/nonexistent.txt");
    
    EXPECT_TRUE(mockFileSystemHandler.readFileCalled);
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), ErrorType::SYSTEM_ERROR);
}

TEST_F(HttpProcessingTest, FileSystemHandlerFileExists) {
    // Test file existence check
    mockFileSystemHandler.mockFileExists = true;
    bool exists = mockFileSystemHandler.fileExists("/test.txt");
    
    EXPECT_TRUE(mockFileSystemHandler.fileExistsCalled);
    EXPECT_TRUE(exists);
}

TEST_F(HttpProcessingTest, FileSystemHandlerGetFileSize) {
    // Test getting file size
    const size_t testSize = 1024;
    mockFileSystemHandler.setMockFile("content", testSize, true);
    
    size_t size = mockFileSystemHandler.getFileSize("/test.txt");
    
    EXPECT_TRUE(mockFileSystemHandler.getFileSizeCalled);
    EXPECT_EQ(size, testSize);
}

// ========== MIME Type Resolver Tests ==========

TEST_F(HttpProcessingTest, MimeTypeResolverBasicTypes) {
    // Test basic MIME type resolution
    EXPECT_STREQ(mockMimeTypeResolver.getMimeType("index.html"), "text/html");
    EXPECT_STREQ(mockMimeTypeResolver.getMimeType("style.css"), "text/css");
    EXPECT_STREQ(mockMimeTypeResolver.getMimeType("script.js"), "application/javascript");
    EXPECT_STREQ(mockMimeTypeResolver.getMimeType("data.json"), "application/json");
    
    EXPECT_TRUE(mockMimeTypeResolver.getMimeTypeCalled);
}

TEST_F(HttpProcessingTest, MimeTypeResolverImageTypes) {
    // Test image MIME type resolution
    EXPECT_STREQ(mockMimeTypeResolver.getMimeType("image.png"), "image/png");
    EXPECT_STREQ(mockMimeTypeResolver.getMimeType("photo.jpg"), "image/jpeg");
    EXPECT_STREQ(mockMimeTypeResolver.getMimeType("picture.jpeg"), "image/jpeg");
}

TEST_F(HttpProcessingTest, MimeTypeResolverUnknownType) {
    // Test unknown file type
    mockMimeTypeResolver.setMockMimeType("application/octet-stream");
    const char* mimeType = mockMimeTypeResolver.getMimeType("unknown.xyz");
    
    EXPECT_STREQ(mimeType, "application/octet-stream");
}

// ========== Cache Manager Tests ==========

TEST_F(HttpProcessingTest, CacheManagerGetCachedResponse) {
    // Test getting cached response
    const char* cachedContent = "Cached test content";
    mockCacheManager.setCachedResponse(cachedContent, true);
    
    auto result = mockCacheManager.getCachedResponse("/test.html");
    
    EXPECT_TRUE(mockCacheManager.getCachedResponseCalled);
    EXPECT_TRUE(result.isOk());
    EXPECT_STREQ(result.value().content, cachedContent);
}

TEST_F(HttpProcessingTest, CacheManagerCacheResponse) {
    // Test caching a response
    const char* content = "Content to cache";
    const char* etag = "test-etag-123";
    
    bool cached = mockCacheManager.cacheResponse("/test.html", content, etag);
    
    EXPECT_TRUE(mockCacheManager.cacheResponseCalled);
    EXPECT_TRUE(cached);
    EXPECT_TRUE(mockCacheManager.isCached("/test.html"));
}

TEST_F(HttpProcessingTest, CacheManagerClearCache) {
    // Test clearing cache
    mockCacheManager.setCachedResponse("content", true);
    EXPECT_TRUE(mockCacheManager.isCached("/test"));
    
    mockCacheManager.clearCache();
    
    EXPECT_TRUE(mockCacheManager.clearCacheCalled);
    EXPECT_FALSE(mockCacheManager.isCached("/test"));
}

TEST_F(HttpProcessingTest, CacheManagerNoCachedResponse) {
    // Test getting non-cached response
    mockCacheManager.setCachedResponse("", false);
    
    auto result = mockCacheManager.getCachedResponse("/not-cached.html");
    
    EXPECT_TRUE(mockCacheManager.getCachedResponseCalled);
    EXPECT_TRUE(result.isError());
    EXPECT_EQ(result.error(), ErrorType::SYSTEM_ERROR);
}

// ========== Integration Tests ==========

TEST_F(HttpProcessingTest, FullHttpProcessingPipeline) {
    // Test full HTTP processing pipeline integration
    
    // 1. Parse request
    auto request = HttpMockTestHelper::createMockRequest("GET", "/api/status");
    EXPECT_TRUE(request.isValid());
    
    // 2. Check if it's an API request
    mockApiRouter.isApiPath = true;
    EXPECT_TRUE(mockApiRouter.isApiRequest(request.getPath()));
    
    // 3. Handle API request
    auto response = mockApiRouter.handleApiRequest(request);
    EXPECT_EQ(response.statusCode, 200);
    
    // 4. Verify all components were used
    EXPECT_TRUE(mockApiRouter.handleApiRequestCalled);
}

TEST_F(HttpProcessingTest, FileServingPipeline) {
    // Test file serving pipeline
    
    // 1. Parse file request
    auto request = HttpMockTestHelper::createMockRequest("GET", "/index.html");
    
    // 2. Check if it's a static file
    EXPECT_TRUE(mockFileRouter.isStaticFile(request.getPath()));
    
    // 3. Check file existence
    mockFileSystemHandler.mockFileExists = true;
    EXPECT_TRUE(mockFileSystemHandler.fileExists(request.getPath()));
    
    // 4. Get MIME type
    const char* mimeType = mockMimeTypeResolver.getMimeType("index.html");
    EXPECT_STREQ(mimeType, "text/html");
    
    // 5. Serve file
    auto response = mockFileRouter.handleFileRequest(request);
    EXPECT_EQ(response.statusCode, 200);
}

// ========== Performance Tests ==========

TEST_F(HttpProcessingTest, MultipleRequestProcessing) {
    // Test processing multiple requests efficiently
    const int numRequests = 100;
    
    for (int i = 0; i < numRequests; ++i) {
        String path = "/api/test" + String(i);
        auto request = HttpMockTestHelper::createMockRequest("GET", path.c_str());
        
        EXPECT_TRUE(request.isValid());
        
        auto response = mockApiRouter.handleApiRequest(request);
        EXPECT_EQ(response.statusCode, 200);
    }
    
    // Verify all requests were processed
    EXPECT_TRUE(mockApiRouter.handleApiRequestCalled);
}

// ========== Error Handling Tests ==========

TEST_F(HttpProcessingTest, ErrorHandlingInPipeline) {
    // Test error handling throughout the HTTP processing pipeline
    
    // 1. Invalid request
    mockRequestParser.isValidRequest = false;
    auto invalidRequest = mockRequestParser.parse("INVALID");
    EXPECT_FALSE(invalidRequest.isValid());
    
    // 2. File not found
    mockFileSystemHandler.mockFileExists = false;
    auto fileResult = mockFileSystemHandler.readFile("/missing.html");
    EXPECT_TRUE(fileResult.isError());
    
    // 3. API error response
    mockApiRouter.isApiPath = false;  // Simulate API endpoint not found
    auto request = HttpMockTestHelper::createMockRequest("GET", "/api/unknown");
    auto response = mockApiRouter.handleApiRequest(request);
    EXPECT_EQ(response.statusCode, 404);
}

// ========== Test Data Integration ==========

TEST_F(HttpProcessingTest, HttpTestDataUsage) {
    // Test using predefined HTTP test data
    auto& testData = HttpTestDataManager::getInstance();
    
    // Test with predefined requests
    String getRequest = HttpTestDataManager::GET_ROOT_REQUEST;
    EXPECT_TRUE(getRequest.indexOf("GET / HTTP/1.1") >= 0);
    
    String apiRequest = HttpTestDataManager::GET_API_STATUS_REQUEST;
    EXPECT_TRUE(apiRequest.indexOf("/api/status") >= 0);
    
    String postRequest = HttpTestDataManager::POST_CONFIG_REQUEST;
    EXPECT_TRUE(postRequest.indexOf("POST") >= 0);
    EXPECT_TRUE(postRequest.indexOf("Content-Type: application/json") >= 0);
}

// ========== Mock Verification Tests ==========

TEST_F(HttpProcessingTest, AllMocksProperlyReset) {
    // Verify all mocks start in reset state
    EXPECT_FALSE(mockRequestParser.parseCalled);
    EXPECT_FALSE(mockResponseBuilder.buildResponseCalled);
    EXPECT_FALSE(mockRouteHandler.addRouteCalled);
    EXPECT_FALSE(mockApiRouter.setupApiRoutesCalled);
    EXPECT_FALSE(mockFileRouter.setupFileRoutesCalled);
    EXPECT_FALSE(mockFileSystemHandler.readFileCalled);
    EXPECT_FALSE(mockMimeTypeResolver.getMimeTypeCalled);
    EXPECT_FALSE(mockCacheManager.getCachedResponseCalled);
}

TEST_F(HttpProcessingTest, MockStateConsistency) {
    // Test that mock states remain consistent after operations
    auto request = HttpMockTestHelper::createMockRequest("POST", "/api/test", "{\"data\":\"test\"}", 15);
    auto response = HttpMockTestHelper::createMockResponse(200, "Success");
    
    EXPECT_STREQ(request.getMethod(), "POST");
    EXPECT_STREQ(request.getPath(), "/api/test");
    EXPECT_EQ(request.getContentLength(), 15);
    
    EXPECT_EQ(response.statusCode, 200);
    EXPECT_STREQ(response.body, "Success");
}
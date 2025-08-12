#include "system_mocks.h"
#include "http_mocks.h"

// ========== Global System Mock Instances ==========

MockSystemState* g_mockSystemState = nullptr;
MockServiceContainer* g_mockServiceContainer = nullptr;
MockSystemInitializer* g_mockSystemInitializer = nullptr;
MockMainLoop* g_mockMainLoop = nullptr;

// ========== Global HTTP Mock Instances ==========

MockHttpRequestParser* g_mockHttpRequestParser = nullptr;
MockHttpResponseBuilder* g_mockHttpResponseBuilder = nullptr;
MockRouteHandler* g_mockRouteHandler = nullptr;
MockApiRouter* g_mockApiRouter = nullptr;
MockFileRouter* g_mockFileRouter = nullptr;
MockFileSystemHandler* g_mockFileSystemHandler = nullptr;
MockMimeTypeResolver* g_mockMimeTypeResolver = nullptr;
MockCacheManager* g_mockCacheManager = nullptr;
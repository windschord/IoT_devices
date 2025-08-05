/**
 * @file test_web_config_api.cpp
 * @brief Web設定インターフェースのバックエンドAPIテスト
 * 
 * Task 32.1の実装：
 * - 各設定カテゴリ（Network、GNSS、NTP、System、Log）のAPIテスト
 * - 基本状態表示APIのテスト
 * - エラーハンドリングとエッジケースのテスト
 * - セキュリティ機能（レート制限、入力サニタイゼーション）のテスト
 */

#include <Arduino.h>
#include <unity.h>
#include <ArduinoJson.h>
#include "test_common.h"

// Mock implementations
#include "arduino_mock.h"
#include "Ethernet.h"

// Test target includes
#include "../src/network/webserver.h"
#include "../src/config/ConfigManager.h"
#include "../src/config/LoggingService.h"
// #include "../src/time/TimeManager.h" // TimeManager is not needed for API tests

/**
 * Web Config API Backend Test Class
 * 各設定カテゴリのAPIテストとセキュリティテスト
 */
class TestWebConfigApi {
private:
  GpsWebServer* webServer;
  ConfigManager* configManager;
  LoggingService* loggingService;
  // TimeManager* timeManager;
  
  // Mock Ethernet Client for testing
  class MockEthernetClient : public EthernetClient {
  private:
    String responseBuffer;
    String postBuffer;
    
  public:
    MockEthernetClient() : responseBuffer(""), postBuffer("") {}
    
    // Mock response capture
    size_t print(const String& s) override { responseBuffer += s; return s.length(); }
    size_t println(const String& s) override { responseBuffer += s + "\n"; return s.length() + 1; }
    size_t print(const char* s) override { responseBuffer += String(s); return strlen(s); }
    size_t println(const char* s) override { responseBuffer += String(s) + "\n"; return strlen(s) + 1; }
    size_t write(uint8_t b) override { responseBuffer += (char)b; return 1; }
    
    // Mock client info
    IPAddress remoteIP() override { return IPAddress(192, 168, 1, 100); }
    uint16_t remotePort() override { return 12345; }
    bool connected() override { return true; }
    void stop() override {}
    
    // Test helpers
    String getResponse() const { return responseBuffer; }
    void clearResponse() { responseBuffer = ""; }
    void setPostData(const String& data) { postBuffer = data; }
    String getPostData() const { return postBuffer; }
  };
  
  MockEthernetClient mockClient;
  
public:
  TestWebConfigApi() {
    configManager = new ConfigManager();
    loggingService = new LoggingService(nullptr, nullptr);
    // timeManager = new TimeManager();
    
    webServer = new GpsWebServer();
    webServer->setConfigManager(configManager);
    webServer->setLoggingService(loggingService);
    
    // 初期化
    configManager->init();
  }
  
  ~TestWebConfigApi() {
    delete webServer;
    delete configManager;
    delete loggingService;
    // delete timeManager;
  }
  
  /**
   * Test 1: Network Configuration API Test
   * ネットワーク設定APIの基本機能テスト
   */
  void testNetworkConfigApi() {
    Serial.println("Testing Network Configuration API...");
    
    // GET request test
    mockClient.clearResponse();
    webServer->configNetworkApiGet(mockClient);
    String getResponse = mockClient.getResponse();
    
    // JSON形式であることを確認
    TEST_ASSERT_TRUE(getResponse.indexOf("Content-Type: application/json") >= 0);
    TEST_ASSERT_TRUE(getResponse.indexOf("{") >= 0);
    TEST_ASSERT_TRUE(getResponse.indexOf("hostname") >= 0);
    TEST_ASSERT_TRUE(getResponse.indexOf("ip_address") >= 0);
    
    // POST request test - valid data
    DynamicJsonDocument postDoc(512);
    postDoc["hostname"] = "test-server";
    postDoc["ip_address"] = 0; // DHCP
    
    String postData;
    serializeJson(postDoc, postData);
    
    mockClient.clearResponse();
    webServer->configNetworkApiPost(mockClient, postData);
    String postResponse = mockClient.getResponse();
    
    // 成功レスポンスを確認
    TEST_ASSERT_TRUE(postResponse.indexOf("200 OK") >= 0 || postResponse.indexOf("success") >= 0);
    
    // 設定が適用されたことを確認
    auto config = configManager->getConfig();
    TEST_ASSERT_EQUAL_STRING("test-server", config.hostname);
    
    Serial.println("✓ Network Configuration API test passed");
  }
  
  /**
   * Test 2: GNSS Configuration API Test  
   * GNSS設定APIの機能テスト
   */
  void testGnssConfigApi() {
    Serial.println("Testing GNSS Configuration API...");
    
    // GET request test
    mockClient.clearResponse();
    webServer->configGnssApiGet(mockClient);
    String getResponse = mockClient.getResponse();
    
    // GNSS関連設定が含まれることを確認
    TEST_ASSERT_TRUE(getResponse.indexOf("gps_enabled") >= 0);
    TEST_ASSERT_TRUE(getResponse.indexOf("glonass_enabled") >= 0);
    TEST_ASSERT_TRUE(getResponse.indexOf("galileo_enabled") >= 0);
    TEST_ASSERT_TRUE(getResponse.indexOf("gnss_update_rate") >= 0);
    
    // POST request test - constellation settings
    DynamicJsonDocument postDoc(512);
    postDoc["gps_enabled"] = true;
    postDoc["glonass_enabled"] = false;
    postDoc["galileo_enabled"] = true;
    postDoc["gnss_update_rate"] = 5;
    
    String postData;
    serializeJson(postDoc, postData);
    
    mockClient.clearResponse();
    webServer->configGnssApiPost(mockClient, postData);
    String postResponse = mockClient.getResponse();
    
    // 成功レスポンスを確認
    TEST_ASSERT_TRUE(postResponse.indexOf("success") >= 0);
    
    // 設定が適用されたことを確認
    auto config = configManager->getConfig();
    TEST_ASSERT_TRUE(config.gps_enabled);
    TEST_ASSERT_FALSE(config.glonass_enabled);
    TEST_ASSERT_TRUE(config.galileo_enabled);
    TEST_ASSERT_EQUAL_UINT8(5, config.gnss_update_rate);
    
    Serial.println("✓ GNSS Configuration API test passed");
  }
  
  /**
   * Test 3: NTP Configuration API Test
   * NTP設定APIの機能テスト
   */
  void testNtpConfigApi() {
    Serial.println("Testing NTP Configuration API...");
    
    // GET request test
    mockClient.clearResponse();
    webServer->configNtpApiGet(mockClient);
    String getResponse = mockClient.getResponse();
    
    // NTP関連設定が含まれることを確認
    TEST_ASSERT_TRUE(getResponse.indexOf("ntp_enabled") >= 0);
    TEST_ASSERT_TRUE(getResponse.indexOf("ntp_port") >= 0);
    TEST_ASSERT_TRUE(getResponse.indexOf("ntp_stratum") >= 0);
    
    // POST request test - NTP settings
    DynamicJsonDocument postDoc(512);
    postDoc["ntp_enabled"] = false;
    postDoc["ntp_port"] = 1123;
    postDoc["ntp_stratum"] = 2;
    
    String postData;
    serializeJson(postDoc, postData);
    
    mockClient.clearResponse();
    webServer->configNtpApiPost(mockClient, postData);
    String postResponse = mockClient.getResponse();
    
    // 成功レスポンスを確認
    TEST_ASSERT_TRUE(postResponse.indexOf("success") >= 0);
    
    // 設定が適用されたことを確認
    auto config = configManager->getConfig();
    TEST_ASSERT_FALSE(config.ntp_enabled);
    TEST_ASSERT_EQUAL_UINT16(1123, config.ntp_port);
    TEST_ASSERT_EQUAL_UINT8(2, config.ntp_stratum);
    
    Serial.println("✓ NTP Configuration API test passed");
  }
  
  /**
   * Test 4: System Configuration API Test
   * システム設定APIの機能テスト
   */
  void testSystemConfigApi() {
    Serial.println("Testing System Configuration API...");
    
    // GET request test
    mockClient.clearResponse();
    webServer->configSystemApiGet(mockClient);
    String getResponse = mockClient.getResponse();
    
    // システム関連設定が含まれることを確認
    TEST_ASSERT_TRUE(getResponse.indexOf("auto_restart_enabled") >= 0);
    TEST_ASSERT_TRUE(getResponse.indexOf("restart_interval") >= 0);
    TEST_ASSERT_TRUE(getResponse.indexOf("debug_enabled") >= 0);
    
    // POST request test - system settings
    DynamicJsonDocument postDoc(512);
    postDoc["auto_restart_enabled"] = true;
    postDoc["restart_interval"] = 12;
    postDoc["debug_enabled"] = true;
    
    String postData;
    serializeJson(postDoc, postData);
    
    mockClient.clearResponse();
    webServer->configSystemApiPost(mockClient, postData);
    String postResponse = mockClient.getResponse();
    
    // 成功レスポンスを確認
    TEST_ASSERT_TRUE(postResponse.indexOf("success") >= 0);
    
    // 設定が適用されたことを確認
    auto config = configManager->getConfig();
    TEST_ASSERT_TRUE(config.auto_restart_enabled);
    TEST_ASSERT_EQUAL_UINT8(12, config.restart_interval);
    TEST_ASSERT_TRUE(config.debug_enabled);
    
    Serial.println("✓ System Configuration API test passed");
  }
  
  /**
   * Test 5: Log Configuration API Test
   * ログ設定APIの機能テスト
   */
  void testLogConfigApi() {
    Serial.println("Testing Log Configuration API...");
    
    // GET request test
    mockClient.clearResponse();
    webServer->configLogApiGet(mockClient);
    String getResponse = mockClient.getResponse();
    
    // ログ関連設定が含まれることを確認
    TEST_ASSERT_TRUE(getResponse.indexOf("syslog_server") >= 0);
    TEST_ASSERT_TRUE(getResponse.indexOf("syslog_port") >= 0);
    TEST_ASSERT_TRUE(getResponse.indexOf("log_level") >= 0);
    
    // POST request test - log settings
    DynamicJsonDocument postDoc(512);
    postDoc["syslog_server"] = "192.168.1.200";
    postDoc["syslog_port"] = 1514;
    postDoc["log_level"] = 3;
    
    String postData;
    serializeJson(postDoc, postData);
    
    mockClient.clearResponse();
    webServer->configLogApiPost(mockClient, postData);
    String postResponse = mockClient.getResponse();
    
    // 成功レスポンスを確認
    TEST_ASSERT_TRUE(postResponse.indexOf("success") >= 0);
    
    // 設定が適用されたことを確認
    auto config = configManager->getConfig();
    TEST_ASSERT_EQUAL_STRING("192.168.1.200", config.syslog_server);
    TEST_ASSERT_EQUAL_UINT16(1514, config.syslog_port);
    TEST_ASSERT_EQUAL_UINT8(3, config.log_level);
    
    Serial.println("✓ Log Configuration API test passed");
  }
  
  /**
   * Test 6: Status API Test
   * システム状態表示APIのテスト
   */
  void testStatusApi() {
    Serial.println("Testing Status API...");
    
    mockClient.clearResponse();
    webServer->statusApiGet(mockClient);
    String response = mockClient.getResponse();
    
    // JSON形式であることを確認
    TEST_ASSERT_TRUE(response.indexOf("Content-Type: application/json") >= 0);
    TEST_ASSERT_TRUE(response.indexOf("{") >= 0);
    
    // 基本的なステータス情報が含まれることを確認
    TEST_ASSERT_TRUE(response.indexOf("uptime") >= 0);
    TEST_ASSERT_TRUE(response.indexOf("memory") >= 0);
    TEST_ASSERT_TRUE(response.indexOf("network") >= 0);
    
    Serial.println("✓ Status API test passed");
  }
  
  /**
   * Test 7: Security Features Test
   * セキュリティ機能（レート制限、入力サニタイゼーション）のテスト
   */
  void testSecurityFeatures() {
    Serial.println("Testing Security Features...");
    
    // Test 1: Input Sanitization
    DynamicJsonDocument maliciousDoc(512);
    maliciousDoc["hostname"] = "<script>alert('xss')</script>";
    
    String maliciousData;
    serializeJson(maliciousDoc, maliciousData);
    
    mockClient.clearResponse();
    webServer->configNetworkApiPost(mockClient, maliciousData);
    String response = mockClient.getResponse();
    
    // 悪意のあるスクリプトがサニタイズされることを確認
    auto config = configManager->getConfig();
    String hostname = String(config.hostname);
    TEST_ASSERT_FALSE(hostname.indexOf("<script>") >= 0);
    TEST_ASSERT_TRUE(hostname.indexOf("&lt;script&gt;") >= 0 || hostname.length() < 8);
    
    // Test 2: JSON Validation
    String invalidJson = "{invalid json}";
    
    mockClient.clearResponse();
    webServer->configNetworkApiPost(mockClient, invalidJson);
    String jsonResponse = mockClient.getResponse();
    
    // 不正なJSONが拒否されることを確認
    TEST_ASSERT_TRUE(jsonResponse.indexOf("400") >= 0 || jsonResponse.indexOf("error") >= 0);
    
    // Test 3: Rate Limiting (シミュレーション)
    // 実際のレート制限は時間ベースなので、基本的な機能確認のみ
    for (int i = 0; i < 5; i++) {
      mockClient.clearResponse();
      webServer->configNetworkApiPost(mockClient, "{\"hostname\":\"test\"}");
      String rateResponse = mockClient.getResponse();
      // 基本的にはレスポンスが返ることを確認（実装によりレート制限が発動する可能性あり）
      TEST_ASSERT_TRUE(rateResponse.length() > 0);
    }
    
    Serial.println("✓ Security Features test passed");
  }
  
  /**
   * Test 8: Error Handling Test
   * エラーハンドリングとエッジケースのテスト
   */
  void testErrorHandling() {
    Serial.println("Testing Error Handling...");
    
    // Test 1: Invalid field values
    DynamicJsonDocument invalidDoc(512);
    invalidDoc["gnss_update_rate"] = 999; // Invalid range
    invalidDoc["ntp_port"] = 0; // Invalid port
    invalidDoc["log_level"] = 255; // Invalid log level
    
    String invalidData;
    serializeJson(invalidDoc, invalidData);
    
    mockClient.clearResponse();
    webServer->configGnssApiPost(mockClient, invalidData);
    String gnssResponse = mockClient.getResponse();
    
    // エラーが適切に処理されることを確認
    TEST_ASSERT_TRUE(gnssResponse.indexOf("400") >= 0 || gnssResponse.indexOf("error") >= 0);
    
    // Test 2: Empty JSON
    mockClient.clearResponse();
    webServer->configSystemApiPost(mockClient, "{}");
    String emptyResponse = mockClient.getResponse();
    
    // 空のJSONでも適切に処理されることを確認
    TEST_ASSERT_TRUE(emptyResponse.indexOf("200") >= 0 || emptyResponse.indexOf("success") >= 0);
    
    // Test 3: Oversized data
    String oversizedHostname = "";
    for (int i = 0; i < 100; i++) {
      oversizedHostname += "a";
    }
    
    DynamicJsonDocument oversizedDoc(512);
    oversizedDoc["hostname"] = oversizedHostname;
    
    String oversizedData;
    serializeJson(oversizedDoc, oversizedData);
    
    mockClient.clearResponse();
    webServer->configNetworkApiPost(mockClient, oversizedData);
    String oversizedResponse = mockClient.getResponse();
    
    // サイズ制限が適切に処理されることを確認
    auto config = configManager->getConfig();
    TEST_ASSERT_TRUE(strlen(config.hostname) < 32); // hostname field size limit
    
    Serial.println("✓ Error Handling test passed");
  }
  
  /**
   * All Tests Runner
   * 全テストケースの実行
   */
  void runAllTests() {
    Serial.println("=== Web Config API Backend Tests ===");
    
    testNetworkConfigApi();
    testGnssConfigApi();
    testNtpConfigApi();
    testSystemConfigApi();
    testLogConfigApi();
    testStatusApi();
    testSecurityFeatures();
    testErrorHandling();
    
    Serial.println("=== All Web Config API Tests Completed Successfully ===");
  }
};

// Global test instance
static TestWebConfigApi* testInstance = nullptr;

// Unity test functions
void test_network_config_api() {
  if (testInstance) testInstance->testNetworkConfigApi();
}

void test_gnss_config_api() {
  if (testInstance) testInstance->testGnssConfigApi();
}

void test_ntp_config_api() {
  if (testInstance) testInstance->testNtpConfigApi();
}

void test_system_config_api() {
  if (testInstance) testInstance->testSystemConfigApi();
}

void test_log_config_api() {
  if (testInstance) testInstance->testLogConfigApi();
}

void test_status_api() {
  if (testInstance) testInstance->testStatusApi();
}

void test_security_features() {
  if (testInstance) testInstance->testSecurityFeatures();
}

void test_error_handling() {
  if (testInstance) testInstance->testErrorHandling();
}

void setUp(void) {
  // テスト前の初期化
  if (!testInstance) {
    testInstance = new TestWebConfigApi();
  }
}

void tearDown(void) {
  // テスト後のクリーンアップは各テストで実行
}

/**
 * Main test execution
 */
int main() {
  UNITY_BEGIN();
  
  // Initialize test environment
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("Starting Web Config API Backend Tests...");
  
  // Execute all tests
  RUN_TEST(test_network_config_api);
  RUN_TEST(test_gnss_config_api);
  RUN_TEST(test_ntp_config_api);
  RUN_TEST(test_system_config_api);
  RUN_TEST(test_log_config_api);
  RUN_TEST(test_status_api);
  RUN_TEST(test_security_features);
  RUN_TEST(test_error_handling);
  
  // Cleanup
  if (testInstance) {
    delete testInstance;
    testInstance = nullptr;
  }
  
  return UNITY_END();
}

#ifdef ARDUINO
void setup() {
  main();
}

void loop() {
  // Empty loop for Arduino compatibility
}
#endif
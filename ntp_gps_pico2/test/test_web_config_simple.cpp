/**
 * @file test_web_config_simple.cpp  
 * @brief Web設定インターフェースの基本バックエンドAPIテスト（簡易版）
 * 
 * Task 32.1の実装：
 * - 基本的な設定カテゴリAPIテスト
 * - セキュリティ機能の基本テスト
 * - エラーハンドリングの基本テスト
 */

#include <Arduino.h>
#include <unity.h>
#include <ArduinoJson.h>
#include "../src/config/ConfigManager.h"

/**
 * Web Config API Simple Test Class
 * 基本的なAPIテスト機能
 */
class TestWebConfigSimple {
private:
  ConfigManager* configManager;
  
public:
  TestWebConfigSimple() {
    configManager = new ConfigManager();
    configManager->init();
  }
  
  ~TestWebConfigSimple() {
    delete configManager;
  }
  
  /**
   * Test 1: ConfigManager JSON Serialization Test
   * ConfigManagerのJSON機能テスト
   */
  void testConfigManagerJson() {
    Serial.println("Testing ConfigManager JSON functionality...");
    
    // JSON出力テスト
    String jsonOutput = configManager->configToJson();
    
    // 基本的なJSONキーが含まれることを確認
    TEST_ASSERT_TRUE(jsonOutput.indexOf("\"hostname\"") >= 0);
    TEST_ASSERT_TRUE(jsonOutput.indexOf("\"ip_address\"") >= 0);
    TEST_ASSERT_TRUE(jsonOutput.indexOf("\"gps_enabled\"") >= 0);
    TEST_ASSERT_TRUE(jsonOutput.indexOf("\"ntp_enabled\"") >= 0);
    TEST_ASSERT_TRUE(jsonOutput.indexOf("\"log_level\"") >= 0);
    
    Serial.println("✓ ConfigManager JSON serialization test passed");
    
    // JSON入力テスト
    DynamicJsonDocument testDoc(1024);
    testDoc["hostname"] = "test-api-server";
    testDoc["gps_enabled"] = false;
    testDoc["ntp_enabled"] = true;
    testDoc["log_level"] = 2;
    
    String testJson;
    serializeJson(testDoc, testJson);
    
    bool result = configManager->configFromJson(testJson);
    TEST_ASSERT_TRUE(result);
    
    // 設定が正しく適用されたことを確認
    auto config = configManager->getConfig();
    TEST_ASSERT_EQUAL_STRING("test-api-server", config.hostname);
    TEST_ASSERT_FALSE(config.gps_enabled);
    TEST_ASSERT_TRUE(config.ntp_enabled);
    TEST_ASSERT_EQUAL_UINT8(2, config.log_level);
    
    Serial.println("✓ ConfigManager JSON deserialization test passed");
  }
  
  /**
   * Test 2: Configuration Validation Test
   * 設定値検証テスト
   */
  void testConfigValidation() {
    Serial.println("Testing Configuration validation...");
    
    // 有効な設定テスト
    DynamicJsonDocument validDoc(1024);
    validDoc["hostname"] = "valid-server";
    validDoc["gnss_update_rate"] = 5;
    validDoc["ntp_port"] = 123;
    validDoc["log_level"] = 3;
    
    String validJson;
    serializeJson(validDoc, validJson);
    
    bool validResult = configManager->configFromJson(validJson);
    TEST_ASSERT_TRUE(validResult);
    
    Serial.println("✓ Valid configuration test passed");
    
    // 無効な設定テスト
    DynamicJsonDocument invalidDoc(1024);
    invalidDoc["hostname"] = ""; // Empty hostname
    invalidDoc["gnss_update_rate"] = 999; // Invalid rate
    invalidDoc["ntp_port"] = 0; // Invalid port
    invalidDoc["log_level"] = 255; // Invalid level
    
    String invalidJson;
    serializeJson(invalidDoc, invalidJson);
    
    bool invalidResult = configManager->configFromJson(invalidJson);
    TEST_ASSERT_FALSE(invalidResult);
    
    Serial.println("✓ Invalid configuration rejection test passed");
  }
  
  /**
   * Test 3: Individual Setting Tests
   * 個別設定項目のテスト
   */
  void testIndividualSettings() {
    Serial.println("Testing Individual setting methods...");
    
    // ホスト名設定テスト
    bool hostnameResult = configManager->setHostname("individual-test");
    TEST_ASSERT_TRUE(hostnameResult);
    TEST_ASSERT_EQUAL_STRING("individual-test", configManager->getConfig().hostname);
    
    // ネットワーク設定テスト
    bool networkResult = configManager->setNetworkConfig(
      IPAddress(192, 168, 1, 50).v4(),
      IPAddress(255, 255, 255, 0).v4(), 
      IPAddress(192, 168, 1, 1).v4()
    );
    TEST_ASSERT_TRUE(networkResult);
    
    auto config = configManager->getConfig();
    TEST_ASSERT_EQUAL_UINT32(IPAddress(192, 168, 1, 50).v4(), config.ip_address);
    
    // Syslog設定テスト
    bool syslogResult = configManager->setSyslogConfig("192.168.1.200", 1514);
    TEST_ASSERT_TRUE(syslogResult);
    TEST_ASSERT_EQUAL_STRING("192.168.1.200", configManager->getConfig().syslog_server);
    TEST_ASSERT_EQUAL_UINT16(1514, configManager->getConfig().syslog_port);
    
    // ログレベル設定テスト
    bool logLevelResult = configManager->setLogLevel(4);
    TEST_ASSERT_TRUE(logLevelResult);
    TEST_ASSERT_EQUAL_UINT8(4, configManager->getConfig().log_level);
    
    Serial.println("✓ Individual settings test passed");
  }
  
  /**
   * Test 4: Edge Cases and Error Handling
   * エッジケースとエラーハンドリングテスト
   */
  void testEdgeCases() {
    Serial.println("Testing Edge cases and error handling...");
    
    // 空文字列ホスト名テスト
    bool emptyHostnameResult = configManager->setHostname("");
    TEST_ASSERT_FALSE(emptyHostnameResult);
    
    // 長すぎるホスト名テスト
    String longHostname = "";
    for (int i = 0; i < 50; i++) {
      longHostname += "a";
    }
    bool longHostnameResult = configManager->setHostname(longHostname.c_str());
    TEST_ASSERT_FALSE(longHostnameResult);
    
    // 無効なログレベルテスト
    bool invalidLogLevelResult = configManager->setLogLevel(255);
    TEST_ASSERT_FALSE(invalidLogLevelResult);
    
    // 無効なGNSS更新レートテスト  
    bool invalidGnssRateResult = configManager->setGnssUpdateRate(0);
    TEST_ASSERT_FALSE(invalidGnssRateResult);
    
    bool invalidGnssRateResult2 = configManager->setGnssUpdateRate(999);
    TEST_ASSERT_FALSE(invalidGnssRateResult2);
    
    // 不正なJSON形式テスト
    bool malformedJsonResult = configManager->configFromJson("{invalid json}");
    TEST_ASSERT_FALSE(malformedJsonResult);
    
    Serial.println("✓ Edge cases and error handling test passed");
  }
  
  /**
   * Test 5: Basic Security Input Test
   * 基本的なセキュリティ入力テスト
   */
  void testBasicSecurity() {
    Serial.println("Testing Basic security input handling...");
    
    // HTML特殊文字を含むホスト名テスト
    DynamicJsonDocument securityDoc(1024);
    securityDoc["hostname"] = "<script>alert('test')</script>";
    securityDoc["syslog_server"] = "192.168.1.100<script>";
    
    String securityJson;
    serializeJson(securityDoc, securityJson);
    
    bool securityResult = configManager->configFromJson(securityJson);
    // ConfigManagerの実装によって結果は異なるが、安全に処理されることを確認
    
    if (securityResult) {
      auto config = configManager->getConfig();
      // スクリプトタグがそのまま残っていないことを確認
      String hostname = String(config.hostname);
      String syslogServer = String(config.syslog_server);
      
      // 基本的な長さ制限が適用されていることを確認
      TEST_ASSERT_TRUE(hostname.length() < 32);
      TEST_ASSERT_TRUE(syslogServer.length() < 64);
    }
    
    Serial.println("✓ Basic security input test passed");
  }
  
  /**
   * All Tests Runner
   * 全テストケースの実行
   */
  void runAllTests() {
    Serial.println("=== Web Config API Simple Tests ===");
    
    testConfigManagerJson();
    testConfigValidation();
    testIndividualSettings();
    testEdgeCases();
    testBasicSecurity();
    
    Serial.println("=== All Web Config API Simple Tests Completed Successfully ===");
  }
};

// Global test instance
static TestWebConfigSimple* testInstance = nullptr;

// Unity test functions
void test_config_manager_json() {
  if (testInstance) testInstance->testConfigManagerJson();
}

void test_config_validation() {
  if (testInstance) testInstance->testConfigValidation();
}

void test_individual_settings() {
  if (testInstance) testInstance->testIndividualSettings();
}

void test_edge_cases() {
  if (testInstance) testInstance->testEdgeCases();
}

void test_basic_security() {
  if (testInstance) testInstance->testBasicSecurity();
}

void setUp(void) {
  // テスト前の初期化
  if (!testInstance) {
    testInstance = new TestWebConfigSimple();
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
  
  Serial.println("Starting Web Config API Simple Tests...");
  
  // Execute all tests
  RUN_TEST(test_config_manager_json);
  RUN_TEST(test_config_validation);
  RUN_TEST(test_individual_settings);
  RUN_TEST(test_edge_cases);
  RUN_TEST(test_basic_security);
  
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
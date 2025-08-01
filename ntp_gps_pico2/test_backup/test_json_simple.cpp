/**
 * Simple JSON Test
 * シンプルなJSON解析テスト
 */

#include <unity.h>
#include <ArduinoJson.h>

void setUp(void) {
    // テスト前の初期化
}

void tearDown(void) {
    // テスト後のクリーンアップ
}

void test_basic_json_parsing() {
    // 基本的なJSON解析テスト
    const char* testJson = "{\"log_level\": 6, \"syslog_port\": 514}";
    
    // バッファサイズ計算
    size_t capacity = JSON_OBJECT_SIZE(10) + strlen(testJson) + 200;
    DynamicJsonDocument doc(capacity);
    
    // JSONパース実行
    DeserializationError error = deserializeJson(doc, testJson);
    
    // アサーション
    TEST_ASSERT_EQUAL(DeserializationError::Ok, error);
    TEST_ASSERT_EQUAL(6, doc["log_level"]);
    TEST_ASSERT_EQUAL(514, doc["syslog_port"]);
}

void test_empty_json_parsing() {
    // 空JSONテスト
    const char* emptyJson = "";
    
    size_t capacity = JSON_OBJECT_SIZE(10) + 200;
    DynamicJsonDocument doc(capacity);
    
    DeserializationError error = deserializeJson(doc, emptyJson);
    
    // 空の文字列はEmptyInputエラーになる
    TEST_ASSERT_EQUAL(DeserializationError::EmptyInput, error);
}

void test_actual_post_data() {
    // 実際のPOSTデータテスト
    const char* postData = "{\"log_level\": 6, \"syslog_port\": 514, \"syslog_server\": \"192.168.1.100\", \"prometheus_enabled\": true}";
    
    size_t capacity = JSON_OBJECT_SIZE(10) + strlen(postData) + 200;
    DynamicJsonDocument doc(capacity);
    
    DeserializationError error = deserializeJson(doc, postData);
    
    // この実際のデータで問題が起きるか確認
    TEST_ASSERT_EQUAL(DeserializationError::Ok, error);
    TEST_ASSERT_EQUAL(6, doc["log_level"]);
    TEST_ASSERT_EQUAL(514, doc["syslog_port"]);
    TEST_ASSERT_EQUAL_STRING("192.168.1.100", doc["syslog_server"]);
    TEST_ASSERT_TRUE(doc["prometheus_enabled"]);
}

void setup() {
    delay(2000);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_basic_json_parsing);
    RUN_TEST(test_empty_json_parsing);
    RUN_TEST(test_actual_post_data);
    
    UNITY_END();
}

void loop() {
    // テスト完了後は何もしない
}
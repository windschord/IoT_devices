/**
 * JSON Parsing Debug Test
 * JSONパース問題のデバッグテスト
 */

#include <unity.h>
#include <ArduinoJson.h>
#include <Arduino.h>

void setUp(void) {
    // テスト前の初期化
}

void tearDown(void) {
    // テスト後のクリーンアップ
}

void test_basic_json_parsing() {
    // 基本的なJSON解析テスト
    String testJson = "{\"log_level\": 6, \"syslog_port\": 514, \"syslog_server\": \"192.168.1.100\", \"prometheus_enabled\": true}";
    
    // バッファサイズ計算
    size_t capacity = JSON_OBJECT_SIZE(10) + testJson.length() + 200;
    DynamicJsonDocument doc(capacity);
    
    // JSONパース実行
    doc.clear();
    DeserializationError error = deserializeJson(doc, testJson);
    
    Serial.print("Test JSON: ");
    Serial.println(testJson);
    Serial.print("Buffer capacity: ");
    Serial.println(capacity);
    Serial.print("JSON length: ");
    Serial.println(testJson.length());
    Serial.print("Parse error: ");
    Serial.println(error.c_str());
    
    // アサーション
    TEST_ASSERT_EQUAL(DeserializationError::Ok, error);
    TEST_ASSERT_EQUAL(6, doc["log_level"].as<int>());
    TEST_ASSERT_EQUAL(514, doc["syslog_port"].as<int>());
    TEST_ASSERT_EQUAL_STRING("192.168.1.100", doc["syslog_server"].as<const char*>());
    TEST_ASSERT_TRUE(doc["prometheus_enabled"].as<bool>());
}

void test_empty_json_parsing() {
    // 空JSONテスト
    String emptyJson = "";
    
    size_t capacity = JSON_OBJECT_SIZE(10) + 200;
    DynamicJsonDocument doc(capacity);
    
    doc.clear();
    DeserializationError error = deserializeJson(doc, emptyJson);
    
    Serial.print("Empty JSON error: ");
    Serial.println(error.c_str());
    
    // 空の文字列はEmptyInputエラーになる
    TEST_ASSERT_EQUAL(DeserializationError::EmptyInput, error);
}

void test_malformed_json_parsing() {
    // 不正なJSONテスト
    String malformedJson = "{\"log_level\": 6, \"syslog_port\": 514, \"syslog_server\": \"192.168.1.100\", \"prometheus_enabled\": true"; // 閉じ括弧なし
    
    size_t capacity = JSON_OBJECT_SIZE(10) + malformedJson.length() + 200;
    DynamicJsonDocument doc(capacity);
    
    doc.clear();
    DeserializationError error = deserializeJson(doc, malformedJson);
    
    Serial.print("Malformed JSON: ");
    Serial.println(malformedJson);
    Serial.print("Malformed JSON error: ");
    Serial.println(error.c_str());
    
    // 不正なJSONは InvalidInputエラーになる
    TEST_ASSERT_NOT_EQUAL(DeserializationError::Ok, error);
}

void test_large_json_parsing() {
    // 大きなJSONテスト
    String largeJson = "{\"log_level\": 6, \"syslog_port\": 514, \"syslog_server\": \"192.168.1.100\", \"prometheus_enabled\": true, "
                      "\"extra_field1\": \"value1\", \"extra_field2\": \"value2\", \"extra_field3\": \"value3\", "
                      "\"extra_field4\": \"value4\", \"extra_field5\": \"value5\"}";
    
    // 小さいバッファサイズでテスト
    size_t smallCapacity = 128;
    DynamicJsonDocument smallDoc(smallCapacity);
    
    smallDoc.clear();
    DeserializationError error1 = deserializeJson(smallDoc, largeJson);
    
    Serial.print("Large JSON (small buffer): ");
    Serial.println(largeJson);
    Serial.print("Small buffer capacity: ");
    Serial.println(smallCapacity);
    Serial.print("Large JSON error (small buffer): ");
    Serial.println(error1.c_str());
    
    // 適切なサイズのバッファでテスト
    size_t properCapacity = JSON_OBJECT_SIZE(20) + largeJson.length() + 200;
    DynamicJsonDocument properDoc(properCapacity);
    
    properDoc.clear();
    DeserializationError error2 = deserializeJson(properDoc, largeJson);
    
    Serial.print("Proper buffer capacity: ");
    Serial.println(properCapacity);
    Serial.print("Large JSON error (proper buffer): ");
    Serial.println(error2.c_str());
    
    // 適切なサイズなら成功するはず
    TEST_ASSERT_EQUAL(DeserializationError::Ok, error2);
}

void test_json_string_creation() {
    // JSON文字列作成テスト
    DynamicJsonDocument doc(1024);
    
    doc["log_level"] = 6;
    doc["syslog_port"] = 514;
    doc["syslog_server"] = "192.168.1.100";
    doc["prometheus_enabled"] = true;
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    Serial.print("Created JSON: ");
    Serial.println(jsonString);
    
    // 作成したJSONをパースし直す
    DynamicJsonDocument parseDoc(1024);
    DeserializationError error = deserializeJson(parseDoc, jsonString);
    
    Serial.print("Reparse error: ");
    Serial.println(error.c_str());
    
    TEST_ASSERT_EQUAL(DeserializationError::Ok, error);
    TEST_ASSERT_EQUAL(6, parseDoc["log_level"].as<int>());
}

void test_http_post_data_simulation() {
    // HTTP POSTデータシミュレーション
    String postData = "{\"log_level\": 6, \"syslog_port\": 514, \"syslog_server\": \"192.168.1.100\", \"prometheus_enabled\": true}";
    
    Serial.print("Simulated POST data: ");
    Serial.println(postData);
    Serial.print("POST data length: ");
    Serial.println(postData.length());
    
    // 空白文字の確認
    for (int i = 0; i < postData.length(); i++) {
        char c = postData.charAt(i);
        if (c < 32 || c > 126) {
            Serial.print("Non-printable character at position ");
            Serial.print(i);
            Serial.print(": 0x");
            Serial.println(c, HEX);
        }
    }
    
    // トリムテスト
    String trimmed = postData;
    trimmed.trim();
    
    Serial.print("Trimmed data: ");
    Serial.println(trimmed);
    Serial.print("Trimmed length: ");
    Serial.println(trimmed.length());
    
    TEST_ASSERT_TRUE(trimmed.length() > 0);
    TEST_ASSERT_TRUE(trimmed.startsWith("{"));
    TEST_ASSERT_TRUE(trimmed.endsWith("}"));
}

void setup() {
    Serial.begin(9600);
    delay(2000);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_basic_json_parsing);
    RUN_TEST(test_empty_json_parsing);
    RUN_TEST(test_malformed_json_parsing);
    RUN_TEST(test_large_json_parsing);
    RUN_TEST(test_json_string_creation);
    RUN_TEST(test_http_post_data_simulation);
    
    UNITY_END();
}

void loop() {
    // テスト完了後は何もしない
}
#include <Arduino.h>
#include <unity.h>
#include <ArduinoJson.h>

// Basic JSON integrity tests for debugging position 2048 error

void test_json_document_capacity() {
    Serial.println("Testing JSON document capacity");
    
    // Test document size that could cause issues around position 2048
    DynamicJsonDocument doc(6144);
    
    // Add test data similar to GPS API response
    doc["latitude"] = 35.6762;
    doc["longitude"] = 139.6503;
    doc["altitude"] = 10.5;
    doc["fix_type"] = 3;
    
    // Add constellation stats
    JsonObject stats = doc.createNestedObject("constellation_stats");
    stats["satellites_total"] = 32;
    stats["satellites_used"] = 24;
    
    // Add large satellites array
    JsonArray satellites = doc.createNestedArray("satellites");
    for (int i = 0; i < 32; i++) {
        JsonObject sat = satellites.createNestedObject();
        sat["prn"] = i + 1;
        sat["constellation"] = i % 6;
        sat["azimuth"] = i * 11.25;
        sat["elevation"] = 45.0;
        sat["signal_strength"] = 40 + (i % 20);
        sat["used_in_nav"] = (i % 3 == 0);
        sat["tracked"] = true;
    }
    
    // Test serialization
    String jsonString;
    size_t serializedBytes = serializeJson(doc, jsonString);
    
    Serial.printf("JSON document size: %zu bytes\n", serializedBytes);
    Serial.printf("JSON string length: %zu characters\n", jsonString.length());
    
    TEST_ASSERT_GREATER_THAN(0, serializedBytes);
    TEST_ASSERT_GREATER_THAN(0, jsonString.length());
    TEST_ASSERT_LESS_THAN(6144, jsonString.length()); // Should fit in buffer
    
    Serial.println("✓ JSON document capacity test passed");
}

void test_float_validation() {
    Serial.println("Testing float validation");
    
    // Test problematic float values
    float testValues[] = {NAN, INFINITY, -INFINITY, 123.456789, -999.999};
    size_t valueCount = sizeof(testValues) / sizeof(testValues[0]);
    
    DynamicJsonDocument doc(1024);
    
    for (size_t i = 0; i < valueCount; i++) {
        float val = testValues[i];
        
        // Validate and sanitize
        if (isnan(val) || isinf(val)) {
            val = 0.0;
        }
        
        doc["test_value"] = val;
        
        String jsonString;
        size_t bytes = serializeJson(doc, jsonString);
        
        TEST_ASSERT_GREATER_THAN(0, bytes);
        TEST_ASSERT_FALSE(jsonString.indexOf("nan") != -1);
        TEST_ASSERT_FALSE(jsonString.indexOf("inf") != -1);
        TEST_ASSERT_FALSE(jsonString.indexOf("null") != -1);
    }
    
    Serial.println("✓ Float validation test passed");
}

void test_character_sanitization() {
    Serial.println("Testing character sanitization");
    
    // Create a string with potential control characters
    String testString = "Normal text";
    for (int i = 0; i < 32; i++) {
        if (i != 9 && i != 10 && i != 13) { // Skip tab, LF, CR
            testString += char(i);
        }
    }
    testString += " End";
    
    // Sanitize string
    String sanitized = testString;
    for (size_t i = 0; i < sanitized.length(); i++) {
        char c = sanitized.charAt(i);
        if (c < 32 && c != '\t' && c != '\n' && c != '\r') {
            sanitized.setCharAt(i, ' ');
        }
    }
    
    // Test in JSON
    DynamicJsonDocument doc(1024);
    doc["test_string"] = sanitized;
    
    String jsonString;
    size_t bytes = serializeJson(doc, jsonString);
    
    TEST_ASSERT_GREATER_THAN(0, bytes);
    
    // Verify no control characters remain (except allowed ones)
    for (size_t i = 0; i < jsonString.length(); i++) {
        char c = jsonString.charAt(i);
        if (c < 32) {
            TEST_ASSERT_TRUE(c == '\t' || c == '\n' || c == '\r');
        }
    }
    
    Serial.println("✓ Character sanitization test passed");
}

void test_array_bounds() {
    Serial.println("Testing array bounds checking");
    
    const int MAX_ITEMS = 32;
    int actualItems = 50; // Intentionally over limit
    
    DynamicJsonDocument doc(4096);
    JsonArray array = doc.createNestedArray("test_array");
    
    // Safe bounds checking
    int safeCount = actualItems > MAX_ITEMS ? MAX_ITEMS : actualItems;
    TEST_ASSERT_EQUAL(MAX_ITEMS, safeCount);
    
    // Add items with bounds checking
    for (int i = 0; i < safeCount; i++) {
        if (i >= MAX_ITEMS) break; // Additional safety check
        
        JsonObject item = array.createNestedObject();
        item["id"] = i;
        item["value"] = i * 10;
    }
    
    // Verify array size
    TEST_ASSERT_EQUAL(MAX_ITEMS, array.size());
    
    String jsonString;
    size_t bytes = serializeJson(doc, jsonString);
    TEST_ASSERT_GREATER_THAN(0, bytes);
    
    Serial.println("✓ Array bounds test passed");
}

void test_memory_usage_around_2048() {
    Serial.println("Testing memory usage around position 2048");
    
    DynamicJsonDocument doc(8192);
    
    // Build JSON that approaches 2048 characters
    doc["header"] = "GPS Data Response";
    doc["timestamp"] = 1234567890;
    doc["version"] = "1.0";
    
    // Add data to reach around 2048 characters
    JsonArray longArray = doc.createNestedArray("data");
    for (int i = 0; i < 20; i++) {
        JsonObject obj = longArray.createNestedObject();
        obj["id"] = i;
        obj["description"] = "This is a longer description string to increase JSON size";
        obj["coordinates"] = "12.345678,98.765432";
        obj["status"] = "active";
        obj["metadata"] = "Additional metadata information for testing purposes";
    }
    
    String jsonString;
    size_t bytes = serializeJson(doc, jsonString);
    
    Serial.printf("Generated JSON size: %zu bytes\n", bytes);
    Serial.printf("Target around 2048, actual: %zu\n", bytes);
    
    TEST_ASSERT_GREATER_THAN(1500, bytes); // Should be substantial
    TEST_ASSERT_LESS_THAN(8192, bytes);    // Should fit in buffer
    
    // Check character at position 2048 if JSON is long enough
    if (bytes > 2048) {
        char charAt2048 = jsonString.charAt(2048);
        Serial.printf("Character at position 2048: '%c' (code: %d)\n", charAt2048, (int)charAt2048);
        
        // Should be a valid JSON character
        TEST_ASSERT_TRUE(charAt2048 >= 32 || charAt2048 == '\t' || charAt2048 == '\n' || charAt2048 == '\r');
    }
    
    Serial.println("✓ Memory usage test passed");
}

void setUp(void) {
    // Set up before each test
}

void tearDown(void) {
    // Clean up after each test
}

void setup() {
    Serial.begin(9600);
    delay(2000);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_json_document_capacity);
    RUN_TEST(test_float_validation);
    RUN_TEST(test_character_sanitization);
    RUN_TEST(test_array_bounds);
    RUN_TEST(test_memory_usage_around_2048);
    
    UNITY_END();
}

void loop() {
    // Test framework - no loop needed
}
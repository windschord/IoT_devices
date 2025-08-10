#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstdio>
#include <cstring>

// Mock LoggingService interface with GMock
class MockLoggingService {
public:
    char last_component[32] = {0};
    char last_message[256] = {0};
    int call_count = 0;
    int info_count = 0;
    int error_count = 0;
    
    MOCK_METHOD2(logInfo, void(const char* component, const char* message));
    MOCK_METHOD2(logError, void(const char* component, const char* message));
    
    // Real implementation for testing
    void realLogInfo(const char* component, const char* message) {
        if (component) strncpy(last_component, component, sizeof(last_component) - 1);
        if (message) strncpy(last_message, message, sizeof(last_message) - 1);
        call_count++;
        info_count++;
    }
    
    void realLogError(const char* component, const char* message) {
        if (component) strncpy(last_component, component, sizeof(last_component) - 1);
        if (message) strncpy(last_message, message, sizeof(last_message) - 1);
        call_count++;
        error_count++;
    }
    
    void reset() {
        memset(last_component, 0, sizeof(last_component));
        memset(last_message, 0, sizeof(last_message));
        call_count = 0;
        info_count = 0;
        error_count = 0;
    }
};

// Simple LogUtils implementation for testing
class LogUtils {
public:
    static void logInfo(MockLoggingService* service, const char* component, const char* message) {
        if (service && component && message) {
            service->logInfo(component, message);
        }
    }
    
    static void logError(MockLoggingService* service, const char* component, const char* message) {
        if (service && component && message) {
            service->logError(component, message);
        }
    }
    
    // 追加機能: ログレベル制御
    enum LogLevel {
        DEBUG = 0,
        INFO = 1,
        WARN = 2,
        ERROR = 3
    };
    
    static LogLevel current_level;
    
    static void setLogLevel(LogLevel level) {
        current_level = level;
    }
    
    static bool shouldLog(LogLevel level) {
        return level >= current_level;
    }
    
    static void logWithLevel(MockLoggingService* service, LogLevel level, 
                           const char* component, const char* message) {
        if (!shouldLog(level) || !service || !component || !message) {
            return;
        }
        
        switch (level) {
            case INFO:
                service->logInfo(component, message);
                break;
            case ERROR:
                service->logError(component, message);
                break;
            default:
                break;
        }
    }
    
    // フォーマット付きログ
    static void logInfoF(MockLoggingService* service, const char* component, 
                        const char* format, ...) {
        if (!service || !component || !format) return;
        
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        
        service->logInfo(component, buffer);
    }
    
    static void logErrorF(MockLoggingService* service, const char* component,
                         const char* format, ...) {
        if (!service || !component || !format) return;
        
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        
        service->logError(component, buffer);
    }
};

// 静的変数の初期化
LogUtils::LogLevel LogUtils::current_level = LogUtils::INFO;

// GoogleTest Fixture
class LogUtilsTest : public ::testing::Test {
protected:
    MockLoggingService mockLogger;

    void SetUp() override {
        mockLogger.reset();
        LogUtils::setLogLevel(LogUtils::INFO);
        
        // デフォルトの動作設定 - realメソッドを呼び出す
        ON_CALL(mockLogger, logInfo(::testing::_, ::testing::_))
            .WillByDefault([this](const char* component, const char* message) {
                mockLogger.realLogInfo(component, message);
            });
            
        ON_CALL(mockLogger, logError(::testing::_, ::testing::_))
            .WillByDefault([this](const char* component, const char* message) {
                mockLogger.realLogError(component, message);
            });
    }

    void TearDown() override {
        // クリーンアップ処理
    }
};

/**
 * @brief Test basic log functionality
 */
TEST_F(LogUtilsTest, BasicFunctionality) {
    EXPECT_CALL(mockLogger, logInfo(::testing::StrEq("TEST"), ::testing::StrEq("Info message")))
        .Times(1);
    EXPECT_CALL(mockLogger, logError(::testing::StrEq("ERROR"), ::testing::StrEq("Error message")))
        .Times(1);
    
    // Test INFO level
    LogUtils::logInfo(&mockLogger, "TEST", "Info message");
    EXPECT_STREQ("TEST", mockLogger.last_component);
    EXPECT_STREQ("Info message", mockLogger.last_message);
    EXPECT_EQ(1, mockLogger.info_count);
    
    // Test ERROR level
    LogUtils::logError(&mockLogger, "ERROR", "Error message");
    EXPECT_STREQ("ERROR", mockLogger.last_component);
    EXPECT_STREQ("Error message", mockLogger.last_message);
    EXPECT_EQ(1, mockLogger.error_count);
    
    EXPECT_EQ(2, mockLogger.call_count);
}

/**
 * @brief Test null pointer handling
 */
TEST_F(LogUtilsTest, NullHandling) {
    // GMockではnullptrで呼び出された場合は呼び出されないことを確認
    EXPECT_CALL(mockLogger, logInfo(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(mockLogger, logError(::testing::_, ::testing::_)).Times(0);
    
    // Test with null service - should not crash
    LogUtils::logInfo(nullptr, "TEST", "Message");
    LogUtils::logError(nullptr, "TEST", "Message");
    
    // Test with null component/message - should not crash
    LogUtils::logInfo(&mockLogger, nullptr, "Message");
    LogUtils::logInfo(&mockLogger, "TEST", nullptr);
    
    // No calls should be recorded for null inputs
    EXPECT_EQ(0, mockLogger.call_count);
}

/**
 * @brief Test multiple calls
 */
TEST_F(LogUtilsTest, MultipleCalls) {
    EXPECT_CALL(mockLogger, logInfo(::testing::StrEq("MULTI"), ::testing::StrEq("Info message")))
        .Times(5);
    
    // Multiple info calls
    for (int i = 0; i < 5; i++) {
        LogUtils::logInfo(&mockLogger, "MULTI", "Info message");
    }
    
    EXPECT_EQ(5, mockLogger.call_count);
    EXPECT_EQ(5, mockLogger.info_count);
    EXPECT_EQ(0, mockLogger.error_count);
}

/**
 * @brief Test log level control
 */
TEST_F(LogUtilsTest, LogLevelControl) {
    using ::testing::StrictMock;
    
    // ERROR レベルに設定 - ERROR のみログ出力される
    LogUtils::setLogLevel(LogUtils::ERROR);
    
    EXPECT_CALL(mockLogger, logInfo(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(mockLogger, logError(::testing::StrEq("TEST"), ::testing::StrEq("Error message")))
        .Times(1);
    
    LogUtils::logWithLevel(&mockLogger, LogUtils::INFO, "TEST", "Info message");
    LogUtils::logWithLevel(&mockLogger, LogUtils::ERROR, "TEST", "Error message");
    
    EXPECT_EQ(1, mockLogger.error_count);
    EXPECT_EQ(0, mockLogger.info_count);
}

/**
 * @brief Test formatted logging
 */
TEST_F(LogUtilsTest, FormattedLogging) {
    EXPECT_CALL(mockLogger, logInfo(::testing::StrEq("FORMAT"), ::testing::StrEq("Value: 42")))
        .Times(1);
    EXPECT_CALL(mockLogger, logError(::testing::StrEq("FORMAT"), ::testing::StrEq("Error code: 404")))
        .Times(1);
    
    LogUtils::logInfoF(&mockLogger, "FORMAT", "Value: %d", 42);
    LogUtils::logErrorF(&mockLogger, "FORMAT", "Error code: %d", 404);
    
    EXPECT_EQ(1, mockLogger.info_count);
    EXPECT_EQ(1, mockLogger.error_count);
}

/**
 * @brief パラメータ化テスト - 様々なログレベル設定
 */
class LogLevelTest : public LogUtilsTest,
                    public ::testing::WithParamInterface<std::tuple<LogUtils::LogLevel, LogUtils::LogLevel, bool>> {
};

TEST_P(LogLevelTest, CheckLogLevel) {
    LogUtils::LogLevel set_level = std::get<0>(GetParam());
    LogUtils::LogLevel test_level = std::get<1>(GetParam());
    bool should_log = std::get<2>(GetParam());
    
    LogUtils::setLogLevel(set_level);
    bool result = LogUtils::shouldLog(test_level);
    
    EXPECT_EQ(should_log, result) << "Set level: " << set_level << ", Test level: " << test_level;
}

INSTANTIATE_TEST_SUITE_P(
    LogLevelTestCases,
    LogLevelTest,
    ::testing::Values(
        std::make_tuple(LogUtils::DEBUG, LogUtils::DEBUG, true),
        std::make_tuple(LogUtils::DEBUG, LogUtils::INFO, true),
        std::make_tuple(LogUtils::DEBUG, LogUtils::ERROR, true),
        std::make_tuple(LogUtils::INFO, LogUtils::DEBUG, false),
        std::make_tuple(LogUtils::INFO, LogUtils::INFO, true),
        std::make_tuple(LogUtils::INFO, LogUtils::ERROR, true),
        std::make_tuple(LogUtils::ERROR, LogUtils::DEBUG, false),
        std::make_tuple(LogUtils::ERROR, LogUtils::INFO, false),
        std::make_tuple(LogUtils::ERROR, LogUtils::ERROR, true)
    )
);

/**
 * @brief マッチャーを使った高度なアサーション例
 */
TEST_F(LogUtilsTest, AdvancedMatchers) {
    using ::testing::HasSubstr;
    using ::testing::StartsWith;
    using ::testing::EndsWith;
    using ::testing::MatchesRegex;
    
    // 部分文字列マッチ
    EXPECT_CALL(mockLogger, logInfo(::testing::_, HasSubstr("test")))
        .Times(1);
    LogUtils::logInfo(&mockLogger, "COMPONENT", "This is a test message");
    
    // 正規表現マッチ  
    EXPECT_CALL(mockLogger, logError(::testing::_, MatchesRegex("Error: [0-9]+")))
        .Times(1);
    LogUtils::logError(&mockLogger, "REGEX", "Error: 123");
}

/**
 * @brief Action を使った副作用のテスト
 */
TEST_F(LogUtilsTest, ActionsAndSideEffects) {
    using ::testing::DoAll;
    using ::testing::SetArgPointee;
    using ::testing::Return;
    
    bool callback_called = false;
    
    EXPECT_CALL(mockLogger, logInfo(::testing::_, ::testing::_))
        .WillOnce(DoAll(
            [&callback_called](const char*, const char*) { 
                callback_called = true; 
            },
            [this](const char* component, const char* message) {
                mockLogger.realLogInfo(component, message);
            }
        ));
    
    LogUtils::logInfo(&mockLogger, "CALLBACK", "Test message");
    
    EXPECT_TRUE(callback_called);
    EXPECT_EQ(1, mockLogger.info_count);
}

/**
 * @brief Sequence を使った順序テスト
 */
TEST_F(LogUtilsTest, CallSequence) {
    using ::testing::Sequence;
    using ::testing::InSequence;
    
    {
        InSequence seq;
        EXPECT_CALL(mockLogger, logInfo(::testing::StrEq("STEP1"), ::testing::_));
        EXPECT_CALL(mockLogger, logInfo(::testing::StrEq("STEP2"), ::testing::_));
        EXPECT_CALL(mockLogger, logError(::testing::StrEq("STEP3"), ::testing::_));
    }
    
    LogUtils::logInfo(&mockLogger, "STEP1", "First step");
    LogUtils::logInfo(&mockLogger, "STEP2", "Second step");
    LogUtils::logError(&mockLogger, "STEP3", "Third step with error");
}
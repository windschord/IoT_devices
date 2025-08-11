#include <gtest/gtest.h>

// 最小限のGoogleTestサンプル
TEST(SimpleTest, BasicAssertion) {
    EXPECT_EQ(1, 1);
    EXPECT_TRUE(true);
    EXPECT_FALSE(false);
}

TEST(SimpleTest, StringComparison) {
    std::string hello = "hello";
    EXPECT_EQ("hello", hello);
    EXPECT_NE("world", hello);
}
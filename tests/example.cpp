// Examples for simple Unit Tests
// https://google.github.io/googletest/reference/testing.html
// https://google.github.io/googletest/reference/assertions.html

#include <gtest/gtest.h>

#include <array>
#include <algorithm>

// --- General form of a test ---
// 
// TEST(TestSuiteName, TestName)
// {
//      ASSERT_EQ(10, 10) << "Failure Message" 
//      EXPECT_TRUE(should_be_true) << "Failure Message"
// }
//
// Use ASSERT when the condition MUST be true (Test ends if assertion fails)
// Use EXPECT when the condition SHOULD be true (Test still continues, but result is failed)
// Prefer EXPECT since possible cleanup code can be skipped
//
//

TEST(AlgorithmTests, Sorting)
{
    std::array<int, 9> numbers = { 3, 5, 6, 2, 1, 0, -4, 4, 4 };
    std::array<int, 9> sorted = { -4, 0, 1, 2, 3, 4, 4, 5, 6 };

    std::sort(numbers.begin(), numbers.end());

    EXPECT_EQ(numbers, sorted);
}

TEST(AlgorithmTests, MaxAndMinElement)
{
    std::array<int, 9> numbers = { 3, 5, 6, 2, 1, 0, -4, 4, 4 };

    EXPECT_EQ(*std::min_element(numbers.begin(), numbers.end()), -4);
    EXPECT_EQ(*std::max_element(numbers.begin(), numbers.end()), 6);
}

//TEST(ShouldBeCommented, GuaranteedFail)
//{
//    EXPECT_TRUE(false) << "You can type error messages in your expects";
//    ASSERT_TRUE(false) << "Maybe place these inside a comment again so tests pass";
//}
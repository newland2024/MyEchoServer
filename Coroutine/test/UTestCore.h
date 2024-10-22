#pragma once

#include <regex>
#include <string>
#include <vector>

namespace SimpleUTest {

#define GREEN_BEGIN "\033[32m"
#define RED_BEGIN "\033[31m"
#define COLOR_END "\033[0m"

class TestCase {
 public:
  virtual void Run() = 0;
  virtual void Before() {}
  virtual void After() {}
  virtual void TestCaseRun() {
    Before();
    Run();
    After();
  }
  bool Result() { return result; }
  void SetResult(bool ret) { result = ret; }
  std::string CaseName() { return caseName; }
  TestCase(std::string name) : caseName(name) { result = true; }
  virtual ~TestCase() = default;

 private:
  bool result;
  std::string caseName;
};

class UnitTestCore {
 public:
  static UnitTestCore *GetInstance() {
    static UnitTestCore instance;
    return &instance;
  }

  void Run(int argc, char *argv[]) {
    failedCnt = 0;
    successCnt = 0;
    testResult = true;
    std::cout << GREEN_BEGIN << "[==============================] Running " << testCases.size() << " test case."
              << COLOR_END << std::endl;
    for (uint32_t i = 0; i < testCases.size(); i++) {
      if (argc == 2) {
        // 第二参数时，做用例CaseName来做过滤
        if (not std::regex_search(testCases[i]->CaseName(), std::regex(argv[1]))) {
          continue;
        }
      }
      std::cout << GREEN_BEGIN << "Run TestCase:" << testCases[i]->CaseName() << COLOR_END << std::endl;
      testCases[i]->TestCaseRun();
      std::cout << GREEN_BEGIN << "End TestCase:" << testCases[i]->CaseName() << COLOR_END << std::endl;
      if (testCases[i]->Result()) {
        successCnt++;
      } else {
        failedCnt++;
        testResult = false;
      }
    }
    std::cout << GREEN_BEGIN << "[==============================] Total TestCase:" << testCases.size() << COLOR_END
              << std::endl;
    std::cout << GREEN_BEGIN << "Passed:" << successCnt << COLOR_END << std::endl;
    if (failedCnt <= 0) {
      std::cout << GREEN_BEGIN << "Failed:" << failedCnt << COLOR_END << std::endl;
    } else {
      std::cout << RED_BEGIN << "Failed:" << failedCnt << COLOR_END << std::endl;
    }
  }

  TestCase *RegisterTestCase(TestCase *testCase) {
    testCases.push_back(testCase);
    return testCase;
  }

  void Clean() {
    for (auto *item : testCases) {
      delete item;
    }
  }

 private:
  bool testResult;
  int32_t successCnt;
  int32_t failedCnt;
  std::vector<TestCase *> testCases;  // 测试用例集合
};

#define TEST_CASE_CLASS(test_case_name)                                                                \
  class test_case_name : public SimpleUTest::TestCase {                                                \
   public:                                                                                             \
    test_case_name(std::string caseName) : SimpleUTest::TestCase(caseName) {}                          \
    virtual void Run();                                                                                \
                                                                                                       \
   private:                                                                                            \
    static SimpleUTest::TestCase *const testcase;                                                      \
  };                                                                                                   \
  SimpleUTest::TestCase *const test_case_name::testcase =                                              \
      SimpleUTest::UnitTestCore::GetInstance()->RegisterTestCase(new test_case_name(#test_case_name)); \
  void test_case_name::Run()

#define TEST_CASE(test_case_name) TEST_CASE_CLASS(test_case_name)

#define ASSERT_EQ(left, right)                                                                                       \
  if (left != right) {                                                                                               \
    std::cout << RED_BEGIN << "assert_eq failed at " << __FILE__ << ":" << __LINE__ << ". " << left << "!=" << right \
              << COLOR_END << std::endl;                                                                             \
    SetResult(false);                                                                                                \
    return;                                                                                                          \
  }

#define ASSERT_NE(left, right)                                                                                       \
  if (left == right) {                                                                                               \
    std::cout << RED_BEGIN << "assert_ne failed at " << __FILE__ << ":" << __LINE__ << ". " << left << "==" << right \
              << COLOR_END << std::endl;                                                                             \
    SetResult(false);                                                                                                \
    return;                                                                                                          \
  }

#define ASSERT_LT(left, right)                                                                                       \
  if (left >= right) {                                                                                               \
    std::cout << RED_BEGIN << "assert_lt failed at " << __FILE__ << ":" << __LINE__ << ". " << left << ">=" << right \
              << COLOR_END << std::endl;                                                                             \
    SetResult(false);                                                                                                \
    return;                                                                                                          \
  }

#define ASSERT_LE(left, right)                                                                                      \
  if (left > right) {                                                                                               \
    std::cout << RED_BEGIN << "assert_le failed at " << __FILE__ << ":" << __LINE__ << ". " << left << ">" << right \
              << COLOR_END << std::endl;                                                                            \
    SetResult(false);                                                                                               \
    return;                                                                                                         \
  }

#define ASSERT_GT(left, right)                                                                                       \
  if (left <= right) {                                                                                               \
    std::cout << RED_BEGIN << "assert_gt failed at " << __FILE__ << ":" << __LINE__ << ". " << left << "<=" << right \
              << COLOR_END << std::endl;                                                                             \
    SetResult(false);                                                                                                \
    return;                                                                                                          \
  }

#define ASSERT_GE(left, right)                                                                                      \
  if (left < right) {                                                                                               \
    std::cout << RED_BEGIN << "assert_ge failed at " << __FILE__ << ":" << __LINE__ << ". " << left << "<" << right \
              << COLOR_END << std::endl;                                                                            \
    SetResult(false);                                                                                               \
    return;                                                                                                         \
  }

#define ASSERT_TRUE(expr)                                                                                            \
  if (not(expr)) {                                                                                                   \
    std::cout << RED_BEGIN << "assert_true failed at " << __FILE__ << ":" << __LINE__ << ". " << expr << " is false" \
              << COLOR_END << std::endl;                                                                             \
    SetResult(false);                                                                                                \
    return;                                                                                                          \
  }

#define ASSERT_FALSE(expr)                                                                                           \
  if (expr) {                                                                                                        \
    std::cout << RED_BEGIN << "assert_false failed at " << __FILE__ << ":" << __LINE__ << ". " << expr << " if true" \
              << right << COLOR_END << std::endl;                                                                    \
    SetResult(false);                                                                                                \
    return;                                                                                                          \
  }

#define RUN_ALL_TESTS()                                        \
  int main(int argc, char *argv[]) {                           \
    SimpleUTest::UnitTestCore::GetInstance()->Run(argc, argv); \
    SimpleUTest::UnitTestCore::GetInstance()->Clean();         \
  }

}  // namespace SimpleUTest

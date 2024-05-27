#pragma once

#include <iostream>
#include <regex>
#include <string>
#include <vector>

namespace UnitTest {

static const char *kGreenBegin = "\033[32m";
static const char *kRedBegin = "\033[31m";
static const char *kColorEnd = "\033[0m";

class TestCase {
 public:
  virtual void Run() = 0;
  virtual void TestCaseRun() { Run(); }
  bool Result() { return result_; }
  void SetResult(bool result) { result_ = result; }
  std::string CaseName() { return case_name_; }
  TestCase(std::string case_name) : case_name_(case_name) {}

 private:
  bool result_{true};
  std::string case_name_;
};

class UnitTestCore {
 public:
  static UnitTestCore *GetInstance() {
    static UnitTestCore instance;
    return &instance;
  }

  int Run(int argc, char *argv[]) {
    result_ = true;
    failure_count_ = 0;
    success_count_ = 0;
    std::cout << kGreenBegin << "[==============================] Running " << test_cases_.size() << " test case."
              << kColorEnd << std::endl;
    constexpr int kFilterArgc = 2;
    for (size_t i = 0; i < test_cases_.size(); i++) {
      if (argc == kFilterArgc) {
        // 第二参数时，做用例CaseName来做过滤
        if (not std::regex_search(test_cases_[i]->CaseName(), std::regex(argv[1]))) {
          continue;
        }
      }
      std::cout << kGreenBegin << "Run TestCase:" << test_cases_[i]->CaseName() << kColorEnd << std::endl;
      test_cases_[i]->TestCaseRun();
      std::cout << kGreenBegin << "End TestCase:" << test_cases_[i]->CaseName() << kColorEnd << std::endl;
      if (test_cases_[i]->Result()) {
        success_count_++;
      } else {
        failure_count_++;
        result_ = false;
      }
    }
    std::cout << kGreenBegin << "[==============================] Total TestCase:" << test_cases_.size() << kColorEnd
              << std::endl;
    std::cout << kGreenBegin << "Passed:" << success_count_ << kColorEnd << std::endl;
    if (failure_count_ > 0) {
      std::cout << kRedBegin << "Failed:" << failure_count_ << kColorEnd << std::endl;
    }
    return 0;
  }

  TestCase *Register(TestCase *test_case) {
    test_cases_.push_back(test_case);
    return test_case;
  }

 private:
  bool result_{true};
  int32_t success_count_{0};
  int32_t failure_count_{0};
  std::vector<TestCase *> test_cases_;  // 测试用例集合
};

#define TEST_CASE_CLASS(test_case_name)                                                     \
  class test_case_name : public UnitTest::TestCase {                                        \
   public:                                                                                  \
    test_case_name(std::string case_name) : UnitTest::TestCase(case_name) {}                \
    virtual void Run();                                                                     \
                                                                                            \
   private:                                                                                 \
    static UnitTest::TestCase *const test_case_;                                            \
  };                                                                                        \
  UnitTest::TestCase *const test_case_name::test_case_ =                                    \
      UnitTest::UnitTestCore::GetInstance()->Register(new test_case_name(#test_case_name)); \
  void test_case_name::Run()

#define TEST_CASE(test_case_name) TEST_CASE_CLASS(test_case_name)

#define ASSERT_EQ(left, right)                                                                                  \
  if ((left) != (right)) {                                                                                      \
    std::cout << UnitTest::kRedBegin << "assert_eq failed at " << __FILE__ << ":" << __LINE__ << ". " << (left) \
              << "!=" << (right) << UnitTest::kColorEnd << std::endl;                                           \
    SetResult(false);                                                                                           \
    return;                                                                                                     \
  }

#define ASSERT_NE(left, right)                                                                                  \
  if ((left) == (right)) {                                                                                      \
    std::cout << UnitTest::kRedBegin << "assert_ne failed at " << __FILE__ << ":" << __LINE__ << ". " << (left) \
              << "==" << (right) << UnitTest::kColorEnd << std::endl;                                           \
    SetResult(false);                                                                                           \
    return;                                                                                                     \
  }

#define ASSERT_LT(left, right)                                                                                  \
  if ((left) >= (right)) {                                                                                      \
    std::cout << UnitTest::kRedBegin << "assert_lt failed at " << __FILE__ << ":" << __LINE__ << ". " << (left) \
              << ">=" << (right) << UnitTest::kColorEnd << std::endl;                                           \
    SetResult(false);                                                                                           \
    return;                                                                                                     \
  }

#define ASSERT_LE(left, right)                                                                                         \
  if ((left) > (right)) {                                                                                              \
    std::cout << UnitTest::kRedBegin << "assert_le failed at " << __FILE__ << ":" << __LINE__ << ". " << (left) << ">" \
              << (right) << UnitTest::kColorEnd << std::endl;                                                          \
    SetResult(false);                                                                                                  \
    return;                                                                                                            \
  }

#define ASSERT_GT(left, right)                                                                                  \
  if ((left) <= (right)) {                                                                                      \
    std::cout << UnitTest::kRedBegin << "assert_gt failed at " << __FILE__ << ":" << __LINE__ << ". " << (left) \
              << "<=" << (right) << UnitTest::kColorEnd << std::endl;                                           \
    SetResult(false);                                                                                           \
    return;                                                                                                     \
  }

#define ASSERT_GE(left, right)                                                                                         \
  if ((left) < (right)) {                                                                                              \
    std::cout << UnitTest::kRedBegin << "assert_ge failed at " << __FILE__ << ":" << __LINE__ << ". " << (left) << "<" \
              << (right) << UnitTest::kColorEnd << std::endl;                                                          \
    SetResult(false);                                                                                                  \
    return;                                                                                                            \
  }

#define ASSERT_TRUE(expr)                                                                                         \
  if (not(expr)) {                                                                                                \
    std::cout << UnitTest::kRedBegin << "assert_true failed at " << __FILE__ << ":" << __LINE__ << ". " << (expr) \
              << " is false" << UnitTest::kColorEnd << std::endl;                                                 \
    SetResult(false);                                                                                             \
    return;                                                                                                       \
  }

#define ASSERT_FALSE(expr)                                                                                         \
  if ((expr)) {                                                                                                    \
    std::cout << UnitTest::kRedBegin << "assert_false failed at " << __FILE__ << ":" << __LINE__ << ". " << (expr) \
              << " if true" << UnitTest::kColorEnd << std::endl;                                                   \
    SetResult(false);                                                                                              \
    return;                                                                                                        \
  }

#define RUN_ALL_TESTS() \
  int main(int argc, char *argv[]) { return UnitTest::UnitTestCore::GetInstance()->Run(argc, argv); }
}  // namespace UnitTest

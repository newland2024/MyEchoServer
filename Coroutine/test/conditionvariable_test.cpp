#include "conditionvariable.h"

#include <assert.h>

#include <iostream>

#include "UTestCore.h"
#include "mutex.h"
#include "mycoroutine.h"
using namespace std;

namespace {
void CondNotifyOne(MyCoroutine::Schedule &schedule, MyCoroutine::CoCond &co_cond, list<int> &queue) {
  for (int i = 0; i < 10; i++) {
    schedule.CoroutineYield();
  }
  queue.push_back(1);
  schedule.CoCondNotifyOne(co_cond);
  for (int i = 0; i < 10; i++) {
    schedule.CoroutineYield();
  }
  queue.push_back(2);
  schedule.CoCondNotifyOne(co_cond);
}

void CondNotifyAll(MyCoroutine::Schedule &schedule, MyCoroutine::CoCond &co_cond, list<int> &queue) {
  schedule.CoroutineYield();
  schedule.CoCondNotifyAll(co_cond);
  queue.push_back(1);
  queue.push_back(2);
  schedule.CoroutineYield();
  queue.push_back(3);
  schedule.CoCondNotifyAll(co_cond);
}

void CondWait1(MyCoroutine::Schedule &schedule, MyCoroutine::CoCond &co_cond, list<int> &queue) {
  schedule.CoCondWait(co_cond, [&queue]() { return queue.size() > 0; });
  assert(queue.size() >= 1);
  assert(queue.front() == 1);
  queue.pop_front();
}

void CondWait2(MyCoroutine::Schedule &schedule, MyCoroutine::CoCond &co_cond, list<int> &queue) {
  schedule.CoCondWait(co_cond, [&queue]() { return queue.size() > 0; });
  assert(queue.size() >= 1);
  assert(queue.front() == 2);
  queue.pop_front();
}

void CondWait3(MyCoroutine::Schedule &schedule, MyCoroutine::CoCond &co_cond, list<int> &queue) {
  schedule.CoCondWait(co_cond, [&queue]() { return queue.size() > 0; });
  assert(queue.size() >= 1);
  assert(queue.front() == 3);
  queue.pop_front();
}

void CondWait4(MyCoroutine::Schedule &schedule, MyCoroutine::CoCond &co_cond, list<int> &queue) {
  schedule.CoCondWait(co_cond, [&queue]() { return queue.size() > 0; });
  assert(queue.size() >= 1);
  queue.pop_front();
}

void CondWait5(MyCoroutine::Schedule &schedule, MyCoroutine::CoCond &co_cond, list<int> &queue) {
  schedule.CoCondWait(co_cond, [&queue]() { return queue.size() > 0; });
  assert(queue.size() >= 1);
  queue.pop_front();
}

void CondWait6(MyCoroutine::Schedule &schedule, MyCoroutine::CoCond &co_cond, list<int> &queue) {
  schedule.CoCondWait(co_cond, [&queue]() { return queue.size() > 0; });
  assert(queue.size() >= 1);
  queue.pop_front();
}

void CondNotifyOneWarp(MyCoroutine::Schedule &schedule, MyCoroutine::ConditionVariable &cond, list<int> &queue) {
  for (int i = 0; i < 10; i++) {
    schedule.CoroutineYield();
  }
  queue.push_back(1);
  cond.NotifyOne();
  for (int i = 0; i < 10; i++) {
    schedule.CoroutineYield();
  }
  queue.push_back(2);
  cond.NotifyOne();
}

void CondWaitWrap1(MyCoroutine::Schedule &schedule, MyCoroutine::ConditionVariable &cond, list<int> &queue) {
  cond.Wait([&queue]() { return queue.size() > 0; });
  assert(queue.size() >= 1);
  assert(queue.front() == 1);
  queue.pop_front();
}

void CondWaitWrap2(MyCoroutine::Schedule &schedule, MyCoroutine::ConditionVariable &cond, list<int> &queue) {
  cond.Wait([&queue]() { return queue.size() > 0; });
  assert(queue.size() >= 1);
  assert(queue.front() == 2);
  queue.pop_front();
}

void CondWaitWrap3(MyCoroutine::Schedule &schedule, MyCoroutine::ConditionVariable &cond, list<int> &queue) {
  cond.Wait([&queue]() { return queue.size() > 0; });
  assert(queue.size() >= 1);
  assert(queue.front() == 3);
  queue.pop_front();
}

void CondNotifyAllWarp(MyCoroutine::Schedule &schedule, MyCoroutine::ConditionVariable &cond, list<int> &queue) {
  schedule.CoroutineYield();
  cond.NotifyAll();
  queue.push_back(1);
  queue.push_back(2);
  schedule.CoroutineYield();
  queue.push_back(3);
  cond.NotifyAll();
}
}  // namespace

// 协程条件变量测试用例-NotifyOne
TEST_CASE(CoCond_NotifyOne) {
  list<int> queue;
  MyCoroutine::CoCond co_cond;
  MyCoroutine::Schedule schedule(1024);
  schedule.CoCondInit(co_cond);
  schedule.CoroutineCreate(CondNotifyOne, std::ref(schedule), std::ref(co_cond), std::ref(queue));
  schedule.CoroutineCreate(CondWait1, std::ref(schedule), std::ref(co_cond), std::ref(queue));
  schedule.CoroutineCreate(CondWait2, std::ref(schedule), std::ref(co_cond), std::ref(queue));
  schedule.Run();
  schedule.CoCondClear(co_cond);
  ASSERT_EQ(queue.size(), 0);
}

// 协程条件变量测试用例-NotifyOneWarp
TEST_CASE(CoCond_NotifyOneWarp) {
  list<int> queue;
  MyCoroutine::Schedule schedule(1024);
  MyCoroutine::ConditionVariable cond(schedule);
  schedule.CoroutineCreate(CondNotifyOneWarp, std::ref(schedule), std::ref(cond), std::ref(queue));
  schedule.CoroutineCreate(CondWaitWrap1, std::ref(schedule), std::ref(cond), std::ref(queue));
  schedule.CoroutineCreate(CondWaitWrap2, std::ref(schedule), std::ref(cond), std::ref(queue));
  schedule.Run();
  ASSERT_EQ(queue.size(), 0);
}

// 协程条件变量测试用例-NotifyAll
TEST_CASE(CoCond_NotifyAll) {
  list<int> queue;
  MyCoroutine::CoCond co_cond;
  MyCoroutine::Schedule schedule(1024);
  schedule.CoCondInit(co_cond);
  schedule.CoroutineCreate(CondNotifyAll, std::ref(schedule), std::ref(co_cond), std::ref(queue));
  schedule.CoroutineCreate(CondWait1, std::ref(schedule), std::ref(co_cond), std::ref(queue));
  schedule.CoroutineCreate(CondWait2, std::ref(schedule), std::ref(co_cond), std::ref(queue));
  schedule.CoroutineCreate(CondWait3, std::ref(schedule), std::ref(co_cond), std::ref(queue));
  schedule.Run();
  schedule.CoCondClear(co_cond);
  ASSERT_EQ(queue.size(), 0);
}

// 协程条件变量测试用例-NotifyOneWarp
TEST_CASE(CoCond_NotifyAllWarp) {
  list<int> queue;
  MyCoroutine::Schedule schedule(1024);
  MyCoroutine::ConditionVariable cond(schedule);
  schedule.CoroutineCreate(CondNotifyAllWarp, std::ref(schedule), std::ref(cond), std::ref(queue));
  schedule.CoroutineCreate(CondWaitWrap1, std::ref(schedule), std::ref(cond), std::ref(queue));
  schedule.CoroutineCreate(CondWaitWrap2, std::ref(schedule), std::ref(cond), std::ref(queue));
  schedule.CoroutineCreate(CondWaitWrap3, std::ref(schedule), std::ref(cond), std::ref(queue));
  schedule.Run();
  ASSERT_EQ(queue.size(), 0);
}

// 协程条件变量测试用例-CondResume
TEST_CASE(CoCond_CondResume) {
  list<int> queue;
  MyCoroutine::CoCond co_cond;
  MyCoroutine::Schedule schedule(1024);
  schedule.CoCondInit(co_cond);
  schedule.CoroutineCreate(CondNotifyAll, std::ref(schedule), std::ref(co_cond), std::ref(queue));
  schedule.CoroutineCreate(CondWait4, std::ref(schedule), std::ref(co_cond), std::ref(queue));
  schedule.CoroutineCreate(CondWait5, std::ref(schedule), std::ref(co_cond), std::ref(queue));
  schedule.CoroutineCreate(CondWait6, std::ref(schedule), std::ref(co_cond), std::ref(queue));

  schedule.CoroutineResume(0);
  schedule.CoroutineResume(1);
  schedule.CoroutineResume(2);
  schedule.CoroutineResume(3);
  int count = schedule.CoCondResume();
  ASSERT_EQ(count, 0);
  schedule.CoroutineResume(0);
  count = schedule.CoCondResume();
  ASSERT_EQ(count, 3);
  schedule.CoroutineResume(0);
  count = schedule.CoCondResume();
  ASSERT_EQ(count, 1);
  schedule.CoCondClear(co_cond);
  ASSERT_EQ(queue.size(), 0);
}
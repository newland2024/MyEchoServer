#include <assert.h>

#include <iostream>

#include "UTestCore.h"
#include "localvariable.h"
#include "mutex.h"
#include "mycoroutine.h"
#include "waitgroup.h"
using namespace std;

namespace {
void Sum(MyCoroutine::Schedule& schedule, int& total) {
  for (int i = 0; i < 10; i++) {
    schedule.CoroutineYield();
    total++;
  }
}

void Sum2(int& total) { total += 10; }

void SumBatch(MyCoroutine::Schedule& schedule, int& total) {
  for (int i = 0; i < 10; i++) {
    schedule.CoroutineYield();
    total++;
  }
}

void SumHasBatch(MyCoroutine::Schedule& schedule, int& total) {
  int32_t bid = schedule.BatchCreate();
  schedule.BatchAdd(bid, SumBatch, std::ref(schedule), std::ref(total));
  schedule.BatchRun(bid);
}
}  // namespace

// 协程调度的测试用例1
TEST_CASE(Schedule_Run1) {
  int total = 0;
  MyCoroutine::Schedule schedule(1);
  for (int32_t i = 0; i < 1; i++) {
    int32_t cid = schedule.CoroutineCreate(Sum, std::ref(schedule), std::ref(total));
    ASSERT_EQ(cid, i);
  }
  schedule.Run();
  ASSERT_EQ(total, 10);
}

// 协程调度的测试用例2
TEST_CASE(Schedule_Run2) {
  int total = 0;
  MyCoroutine::Schedule schedule(10240);
  for (int32_t i = 0; i < 10240; i++) {
    int32_t cid = schedule.CoroutineCreate(Sum, std::ref(schedule), std::ref(total));
    ASSERT_EQ(cid, i);
  }
  schedule.Run();
  ASSERT_EQ(total, 102400);
  total = 0;
  for (int32_t i = 0; i < 10240; i++) {
    int32_t cid = schedule.CoroutineCreate(Sum2, std::ref(total));
    ASSERT_EQ(cid, i);
  }
  schedule.Run();
  ASSERT_EQ(total, 102400);
}

// 协程调度的测试用例3
TEST_CASE(Schedule_Run3) {
  int total = 0;
  MyCoroutine::Schedule schedule(5120, 1);
  for (int32_t i = 0; i < 5120; i++) {
    int32_t cid = schedule.CoroutineCreate(SumHasBatch, std::ref(schedule), std::ref(total));
    ASSERT_EQ(cid, i);
  }
  schedule.Run();
  ASSERT_EQ(total, 51200);
}

RUN_ALL_TESTS();

#include <assert.h>

#include <iostream>

#include "UTestCore.h"
#include "mutex.h"
#include "mycoroutine.h"
#include "waitgroup.h"

using namespace std;

namespace {
void BatchChild(MyCoroutine::Schedule& schedule, int& total) { total++; }

void BatchParent(MyCoroutine::Schedule& schedule, int& total) {
  int32_t bid = schedule.BatchCreate();
  schedule.BatchAdd(bid, BatchChild, std::ref(schedule), std::ref(total));
  schedule.BatchAdd(bid, BatchChild, std::ref(schedule), std::ref(total));
  schedule.BatchAdd(bid, BatchChild, std::ref(schedule), std::ref(total));
  schedule.BatchRun(bid);

  bid = schedule.BatchCreate();
  schedule.BatchAdd(bid, BatchChild, std::ref(schedule), std::ref(total));
  schedule.BatchAdd(bid, BatchChild, std::ref(schedule), std::ref(total));
  schedule.BatchAdd(bid, BatchChild, std::ref(schedule), std::ref(total));
  schedule.BatchRun(bid);

  bid = schedule.BatchCreate();
  schedule.BatchAdd(bid, BatchChild, std::ref(schedule), std::ref(total));
  schedule.BatchAdd(bid, BatchChild, std::ref(schedule), std::ref(total));
  schedule.BatchAdd(bid, BatchChild, std::ref(schedule), std::ref(total));
  bool add = schedule.BatchAdd(bid, BatchChild, std::ref(schedule), std::ref(total));
  assert(not add);
  schedule.BatchRun(bid);
}

void WaitGroupSub(MyCoroutine::Schedule& schedule, int& total) { total++; }

void BatchWaitGroup(MyCoroutine::Schedule& schedule, int& total) {
  MyCoroutine::WaitGroup wait_group(schedule);
  wait_group.Add(WaitGroupSub, std::ref(schedule), std::ref(total));
  wait_group.Add(WaitGroupSub, std::ref(schedule), std::ref(total));
  wait_group.Add(WaitGroupSub, std::ref(schedule), std::ref(total));
  wait_group.Wait();

  MyCoroutine::WaitGroup wait_group2(schedule);
  wait_group2.Add(WaitGroupSub, std::ref(schedule), std::ref(total));
  wait_group2.Add(WaitGroupSub, std::ref(schedule), std::ref(total));
  wait_group2.Add(WaitGroupSub, std::ref(schedule), std::ref(total));
  wait_group2.Wait();
}
}  // namespace

// Batch测试用例-Origin
TEST_CASE(Batch_Origin) {
  int value = 0;
  MyCoroutine::Schedule schedule(1024, 3);
  schedule.CoroutineCreate(BatchParent, std::ref(schedule), std::ref(value));
  schedule.CoroutineResume(0);
  schedule.CoroutineResume4BatchStart(0);
  schedule.CoroutineResume4BatchFinish();
  schedule.CoroutineResume4BatchStart(0);
  schedule.CoroutineResume4BatchFinish();
  schedule.CoroutineResume4BatchStart(0);
  schedule.CoroutineResume4BatchFinish();
  ASSERT_EQ(value, 9);
}

// Batch测试用例-WaitGroup封装
TEST_CASE(Batch_WaitGroup) {
  int total = 0;
  MyCoroutine::Schedule schedule(2560, 3);
  schedule.CoroutineCreate(BatchWaitGroup, std::ref(schedule), std::ref(total));
  schedule.Run();
  ASSERT_EQ(total, 6);
}

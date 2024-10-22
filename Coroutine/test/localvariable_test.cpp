#include "localvariable.h"

#include <assert.h>

#include <iostream>

#include "UTestCore.h"
#include "mycoroutine.h"
#include "waitgroup.h"

using namespace std;

namespace {
void CoroutineLocalFunc1(MyCoroutine::Schedule& schedule,
                                 MyCoroutine::CoroutineLocal<int>& local_variable) {
  local_variable = 100;
  schedule.CoroutineYield();
  int temp = local_variable;
  assert(temp == 100);
}

void CoroutineLocalFunc2(MyCoroutine::Schedule& schedule,
                                 MyCoroutine::CoroutineLocal<int>& local_variable) {
  local_variable = 200;
  schedule.CoroutineYield();
  assert(local_variable == 200);
}

void CoroutineLocalFunc3(MyCoroutine::Schedule& schedule,
                                 MyCoroutine::CoroutineLocal<int>& local_variable) {
  local_variable = 300;
  schedule.CoroutineYield();
  assert(local_variable == 300);
}

void CoroutineLocalWithBatchChild(MyCoroutine::Schedule& schedule,
                                          MyCoroutine::CoroutineLocal<int>& local_variable) {
  assert(local_variable == 100);
}

void CoroutineLocalWithBatch(MyCoroutine::Schedule& schedule,
                                     MyCoroutine::CoroutineLocal<int>& local_variable) {
  MyCoroutine::WaitGroup wg(schedule);
  local_variable = 100;
  wg.Add(CoroutineLocalWithBatchChild, std::ref(schedule), std::ref(local_variable));
  wg.Add(CoroutineLocalWithBatchChild, std::ref(schedule), std::ref(local_variable));
  wg.Wait();
}
}  // namespace

// 协程本地变量的测试用例
TEST_CASE(Coroutine_LocalVariable) {
  MyCoroutine::Schedule schedule(10240);
  MyCoroutine::CoroutineLocal<int> local_variable(schedule);
  schedule.CoroutineCreate(CoroutineLocalFunc1, std::ref(schedule), std::ref(local_variable));
  schedule.CoroutineCreate(CoroutineLocalFunc2, std::ref(schedule), std::ref(local_variable));
  schedule.CoroutineCreate(CoroutineLocalFunc3, std::ref(schedule), std::ref(local_variable));
  schedule.Run();
}

// 协程本地变量的测试用例
TEST_CASE(Coroutine_LocalVariableWithBatch) {
  MyCoroutine::Schedule schedule(1024, 2);
  MyCoroutine::CoroutineLocal<int> local_variable(schedule);
  schedule.CoroutineCreate(CoroutineLocalWithBatch, std::ref(schedule), std::ref(local_variable));
  schedule.Run();
}

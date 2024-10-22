#include "callonce.h"

#include <assert.h>

#include <iostream>

#include "UTestCore.h"
#include "mycoroutine.h"
using namespace std;

namespace {

void CallOnceFunc(MyCoroutine::Schedule &schedule, int &value) {
  cout << "CallOnceFunc callonce" << endl;
  schedule.CoroutineYield();
  value++;
  assert(1 == value);
}

void CallOnceInit(MyCoroutine::Schedule &schedule, MyCoroutine::CallOnce &callonce, int &value) {
  callonce.Do(CallOnceFunc, std::ref(schedule), std::ref(value));
}
void CallOnceInCall(MyCoroutine::Schedule &schedule, MyCoroutine::CallOnce &callonce, int &value, int &incallvalue) {
  callonce.Do(CallOnceFunc, std::ref(schedule), std::ref(value));
  incallvalue++;
}
void CallOnceFinish(MyCoroutine::Schedule &schedule, MyCoroutine::CallOnce &callonce, int &value, int &finishvalue) {
  callonce.Do(CallOnceFunc, std::ref(schedule), std::ref(value));
  finishvalue++;
}
}  // namespace

// 协程CallOnce测试用例
TEST_CASE(CoCallOnce_ALL) {
  int value = 0;
  int incallvalue = 0;
  int finishvalue = 0;
  MyCoroutine::Schedule schedule(1024);
  MyCoroutine::CallOnce callonce(schedule);
  schedule.CoroutineCreate(CallOnceInit, std::ref(schedule), std::ref(callonce), std::ref(value));
  schedule.CoroutineCreate(CallOnceInCall, std::ref(schedule), std::ref(callonce), std::ref(value),
                           std::ref(incallvalue));
  schedule.CoroutineCreate(CallOnceInCall, std::ref(schedule), std::ref(callonce), std::ref(value),
                           std::ref(incallvalue));
  schedule.CoroutineCreate(CallOnceInCall, std::ref(schedule), std::ref(callonce), std::ref(value),
                           std::ref(incallvalue));
  schedule.CoroutineCreate(CallOnceFinish, std::ref(schedule), std::ref(callonce), std::ref(value),
                           std::ref(finishvalue));
  schedule.CoroutineResume(0);  // CallOnce状态进入kInit
  schedule.CoroutineResume(1);  // 被挂起
  schedule.CoroutineResume(2);  // 被挂起
  schedule.CoroutineResume(3);  // 被挂起
  ASSERT_EQ(incallvalue, 0);    // 因为CallOnceInCall都被挂起，所以incallvalue值还是0
  schedule.CoroutineResume(0);  // CallOnce状态进入kFinish
  ASSERT_EQ(value, 1);
  schedule.CoCallOnceResume();  // 唤醒CallOnceInCall的3个从协程
  ASSERT_EQ(incallvalue, 3);
  schedule.CoroutineResume(4);  // 唤醒CallOnceFinish的从协程
  ASSERT_EQ(value, 1);
  ASSERT_EQ(finishvalue, 1);
}

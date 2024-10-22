#include "callonce.h"

#include <iostream>

#include "mycoroutine.h"

using namespace std;
using namespace MyCoroutine;

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

int main() {
  // 创建一个协程调度对象，并自动生成大小为1024的协程池
  Schedule schedule(1024);
  int value = 0;
  int incallvalue = 0;
  int finishvalue = 0;
  CallOnce callonce(schedule);
  schedule.CoroutineCreate(CallOnceInit, std::ref(schedule), std::ref(callonce), std::ref(value));
  schedule.CoroutineCreate(CallOnceInCall, std::ref(schedule), std::ref(callonce), std::ref(value),
                           std::ref(incallvalue));
  schedule.CoroutineCreate(CallOnceInCall, std::ref(schedule), std::ref(callonce), std::ref(value),
                           std::ref(incallvalue));
  schedule.CoroutineCreate(CallOnceInCall, std::ref(schedule), std::ref(callonce), std::ref(value),
                           std::ref(incallvalue));
  schedule.CoroutineCreate(CallOnceFinish, std::ref(schedule), std::ref(callonce), std::ref(value),
                           std::ref(finishvalue));
  schedule.Run();  // Run函数完成从协程的自行调度，直到所有的从协程都执行完
  assert(1 == value);
  assert(3 == incallvalue);
  assert(1 == finishvalue);
  return 0;
}
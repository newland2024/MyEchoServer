#include "singleflight.h"

#include <assert.h>

#include <iostream>

#include "UTestCore.h"
#include "mycoroutine.h"
using namespace std;

namespace {

void SingleFlightFunc(MyCoroutine::Schedule &schedule, int &value) {
  cout << "SingleFlightFunc SingleFlight" << endl;
  schedule.CoroutineYield();
  value++;
}

void SingleFlightInit(MyCoroutine::Schedule &schedule, MyCoroutine::SingleFlight &single_flight, int &value) {
  single_flight.Do("test", SingleFlightFunc, std::ref(schedule), std::ref(value));
}
void SingleFlightInCall(MyCoroutine::Schedule &schedule, MyCoroutine::SingleFlight &single_flight, int &value,
                        int &incallvalue) {
  single_flight.Do("test", SingleFlightFunc, std::ref(schedule), std::ref(value));
  incallvalue++;
}
void SingleFlightFinish(MyCoroutine::Schedule &schedule, MyCoroutine::SingleFlight &single_flight, int &value,
                        int &finishvalue) {
  single_flight.Do("test", SingleFlightFunc, std::ref(schedule), std::ref(value));
  finishvalue++;
}
}  // namespace

// 协程SingleFlight测试用例
TEST_CASE(CoSingleFlight_ALL) {
  int value = 0;
  int incallvalue = 0;
  int finishvalue = 0;
  MyCoroutine::Schedule schedule(1024);
  MyCoroutine::SingleFlight single_flight(schedule);
  schedule.CoroutineCreate(SingleFlightInit, std::ref(schedule), std::ref(single_flight), std::ref(value));
  schedule.CoroutineCreate(SingleFlightInCall, std::ref(schedule), std::ref(single_flight), std::ref(value),
                           std::ref(incallvalue));
  schedule.CoroutineCreate(SingleFlightInCall, std::ref(schedule), std::ref(single_flight), std::ref(value),
                           std::ref(incallvalue));
  schedule.CoroutineCreate(SingleFlightInCall, std::ref(schedule), std::ref(single_flight), std::ref(value),
                           std::ref(incallvalue));
  schedule.CoroutineCreate(SingleFlightFinish, std::ref(schedule), std::ref(single_flight), std::ref(value),
                           std::ref(finishvalue));
  schedule.CoroutineResume(0);  // single_flight状态进入kInit
  schedule.CoroutineResume(1);  // 被挂起
  schedule.CoroutineResume(2);  // 被挂起
  schedule.CoroutineResume(3);  // 被挂起
  ASSERT_EQ(incallvalue, 0);    // 因为SingleFlightInCall都被挂起，所以incallvalue值还是0
  schedule.CoroutineResume(0);  // single_flight状态进入kFinish
  ASSERT_EQ(value, 1);
  schedule.CoSingleFlightResume();  // 唤醒SingleFlightInCall的3个从协程,single_flight状态重新进入kInit
  ASSERT_EQ(incallvalue, 3);
  schedule.CoroutineResume(4);  // 唤醒SingleFlightFinish的从协程，single_flight状态重新进入kInCall
  ASSERT_EQ(value, 1);
  ASSERT_EQ(finishvalue, 0);
  schedule.CoroutineResume(4);  // 唤醒SingleFlightFinish的从协程，single_flight状态重新进入kFinish
  ASSERT_EQ(value, 2);
  ASSERT_EQ(finishvalue, 1);
}

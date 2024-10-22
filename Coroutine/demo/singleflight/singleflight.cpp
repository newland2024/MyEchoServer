#include "singleflight.h"

#include <iostream>

#include "mycoroutine.h"

using namespace std;
using namespace MyCoroutine;

void SingleFlightFunc(MyCoroutine::Schedule &schedule, int &value) {
  cout << "SingleFlightFunc SingleFlight" << endl;
  schedule.CoroutineYield();
  value++;
}

void SingleFlightInit(MyCoroutine::Schedule &schedule, MyCoroutine::SingleFlight &single_flight, int &value) {
  single_flight.Do("test", SingleFlightFunc, std::ref(schedule), std::ref(value));
}
void SingleFlightInCall(MyCoroutine::Schedule &schedule, MyCoroutine::SingleFlight &single_flight, int &value, int &incallvalue) {
  single_flight.Do("test", SingleFlightFunc, std::ref(schedule), std::ref(value));
  incallvalue++;
}
void SingleFlightFinish(MyCoroutine::Schedule &schedule, MyCoroutine::SingleFlight &single_flight, int &value, int &finishvalue) {
  single_flight.Do("test", SingleFlightFunc, std::ref(schedule), std::ref(value));
  finishvalue++;
}

int main() {
  // 创建一个协程调度对象，并自动生成大小为1024的协程池
  Schedule schedule(1024);
  int value = 0;
  int incallvalue = 0;
  int finishvalue = 0;
  SingleFlight single_flight(schedule);
  schedule.CoroutineCreate(SingleFlightInit, std::ref(schedule), std::ref(single_flight), std::ref(value));
  schedule.CoroutineCreate(SingleFlightInCall, std::ref(schedule), std::ref(single_flight), std::ref(value),
                           std::ref(incallvalue));
  schedule.CoroutineCreate(SingleFlightInCall, std::ref(schedule), std::ref(single_flight), std::ref(value),
                           std::ref(incallvalue));
  schedule.CoroutineCreate(SingleFlightInCall, std::ref(schedule), std::ref(single_flight), std::ref(value),
                           std::ref(incallvalue));
  schedule.Run();  // Run函数完成从协程的自行调度，直到所有的从协程都执行完
  schedule.CoroutineCreate(SingleFlightFinish, std::ref(schedule), std::ref(single_flight), std::ref(value),
                           std::ref(finishvalue));
  schedule.Run();  // Run函数完成从协程的自行调度，直到所有的从协程都执行完
  assert(2 == value);
  assert(3 == incallvalue);
  assert(1 == finishvalue);
  return 0;
}
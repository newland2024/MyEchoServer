#include "mycoroutine.h"
#include <iostream>

using namespace std;
using namespace MyCoroutine;

void HelloWorld(Schedule &schedule) {
  cout << "hello ";
  schedule.CoroutineYield();
  cout << "world" << endl;
}

int main() {
  // 创建一个协程调度对象，并自动生成大小为1024的协程池
  Schedule schedule(1024);
  // 创建一个从协程，并手动调度
  {
    int32_t cid = schedule.CoroutineCreate(HelloWorld, ref(schedule));
    schedule.CoroutineResume(cid);
    schedule.CoroutineResume(cid);
  }
  // 创建一个从协程，并自行调度
  {
    schedule.CoroutineCreate(HelloWorld, ref(schedule));
    schedule.Run();  // Run函数完成从协程的自行调度，直到所有的从协程都执行完
  }
  return 0;
}
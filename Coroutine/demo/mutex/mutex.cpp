#include "mycoroutine.h"
#include "mutex.h"
#include <iostream>

using namespace std;
using namespace MyCoroutine;

void Mutex1(Schedule &schedule, Mutex& mutex, int& sum) {
  LockGuard lg(mutex);
  assert(0 == sum);
  sum++;
}
void Mutex2(Schedule &schedule, Mutex& mutex, int& sum) {
  LockGuard lg(mutex);
  assert(1 == sum);
  sum++;
}
void Mutex3(Schedule &schedule, Mutex& mutex, int& sum) {
  LockGuard lg(mutex);
  assert(2 == sum);
  sum++;
}

int main() {
  // 创建一个协程调度对象，并自动生成大小为1024的协程池
  Schedule schedule(1024);
  Mutex mutex(schedule);  // 互斥量在唤醒被阻塞的从协程，是按先进先出的策略
  int sum = 0;
  schedule.CoroutineCreate(Mutex1, ref(schedule), ref(mutex), ref(sum));
  schedule.CoroutineCreate(Mutex2, ref(schedule), ref(mutex), ref(sum));
  schedule.CoroutineCreate(Mutex3, ref(schedule), ref(mutex), ref(sum));
  schedule.Run();  // Run函数完成从协程的自行调度，直到所有的从协程都执行完
  cout << "sum = " << sum << endl;
  return 0;
}
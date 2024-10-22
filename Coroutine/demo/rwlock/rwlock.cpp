#include "rwlock.h"

#include <iostream>

#include "mycoroutine.h"

using namespace std;
using namespace MyCoroutine;

void RwLockWrLock(Schedule& schedule, RWLock& rwlock, int& sum) {
  WrLockGuard lg(rwlock);
  assert(0 == sum);
  schedule.CoroutineYield();
  sum++;
}
void RwLockRdLock1(Schedule& schedule, RWLock& rwlock, int& sum) {
  RdLockGuard lg(rwlock);
  assert(1 == sum);
  schedule.CoroutineYield();
  sum++;
}
void RwLockRdLock2(Schedule& schedule, RWLock& rwlock, int& sum) {
  RdLockGuard lg(rwlock);
  assert(1 == sum);
  schedule.CoroutineYield();
  sum++;
}

int main() {
  // 创建一个协程调度对象，并自动生成大小为1024的协程池
  Schedule schedule(1024);
  RWLock rwlock(schedule);  // 读写锁唤醒被阻塞的从协程，是按先进先出的策略
  int sum = 0;
  schedule.CoroutineCreate(RwLockWrLock, ref(schedule), ref(rwlock), ref(sum));
  schedule.CoroutineCreate(RwLockRdLock1, ref(schedule), ref(rwlock), ref(sum));
  schedule.CoroutineCreate(RwLockRdLock2, ref(schedule), ref(rwlock), ref(sum));
  schedule.Run();  // Run函数完成从协程的自行调度，直到所有的从协程都执行完
  assert(3 == sum);
  cout << "sum = " << sum << endl;
  return 0;
}
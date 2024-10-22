#include "mycoroutine.h"
#include "conditionvariable.h"
#include <iostream>
using namespace std;
using namespace MyCoroutine;

void Consumer(Schedule &schedule, ConditionVariable &cond, list<int32_t> &q,
              int32_t &sum) {
  while (sum != 10) {
    cond.Wait([&q]() { return q.size() > 0; });
    sum += q.front();
    q.pop_front();
  }
}

void Producer(Schedule &schedule, ConditionVariable &cond, list<int32_t> &q) {
  q.push_back(1);
  cond.NotifyOne();
  q.push_back(2);
  q.push_back(3);
  q.push_back(4);
  cond.NotifyAll();
}

int main() {
  // 创建一个协程调度对象，生成大小为1024的协程池
  Schedule schedule(1024);
  int32_t sum = 0;
  list<int32_t> q;
  // 条件变量
  ConditionVariable cond(schedule);
  schedule.CoroutineCreate(Producer, ref(schedule), ref(cond), ref(q));
  schedule.CoroutineCreate(Consumer, ref(schedule), ref(cond), ref(q), ref(sum));
  schedule.Run(); // Run函数完成从协程的自行调度，直到所有的从协程都执行完
  cout << "sum = " << sum << endl;
  return 0;
}
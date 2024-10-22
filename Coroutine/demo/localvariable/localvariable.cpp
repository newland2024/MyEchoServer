#include "mycoroutine.h"
#include "localvariable.h"
#include <iostream>

using namespace std;
using namespace MyCoroutine;

void LocalVar1(Schedule &schedule, CoroutineLocal<int32_t> &local_var,
               int &sum) {
  local_var = 100;
  schedule.CoroutineYield();
  assert(100 == local_var);
  sum += local_var;
}
void LocalVar2(Schedule &schedule, CoroutineLocal<int32_t> &local_var,
               int &sum) {
  local_var = 200;
  schedule.CoroutineYield();
  assert(200 == local_var);
  sum += local_var;
}

int main() {
  // 创建一个协程调度对象，并自动生成大小为1024的协程池
  Schedule schedule(1024);
  int sum = 0;
  CoroutineLocal<int32_t> local_var(schedule);
  schedule.CoroutineCreate(LocalVar1, ref(schedule), ref(local_var), ref(sum));
  schedule.CoroutineCreate(LocalVar2, ref(schedule), ref(local_var), ref(sum));
  schedule.Run();  // Run函数完成从协程的自行调度，直到所有的从协程都执行完
  cout << "sum = " << sum << endl;
  return 0;
}
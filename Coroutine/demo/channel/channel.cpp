#include "channel.h"

#include <iostream>

#include "mycoroutine.h"

using namespace std;
using namespace MyCoroutine;


void Producer(MyCoroutine::Schedule &schedule, MyCoroutine::Channel<int> &channel) {
  for (int i = 1; i <= 100; i++) {
    int * value = new int;
    *value = i;
    channel.Send(value);
  }
}

void Consumer(MyCoroutine::Schedule &schedule, MyCoroutine::Channel<int> &channel) {
  int sum = 0;
  for (int i = 1; i <= 100; i++) {
    int * value = channel.Receive();
    sum += (*value);
    delete value;
  }
  assert(sum == 5050);
  cout << "sum = " << sum << endl;
}

int main() {
  // 创建一个协程调度对象，并自动生成大小为1024的协程池
  Schedule schedule(1024);
  Channel<int> channel(schedule, 1);  // 带缓冲的Channel
  schedule.CoroutineCreate(Producer, std::ref(schedule), std::ref(channel));
  schedule.CoroutineCreate(Consumer, std::ref(schedule), std::ref(channel));
  schedule.Run();  // Run函数完成从协程的自行调度，直到所有的从协程都执行完
  return 0;
}
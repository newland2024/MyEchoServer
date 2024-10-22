#include "semaphore.h"

#include <assert.h>

#include <iostream>
#include <queue>

#include "mycoroutine.h"
using namespace std;

void Producer(MyCoroutine::Schedule &schedule, MyCoroutine::Semaphore &sem, queue<int> &q, int &finish) {
  for (int i = 1; i <= 100; i++) {
    sem.Post();
    q.push(i);
    schedule.CoroutineYield();
  }
  // 只要 Consumer 没都执行完，就一直调用 Post，驱动 Consumer 被唤醒
  while (finish != 10) {
    sem.Post();
    q.push(0);
    schedule.CoroutineYield();
  }
}

void Consumer(MyCoroutine::Schedule &schedule, MyCoroutine::Semaphore &sem, queue<int> &q, int &value, int &finish) {
  while (value != 50500) {
    sem.Wait();
    assert(not q.empty());
    value += q.front();
    q.pop();
    schedule.CoroutineYield();
  }
  finish++;
}

int main() {
  int value = 0;
  int finish = 0;
  queue<int> q;
  MyCoroutine::Schedule schedule(1024);
  MyCoroutine::Semaphore sem(schedule, 100);
  for (int i = 0; i < 100; i++) {
    q.push(0);
  }
  // 10个 Producer 和 10个 Consumer
  for (int i = 0; i < 10; i++) {
    schedule.CoroutineCreate(Producer, std::ref(schedule), std::ref(sem), std::ref(q), std::ref(finish));
    schedule.CoroutineCreate(Consumer, std::ref(schedule), std::ref(sem), std::ref(q), std::ref(value),
                             std::ref(finish));
  }
  schedule.Run();
  cout << "finish = " << finish << endl;
  cout << "value = " << value << endl;
  cout << "q.size() = " << q.size() << endl;
  return 0;
}

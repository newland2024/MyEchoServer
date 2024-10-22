#include "semaphore.h"

#include "mycoroutine.h"

namespace MyCoroutine {
void Schedule::CoSemaphoreInit(CoSemaphore &semaphore, int64_t value) {
  assert(value >= 0);
  semaphore.value = value;
  semaphores_.insert(&semaphore);
}

void Schedule::CoSemaphoreClear(CoSemaphore &semaphore) { semaphores_.erase(&semaphore); }

void Schedule::CoSemaphorePost(CoSemaphore &semaphore) {
  assert(not is_master_);
  semaphore.value++;
}

void Schedule::CoSemaphoreWait(CoSemaphore &semaphore) {
  while (semaphore.value <= 0) {
    semaphore.suspend_cid_set.insert(slave_cid_);
    CoroutineYield();
  }
  semaphore.value--;
  semaphore.suspend_cid_set.erase(slave_cid_);
}

int Schedule::CoSemaphoreResume() {
  assert(is_master_);
  int count = 0;
  for (auto semaphore : semaphores_) {
    assert(semaphore->value >= 0);
    if (semaphore->value == 0) continue;
    auto cid_set = semaphore->suspend_cid_set;
    for (const auto cid : cid_set) {
      count++;
      CoroutineResume(cid);
      if (semaphore->value <= 0) {
        break;
      }
    }
  }
  return count;
}
}  // namespace MyCoroutine
#include "mutex.h"

namespace MyCoroutine {
void Schedule::CoMutexInit(CoMutex& mutex) {
  mutex.lock = false;
  mutex.hold_cid = kInvalidCid;
  assert(mutexs_.find(&mutex) == mutexs_.end());
  mutexs_.insert(&mutex);
}

void Schedule::CoMutexClear(CoMutex& mutex) { mutexs_.erase(&mutex); }

void Schedule::CoMutexLock(CoMutex& mutex) {
  while (true) {
    assert(not is_master_);
    if (not mutex.lock) {
      mutex.lock = true;  // 加锁成功，直接返回
      mutex.hold_cid = slave_cid_;
      return;
    }
    // 不可重入，同一个从协程只能锁定一次，不能锁定多次
    assert(mutex.hold_cid != slave_cid_);
    // 更新因为等待互斥锁而被挂起的从协程id
    auto iter = find(mutex.suspend_cid_list.begin(), mutex.suspend_cid_list.end(), slave_cid_);
    if (iter == mutex.suspend_cid_list.end()) {
      mutex.suspend_cid_list.push_back(slave_cid_);
    }
    // 从协程让出执行权
    CoroutineYield();
  }
}

bool Schedule::CoMutexTryLock(CoMutex& mutex) {
  assert(not is_master_);
  if (not mutex.lock) {
    mutex.lock = true;  // 加锁成功，直接返回
    mutex.hold_cid = slave_cid_;
    return true;
  }
  return false;
}

void Schedule::CoMutexUnLock(CoMutex& mutex) {
  assert(not is_master_);
  assert(mutex.lock);                    // 必须是锁定的
  assert(mutex.hold_cid == slave_cid_);  // 必须是持有锁的从协程来释放锁。
  mutex.lock = false;  // 设置成false即可，后续由调度器schedule去激活那些被挂起的从协程
  mutex.hold_cid = kInvalidCid;
  // 释放锁之后，要判断之前是否在等待队列中，如果是，则需要从等待队列中删除
  auto iter = find(mutex.suspend_cid_list.begin(), mutex.suspend_cid_list.end(), slave_cid_);
  if (iter != mutex.suspend_cid_list.end()) {
    mutex.suspend_cid_list.erase(iter);
  }
}

void Schedule::CoMutexResume() {
  assert(is_master_);
  for (auto* mutex : mutexs_) {
    if (mutex->lock) continue;                          // 锁没释放，不需要唤醒其他从协程
    if (mutex->suspend_cid_list.size() <= 0) continue;  // 锁已经释放了，但是没有挂起的从协程，也不需要唤醒
    int32_t cid = mutex->suspend_cid_list.front();
    mutex->suspend_cid_list.pop_front();
    CoroutineResume(cid);  // 每次只能唤醒等待队列中的一个从协程，采用先进先出的策略
  }
}
}  // namespace MyCoroutine

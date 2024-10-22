#include "rwlock.h"

namespace MyCoroutine {
void Schedule::CoRWLockInit(CoRWLock &rwlock) {
  rwlock.lock_state = RWLockState::kUnLock;
  rwlock.hold_write_cid = kInvalidCid;
  assert(rwlocks_.find(&rwlock) == rwlocks_.end());
  rwlocks_.insert(&rwlock);
}

void Schedule::CoRWLockClear(CoRWLock &rwlock) { rwlocks_.erase(&rwlock); }

void Schedule::CoRWLockWrLock(CoRWLock &rwlock) {
  while (true) {
    assert(not is_master_);
    if (rwlock.lock_state == RWLockState::kUnLock) {  // 无锁状态，直接加写锁，并返回
      assert(rwlock.hold_write_cid == kInvalidCid);
      rwlock.lock_state = RWLockState::kWriteLock;
      rwlock.hold_write_cid = slave_cid_;
      return;
    }
    if (rwlock.lock_state == RWLockState::kWriteLock) {
      // 不可重入，同一个从协程只能锁一次写锁，不能锁定多次
      assert(rwlock.hold_write_cid != slave_cid_);
    }
    // 更新因为等待读写锁而被挂起的从协程信息（只要有加锁了，加写锁的协程都要做挂起）
    auto iter = find(rwlock.suspend_list.begin(), rwlock.suspend_list.end(), make_pair(RWLockType::kWrite, slave_cid_));
    if (iter == rwlock.suspend_list.end()) {
      rwlock.suspend_list.push_back(make_pair(RWLockType::kWrite, slave_cid_));
    }
    // 从协程让出执行权
    CoroutineYield();
  }
}

void Schedule::CoRWLockWrUnLock(CoRWLock &rwlock) {
  assert(not is_master_);
  assert(rwlock.lock_state == RWLockState::kWriteLock);  // 必须是写锁锁定
  assert(rwlock.hold_write_cid == slave_cid_);           // 必须是持有锁的从协程来释放锁
  assert(rwlock.hold_read_cid_set.size() == 0);          // 持有读锁的协程集合必须为空
  rwlock.lock_state = RWLockState::kUnLock;  // 设置成无锁状态即可，后续由调度器schedule去激活那些被挂起的从协程
  rwlock.hold_write_cid = kInvalidCid;
  // 释放锁之后，则需要从等待队列中删除（可能之前已经在CoRWLockResume中删除了，这里是做兜底的清理）
  rwlock.suspend_list.remove(make_pair(RWLockType::kWrite, slave_cid_));
}

void Schedule::CoRWLockRdLock(CoRWLock &rwlock) {
  while (true) {
    assert(not is_master_);
    if (rwlock.lock_state == RWLockState::kUnLock) {  // 无锁状态，直接加读锁
      assert(rwlock.hold_read_cid_set.size() == 0);
      rwlock.lock_state = RWLockState::kReadLock;
      rwlock.hold_read_cid_set.insert(slave_cid_);
      return;
    }
    if (rwlock.lock_state == RWLockState::kReadLock) {  // 读锁锁定状态，也可以加读锁成功
      // 读锁不可重入，一个协程不可以对同一个读写锁，加多次读锁。
      assert(rwlock.hold_read_cid_set.find(slave_cid_) == rwlock.hold_read_cid_set.end());
      rwlock.hold_read_cid_set.insert(slave_cid_);
      return;
    }
    // 执行到这里，就是写锁锁定状态，需要做挂起等待，等待写锁释放。
    auto iter = find(rwlock.suspend_list.begin(), rwlock.suspend_list.end(), make_pair(RWLockType::kRead, slave_cid_));
    if (iter == rwlock.suspend_list.end()) {
      rwlock.suspend_list.push_back(make_pair(RWLockType::kRead, slave_cid_));
    }
    // 从协程让出执行权
    CoroutineYield();
  }
}

void Schedule::CoRWLockRdUnLock(CoRWLock &rwlock) {
  assert(not is_master_);
  assert(rwlock.lock_state == RWLockState::kReadLock);  // 必须是读锁锁定
  assert(rwlock.hold_read_cid_set.find(slave_cid_) !=
         rwlock.hold_read_cid_set.end());  // 必须是持有锁的从协程来释放锁。
  rwlock.hold_read_cid_set.erase(slave_cid_);
  // 释放锁之后，则需要从等待队列中删除（可能之前已经在CoRWLockResume中删除了，这里是做兜底的清理）
  rwlock.suspend_list.remove(make_pair(RWLockType::kRead, slave_cid_));
  if (rwlock.hold_read_cid_set.size() <= 0) {
    // 没有持有读锁的协程了，才设置成无锁状态，后续由调度器schedule去激活那些被挂起的从协程
    rwlock.lock_state = RWLockState::kUnLock;
  }
}

void Schedule::CoRWLockResume() {
  assert(is_master_);
  for (auto *rwlock : rwlocks_) {
    if (rwlock->lock_state == RWLockState::kWriteLock || rwlock->lock_state == RWLockState::kReadLock) {
      // 这里读锁也不唤醒是因为，因为读锁而被挂起的只有写锁，故唤醒了写锁的协程，这个协程也会再度被挂起
      continue;  // 写锁或者读锁锁定，不需要唤醒其他从协程
    }
    while (rwlock->suspend_list.size() > 0 &&
           (rwlock->lock_state == RWLockState::kUnLock || rwlock->lock_state == RWLockState::kReadLock)) {
      if (rwlock->lock_state == RWLockState::kUnLock) {  // 无锁
        auto item = rwlock->suspend_list.front();
        rwlock->suspend_list.pop_front();
        CoroutineResume(item.second);
        continue;
      }
      // 执行到这里是读锁
      auto item = rwlock->suspend_list.front();
      if (item.first == RWLockType::kRead) {  // 可以再加读锁
        rwlock->suspend_list.pop_front();
        CoroutineResume(item.second);
      } else {
        break;  // 不可以再加写锁，直接跳出while循环
      }
    }
  }
}
}  // namespace MyCoroutine

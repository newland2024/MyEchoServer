#include <assert.h>

#include "mycoroutine.h"

namespace MyCoroutine {

int32_t Schedule::BatchCreate() {
  assert(not is_master_);
  assert(max_concurrency_in_batch_ > 0);
  for (int32_t i = 0; i < kMaxBatchSize; i++) {
    if (batchs_[i]->state == State::kIdle) {
      batchs_[i]->state = State::kReady;
      batchs_[i]->parent_cid = slave_cid_;      // 设置批量执行关联的父从协程
      coroutines_[slave_cid_]->relate_bid = i;  // 设置从协程关联的批量执行
      return i;
    }
  }
  return kInvalidBid;
}

void Schedule::BatchRun(int32_t bid) {
  assert(not is_master_);
  assert(bid >= 0 && bid < kMaxBatchSize);
  assert(batchs_[bid]->parent_cid == slave_cid_);
  batchs_[bid]->state = State::kRun;
  CoroutineYield();  // BatchRun只是一个卡点，等Batch中所有的子从协程都执行完了，主协程再恢复父从协程的执行
  batchs_[bid]->state = State::kIdle;
  batchs_[bid]->parent_cid = kInvalidCid;
  batchs_[bid]->child_cid_2_finish.clear();
  coroutines_[slave_cid_]->relate_bid = kInvalidBid;
}

bool Schedule::IsBatchDone(int32_t bid) {
  assert(is_master_);
  assert(bid >= 0 && bid < kMaxBatchSize);
  assert(batchs_[bid]->state == State::kRun);  // 校验Batch的状态，必须是run的状态
  for (const auto& kv : batchs_[bid]->child_cid_2_finish) {
    if (not kv.second) return false;  // 只要有一个关联的子从协程没执行完，就返回false
  }
  return true;
}

void Schedule::CoroutineResume4BatchStart(int32_t cid) {
  assert(is_master_);
  assert(cid >= 0 && cid < coroutine_count_);
  Coroutine* routine = coroutines_[cid];
  // 从协程中没有关联的Batch，则没有需要唤醒的子从协程
  if (routine->relate_bid == kInvalidBid) {
    return;
  }
  int32_t bid = routine->relate_bid;
  // 从协程不是Batch中的父从协程，则没有需要唤醒的子从协程
  if (batchs_[bid]->parent_cid != cid) {
    return;
  }
  for (const auto& item : batchs_[bid]->child_cid_2_finish) {
    assert(not item.second);
    CoroutineResume(item.first);  // 唤醒Batch中的子从协程
  }
}

void Schedule::CoroutineResume4BatchFinish() {
  assert(is_master_);
  if (batch_finish_cid_list_.size() <= 0) {
    return;
  }
  for (const auto& cid : batch_finish_cid_list_) {
    CoroutineResume(cid);
  }
  batch_finish_cid_list_.clear();
}
}  // namespace MyCoroutine

#include <assert.h>

#include "mycoroutine.h"

namespace MyCoroutine {
void Schedule::LocalVariableSet(void* key, const LocalVariable& local_variable) {
  assert(not is_master_);
  auto iter = coroutines_[slave_cid_]->local.find(key);
  if (iter != coroutines_[slave_cid_]->local.end()) {
    iter->second.free(iter->second.data);  // 之前有值，则要先释放空间
  }
  coroutines_[slave_cid_]->local[key] = local_variable;
}

bool Schedule::LocalVariableGet(void* key, LocalVariable& local_variable) {
  assert(not is_master_);
  auto iter = coroutines_[slave_cid_]->local.find(key);
  if (iter == coroutines_[slave_cid_]->local.end()) {
    int32_t relate_bid = coroutines_[slave_cid_]->relate_bid;
    if (relate_bid == kInvalidBid) {  // 没有关联的Batch，直接返回false
      return false;
    }
    int32_t parent_cid = batchs_[relate_bid]->parent_cid;
    iter = coroutines_[parent_cid]->local.find(key);
    if (iter == coroutines_[parent_cid]->local.end()) {  // 父从协程中也没查找到，直接返回false
      return false;
    }
  }
  local_variable = iter->second;
  return true;
}
}  // namespace MyCoroutine
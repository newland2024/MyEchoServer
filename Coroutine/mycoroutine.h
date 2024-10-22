#pragma once
#include <assert.h>

#include <list>
#include <unordered_set>

#include "common.h"

namespace MyCoroutine {
// 协程调度器
class Schedule {
 public:
  explicit Schedule(int32_t coroutine_count, int32_t max_concurrency_in_batch = 0);
  ~Schedule();

  // 从协程的创建函数，通过模版函数，可以支持不同原型的函数，作为从协程的执行函数
  template <typename Function, typename... Args>
  int32_t CoroutineCreate(Function &&func, Args &&...args) {
    int32_t cid = 0;
    for (cid = 0; cid < coroutine_count_; cid++) {
      if (coroutines_[cid]->state == State::kIdle) {
        break;
      }
    }
    if (cid >= coroutine_count_) {
      return kInvalidCid;
    }
    Coroutine *routine = coroutines_[cid];
    function<void()> entry = bind(forward<Function>(func), forward<Args>(args)...);
    CoroutineInit(routine, entry);
    return cid;
  }

  void Run();                         // 协程调度执行
  void CoroutineYield();              // 从协程让出cpu执行权
  void CoroutineResume(int32_t cid);  // 主协程唤醒指定的从协程

  void CoroutineResume4BatchStart(int32_t cid);  // 主协程唤醒指定从协程中的批量执行中的子从协程
  void CoroutineResume4BatchFinish();            // 主协程唤醒被插入批量执行的父从协程的调用

  void LocalVariableSet(void *key, const LocalVariable &local_variable);  // 设置协程本地变量
  bool LocalVariableGet(void *key, LocalVariable &local_variable);        // 获取协程本地变量

  int32_t BatchCreate();  // 创建一个批量执行
  // 在批量执行中添加任务
  template <typename Function, typename... Args>
  bool BatchAdd(int32_t bid, Function &&func, Args &&...args) {
    assert(not is_master_);                          // 从协程中才可以调用
    assert(bid >= 0 && bid < kMaxBatchSize);         // 校验bid的合法性
    assert(batchs_[bid]->state == State::kReady);    // batch必须是ready的状态
    assert(batchs_[bid]->parent_cid == slave_cid_);  // 父的从协程id必须正确
    if (batchs_[bid]->child_cid_2_finish.size() >= (size_t)max_concurrency_in_batch_) {
      return false;
    }
    int32_t cid = CoroutineCreate(forward<Function>(func), forward<Args>(args)...);
    assert(cid != kInvalidCid);
    coroutines_[cid]->relate_bid = bid;             // 设置关联的bid
    batchs_[bid]->child_cid_2_finish[cid] = false;  // 子的从协程都没执行完
    return true;
  }

  void BatchRun(int32_t bid);  // 运行批量执行

  void CoMutexInit(CoMutex &mutex);     // 互斥锁初始化
  void CoMutexClear(CoMutex &mutex);    // 互斥锁清理
  void CoMutexLock(CoMutex &mutex);     // 锁定互斥锁
  void CoMutexUnLock(CoMutex &mutex);   // 解锁互斥锁
  bool CoMutexTryLock(CoMutex &mutex);  // 尝试锁定互斥锁
  void CoMutexResume();

  void CoCondInit(CoCond &cond);   // 条件变量初始化
  void CoCondClear(CoCond &cond);  // 条件变量清理
  // 条件变量wait，支持设置超时时间的版本，需要配合Reactor模型的定时器功能来实现
  void CoCondWait(CoCond &cond, function<bool()> pred);
  void CoCondNotifyOne(CoCond &cond);  // 条件变量kNotifyOne
  void CoCondNotifyAll(CoCond &cond);  // 条件变量kNotifyAll
  int CoCondResume();

  void CoRWLockInit(CoRWLock &rwlock);      // 读写锁初始化
  void CoRWLockClear(CoRWLock &rwLock);     // 读写锁清理
  void CoRWLockWrLock(CoRWLock &rwlock);    // 加写锁
  void CoRWLockWrUnLock(CoRWLock &rwlock);  // 解写锁
  void CoRWLockRdLock(CoRWLock &rwlock);    // 加读锁
  void CoRWLockRdUnLock(CoRWLock &rwlock);  // 解读锁
  void CoRWLockResume();

  void CoSemaphoreInit(CoSemaphore &semaphore, int64_t value = 0);  // 信号量初始化
  void CoSemaphoreClear(CoSemaphore &semaphore);                // 信号量清理
  void CoSemaphorePost(CoSemaphore &semaphore);                 // （V 操作）释放信号量
  void CoSemaphoreWait(CoSemaphore &semaphore);                 // （P 操作）请求信号量
  int CoSemaphoreResume();

  void CoCallOnceInit(CoCallOnce &call_once);   // CallOnce初始化
  void CoCallOnceClear(CoCallOnce &call_once);  // CallOnce清理
  template <typename Function, typename... Args>
  void CoCallOnceDo(CoCallOnce &call_once, Function &&func, Args &&...args) {
    assert(not is_master_);
    if (call_once.state == CallOnceState::kInit) {
      call_once.state = CallOnceState::kInCall;
      func(forward<Args>(args)...);
      call_once.state = CallOnceState::kFinish;
      return;
    }
    while (call_once.state != CallOnceState::kFinish) {
      call_once.suspend_cid_set.insert(slave_cid_);
      CoroutineYield();
    }
    call_once.suspend_cid_set.erase(slave_cid_);
  }
  int CoCallOnceResume();

  void CoSingleFlightInit(CoSingleFlight &single_flight);
  void CoSingleFlightClear(CoSingleFlight &single_flight);
  template <typename Function, typename... Args>
  void CoSingleFlightDo(CoSingleFlight &single_flight, string key, Function &&func, Args &&...args) {
    assert(not is_master_);
    if (single_flights_.find(key) == single_flights_.end()) {
      single_flight.key = key;
      single_flights_[key] = &single_flight;
    }
    if (single_flight.state == SingleFlightState::kInit) {
      single_flight.state = SingleFlightState::kInCall;
      func(forward<Args>(args)...);
      single_flight.state = SingleFlightState::kFinish;
      return;
    }
    while (single_flight.state != SingleFlightState::kFinish) {
      single_flight.suspend_cid_set.insert(slave_cid_);
      CoroutineYield();
    }
    single_flight.suspend_cid_set.erase(slave_cid_);
  }
  int CoSingleFlightResume();

 private:
  static void CoroutineRun(Schedule *schedule, Coroutine *routine);  // 从协程的执行入口
  void CoroutineInit(Coroutine *routine, function<void()> entry);    // 从协程的初始化
  bool IsBatchDone(int32_t bid);                                     // 批量执行是否完成

 private:
  ucontext_t main_;                                         // 保存主协程的上下文
  bool is_master_{true};                                    // 是否主协程
  int32_t slave_cid_{kInvalidCid};                          // 运行的从协程的id（运行从协程时才有效）
  int32_t not_idle_count_{0};                               // 就绪、运行和挂起的从协程数
  int32_t coroutine_count_{0};                              // 从协程总数
  int32_t stack_size_{kStackSize};                          // 从协程栈大小，单位字节
  int32_t max_concurrency_in_batch_;                        // 一个Batch中最大的并发数
  Coroutine *coroutines_[kMaxCoroutineSize];                // 从协程数组池
  Batch *batchs_[kMaxBatchSize];                            // 批量执行数组池
  list<int> batch_finish_cid_list_;                         // 完成了批量执行的关联的从协程id
  unordered_set<CoMutex *> mutexs_;                         // 互斥锁集合
  unordered_set<CoCond *> conds_;                           // 条件变量集合
  unordered_set<CoRWLock *> rwlocks_;                       // 读写锁集合
  unordered_set<CoSemaphore *> semaphores_;                 // 信号量集合
  unordered_set<CoCallOnce *> call_onces_;                  // CallOnce集合
  unordered_map<string, CoSingleFlight *> single_flights_;  // SingleFlight映射
};
}  // namespace MyCoroutine
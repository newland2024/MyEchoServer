#pragma once

#include <ucontext.h>

#include <algorithm>
#include <functional>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <utility>

using namespace std;

namespace MyCoroutine {
constexpr int32_t kInvalidCid = -1;           // 无效的从协程id
constexpr int32_t kInvalidBid = -1;           // 无效的批量执行id
constexpr int32_t kStackSize = 64 * 1024;     // 协程栈默认大小为 64K
constexpr int32_t kMaxBatchSize = 5120;       // 允许创建的最大批量执行池大小
constexpr int32_t kMaxCoroutineSize = 10240;  // 允许创建的最大协程池大小
/**
 * 1.协程的状态机转移如下所示：
 *  kIdle->kReady
 *  kReady->kRun
 *  kRun->kSuspend
 *  kSuspend->kRun
 *  kRun->kIdle
 * 2.批量的并发执行状态机转移如下所示：
 *  kIdle->kReady
 *  kReady->kRun
 *  kRun->kIdle
 */
enum class State {
  kIdle = 1,     // 空闲
  kReady = 2,    // 就绪
  kRun = 3,      // 运行
  kSuspend = 4,  // 挂起
};

/**
 * 读写锁的状态，读写锁的状态转移如下：
 * kUnLock -> kWriteLock,kReadLock
 * kWriteLock -> kUnLock,kReadLock,kWriteLock
 * kReadLock -> kUnLock,kReadLock,kWriteLock
 */
enum class RWLockState {
  kUnLock = 1,     // 无锁状态
  kWriteLock = 2,  // 写锁锁定状态
  kReadLock = 3,   // 读锁锁定状态
};

/**
 * 读写锁类型
 */
enum class RWLockType {
  kWrite = 1,  // 写锁
  kRead = 2,   // 读锁
};

/**
 * CallOnce状态
 */
enum class CallOnceState {
  kInit = 1,    // 初始化
  kInCall = 2,  // 调用中
  kFinish = 3,  // 完成
};

/**
 * SingleFlight状态
 */
enum class SingleFlightState {
  kInit = 1,    // 初始化
  kInCall = 2,  // 调用中
  kFinish = 3,  // 完成
};

/**
 * 条件变量的状态，条件变量的状态转移如下：
 * kNotifyNone -> kNotifyOne,kNotifyAll
 * kNotifyOne -> kNotifyNone,kNotifyOne,kNotifyAll
 * kNotifyAll -> kNotifyNone,kNotifyAll
 */
enum class CondState {
  kNotifyNone = 1,  // 无通知
  kNotifyOne = 2,   // 通知一个等待者
  kNotifyAll = 3,   // 通知所有等待者
};

// 协程本地变量辅助结构体
typedef struct LocalVariable {
  void *data{nullptr};
  function<void(void *)> free{nullptr};  // 用于释放本地协程变量的内存
} LocalVariable;

// 批量并发执行结构体
typedef struct Batch {
  int32_t bid{kInvalidBid};                         // 批量执行id
  State state{State::kIdle};                        // 批量执行的状态
  int32_t parent_cid{kInvalidCid};                  // 父的从协程id
  unordered_map<int32_t, bool> child_cid_2_finish;  // 标记子的从协程是否执行完
} Batch;

// 协程互斥锁
typedef struct CoMutex {
  bool lock;                       // true表示被锁定，false表示被解锁
  int32_t hold_cid;                // 当前持有互斥锁的从协程id
  list<int32_t> suspend_cid_list;  // 因为等待互斥锁而被挂起的从协程id列表
} CoMutex;

// 协程读写锁
typedef struct CoRWLock {
  RWLockState lock_state;                    // 读写锁状态
  int32_t hold_write_cid;                    // 当前持有写锁的从协程id
  unordered_set<int32_t> hold_read_cid_set;  // 当前持有读锁的从协程id查重集合
  list<pair<RWLockType, int32_t>> suspend_list;  // 因为等待写锁而被挂起的从协程信息（锁类型+从协程id）
} CoRWLock;

// 协程条件变量
typedef struct CoCond {
  CondState state;                         // 条件变量状态
  unordered_set<int32_t> suspend_cid_set;  // 被挂起的从协程id查重集合
} CoCond;

// 协程信号量
typedef struct CoSemaphore {
  int64_t value;                          // 信号量的计数值
  unordered_set<int32_t> suspend_cid_set; // 被挂起的从协程id查重集合
} CoSemaphore;

// 协程Channel
typedef struct CoChannel {
  CoSemaphore idle; // 计数信号量，表示缓冲区中空闲出来的大小
  CoSemaphore fill; // 计数信号量，表示缓冲区中已被填充的大小
  list<void *> buffer; // 通用数据缓冲区
} CoChannel;

// 协程SingleFlight
typedef struct CoSingleFlight {
  string key;                              // SingleFlight key
  SingleFlightState state;                 // SingleFlight状态
  unordered_set<int32_t> suspend_cid_set;  // 被挂起的从协程id查重集合
} CoSingleFlight;

// 协程CallOnce
typedef struct CoCallOnce {
  CallOnceState state;                     // CallOnce状态
  unordered_set<int32_t> suspend_cid_set;  // 被挂起的从协程id查重集合
} CoCallOnce;

// 协程结构体
typedef struct Coroutine {
  int32_t cid{kInvalidCid};                    // 从协程id
  State state{State::kIdle};                   // 从协程当前的状态
  function<void()> entry{nullptr};             // 从协程入口函数
  ucontext_t ctx;                              // 从协程执行上下文
  uint8_t *stack{nullptr};                     // 每个协程独占的协程栈，动态分配
  unordered_map<void *, LocalVariable> local;  // 协程本地变量映射map，key是协程变量的内存地址
  int32_t relate_bid{kInvalidBid};             // 关联的批量执行id
} Coroutine;
};  // namespace MyCoroutine
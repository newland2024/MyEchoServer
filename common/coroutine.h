#pragma once

#include <stdlib.h>
#include <string.h>
#include <ucontext.h>

#include <cstdint>
#include <functional>
#include <list>
#include <unordered_map>
#include <unordered_set>

namespace MyCoroutine {

constexpr int kInvalidRoutineId = -1;  // 无效的协程id
constexpr int kMaxCoroutineSize = 10240;  // 最多创建10240个协程

/* 协程的状态，协程的状态转移如下：
 *  idle->ready
 *  ready->run
 *  run->suspend
 *  suspend->run
 *  run->idle
 */
enum State {
  Idle = 1,  // 空闲
  Ready = 2,  // 就绪
  Run = 3,  // 运行
  Suspend = 4,  // 挂起
};

/* 条件变量的状态，条件变量的状态转移如下：
 * notify_none -> notify_one,notify_all
 * notify_one -> notify_all
 * notify_all -> notify_all
 */
enum CondState {
  NotifyNone = 1,  // 无通知
  NotifyOne = 2,  // 通知一个等待者
  NotifyAll = 3,  // 通知所有等待者
};

enum ResumeResult {
  NotRunnable = 1,  // 无可运行的协程
  Success = 2,  // 成功唤醒一个挂起状态的协程
};

// 协程结构体
typedef struct Coroutine {
  State state;  // 协程当前的状态
  uint32_t priority;  // 协程优先级，值越小，优先级越高
  std::function<void()> entry;  // 协程入口函数
  ucontext_t ctx;  // 协程执行上下文
  uint8_t* stack;  // 每个协程独占的协程栈，动态分配
} Coroutine;

// 协程互斥量
typedef struct CoMutex {
  uint64_t id;  // 互斥锁id
  int cid;  // 当前持有互斥锁的从协程id
  bool lock;  // true表示被锁定，false表示被解锁
  std::unordered_set<int> suspend_id_set;  // 被挂起的从协程id查重集合
  std::list<int> suspend_id_list;  // 因为等待互斥量而被挂起的从协程id列表
} CoMutex;

// 协程互斥量管理器
typedef struct CoMutexManage {
  uint64_t alloc_id;  // 要分配的互斥量id
  std::unordered_map<uint64_t, CoMutex*> mutexs;
} CoMutexManage;

// 协程条件变量
typedef struct CoCond {
  uint64_t id;  // 条件变量id
  CondState state;  // 条件变量状态
  std::unordered_set<int> suspend_id_set;  // 被挂起的从协程id查重集合
  std::list<int> suspend_id_list;  // 因为等待条件变量而被挂起的从协程id列表
} CoCond;

// 协程条件变量管理器
typedef struct CoCondManage {
  uint64_t alloc_id;  // 要分配的条件变量id
  std::unordered_map<uint64_t, CoCond*> conds;
} CoCondManage;

// 协程调度器
typedef struct Schedule {
  ucontext_t main;  // 用于保存主协程的上下文
  int32_t runningCoroutineId;  // 运行中（Run + Suspend）的从协程的id
  int32_t coroutineCnt;  // 协程个数
  bool isMasterCoroutine;  // 当前协程是否为主协程
  Coroutine* coroutines[kMaxCoroutineSize];  // 从协程数组池
  int stackSize;  // 协程栈的大小，单位字节
  CoMutexManage mutexManage;  // 互斥量管理
  CoCondManage condManage;  // 条件变量管理
} Schedule;

// 协程初始化
void CoroutineInit(Schedule& schedule, Coroutine* routine, std::function<void()> entry);
// 创建协程
template <typename Function, typename... Args>
int CoroutineCreate(Schedule& schedule, Function&& f, Args&&... args) {
  int id = 0;
  for (id = 0; id < schedule.coroutineCnt; id++) {
    if (schedule.coroutines[id]->state == Idle) break;
  }
  if (id >= schedule.coroutineCnt) {
    return kInvalidRoutineId;
  }
  Coroutine* routine = schedule.coroutines[id];
  std::function<void()> entry = std::bind(std::forward<Function>(f), std::forward<Args>(args)...);
  CoroutineInit(schedule, routine, entry);
  return id;
}
// 让出执行权，只能在从协程中调用
void CoroutineYield(Schedule& schedule);
// 恢复一个从协程的调用，只能在主协程中调用
int CoroutineResume(Schedule& schedule);
// 恢复所有可以恢复的从协程的调用一遍，只能在主协程中调用
int CoroutineResumeAll(Schedule& schedule);
// 恢复指定从协程的调用，只能在主协程中调用
int CoroutineResumeById(Schedule& schedule, int id);

// 协程调度结构体初始化
int ScheduleInit(Schedule& schedule, int coroutineCnt, int stackSize = 8 * 1024);
// 判断是否还有协程在运行
bool ScheduleRunning(Schedule& schedule);
// 调度器主动驱动协程执行
void ScheduleRun(Schedule& schedule);
// 释放调度器
void ScheduleClean(Schedule& schedule);
// 互斥量初始化
void CoMutexInit(Schedule& schedule, CoMutex& mutex);
// 互斥量初清理
void CoMutexClear(Schedule& schedule, CoMutex& mutex);
// 互斥量锁定
void CoMutexLock(Schedule& schedule, CoMutex& mutex);
// 互斥量解锁
void CoMutexUnLock(Schedule& schedule, CoMutex& mutex);
// 条件变量初始化
void CoCondInit(Schedule& schedule, CoCond& cond);
// 条件变量清理
void CoCondClear(Schedule& schedule, CoCond& cond);
// 条件变量wait
void CoCondWait(Schedule& schedule, CoCond& cond);
// 条件变量notify_one
void CoCondNotifyOne(Schedule& schedule, CoCond& cond);
// 条件变量notify_all
void CoCondNotifyAll(Schedule& schedule, CoCond& cond);
}  // namespace MyCoroutine

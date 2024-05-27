#pragma once

#include <stdlib.h>
#include <string.h>
#include <ucontext.h>

#include <cstdint>
#include <functional>

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

// 协程调度器
typedef struct Schedule {
  ucontext_t main;  // 用于保存主协程的上下文
  int32_t runningCoroutineId;  // 运行中（Run + Suspend）的从协程的id
  int32_t coroutineCnt;  // 协程个数
  bool isMasterCoroutine;  // 当前协程是否为主协程
  Coroutine* coroutines[kMaxCoroutineSize];  // 从协程数组池
  int stackSize;  // 协程栈的大小，单位字节
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
// 恢复从协程的调用，只能在主协程中调用
int CoroutineResume(Schedule& schedule);
// 恢复指定从协程的调用，只能在主协程中调用
int CoroutineResumeById(Schedule& schedule, int id);

// 协程调度结构体初始化
int ScheduleInit(Schedule& schedule, int coroutineCnt, int stackSize = 8 * 1024);
// 判断是否还有协程在运行
bool ScheduleRunning(Schedule& schedule);
// 释放调度器
void ScheduleClean(Schedule& schedule);
}  // namespace MyCoroutine

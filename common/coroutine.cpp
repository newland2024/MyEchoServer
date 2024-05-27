#include "coroutine.h"

#include <assert.h>

#include <iostream>

namespace MyCoroutine {

static void CoroutineRun(Schedule* schedule) {
  schedule->isMasterCoroutine = false;
  int id = schedule->runningCoroutineId;
  assert(id >= 0 && id < schedule->coroutineCnt);
  Coroutine* routine = schedule->coroutines[id];
  // 执行entry函数
  routine->entry();
  // entry函数执行完之后，才能把协程状态更新为idle，并标记runningCoroutineId为无效的id
  routine->state = Idle;
  schedule->runningCoroutineId = kInvalidRoutineId;
  // 这个函数执行完，调用栈会回到主协程中，执行routine->ctx.uc_link指向的上下文的下一条指令
}

void CoroutineInit(Schedule& schedule, Coroutine* routine, std::function<void()> entry) {
  routine->entry = entry;
  routine->state = Ready;
  routine->priority = 0;
  routine->stack = new uint8_t[schedule.stackSize];

  getcontext(&(routine->ctx));
  routine->ctx.uc_stack.ss_flags = 0;
  routine->ctx.uc_stack.ss_sp = routine->stack;
  routine->ctx.uc_stack.ss_size = schedule.stackSize;
  routine->ctx.uc_link = &(schedule.main);
  // 设置routine->ctx上下文要执行的函数和对应的参数，
  // 这里没有直接使用entry和arg设置，而是多包了一层CoroutineRun函数的调用，
  // 是为了在CoroutineRun中entry函数执行完之后，从协程的状态更新Idle，并更新当前处于运行中的从协程id为无效id，
  // 这样这些逻辑就可以对上层调用透明。
  makecontext(&(routine->ctx), (void (*)(void))(CoroutineRun), 1, &schedule);
}

void CoroutineYield(Schedule& schedule) {
  assert(not schedule.isMasterCoroutine);
  int id = schedule.runningCoroutineId;
  assert(id >= 0 && id < schedule.coroutineCnt);
  Coroutine* routine = schedule.coroutines[schedule.runningCoroutineId];
  // 更新当前的从协程状态为挂起
  routine->state = Suspend;
  // 当前的从协程让出执行权，并把当前的从协程的执行上下文保存到routine->ctx中，
  // 执行权回到主协程中，主协程再做调度，当从协程被主协程resume时，swapcontext才会返回。
  swapcontext(&routine->ctx, &(schedule.main));
  schedule.isMasterCoroutine = false;
}

int CoroutineResume(Schedule& schedule) {
  assert(schedule.isMasterCoroutine);
  uint32_t priority = UINT32_MAX;
  int coroutineId = kInvalidRoutineId;
  // 按优先级调度，选择优先级最高的状态为可运行的从协程来运行
  for (int i = 0; i < schedule.coroutineCnt; i++) {
    if (schedule.coroutines[i]->state == Idle || schedule.coroutines[i]->state == Run) {
      continue;
    }
    // 执行到这里，schedule.coroutines[i]->state的值为 Suspend 或者 Ready
    if (schedule.coroutines[i]->priority < priority) {
      coroutineId = i;
      priority = schedule.coroutines[i]->priority;
    }
  }
  if (coroutineId == kInvalidRoutineId) return NotRunnable;
  Coroutine* routine = schedule.coroutines[coroutineId];
  routine->state = Run;
  schedule.runningCoroutineId = coroutineId;
  // 从主协程切换到协程编号为id的协程中执行，并把当前执行上下文保存到schedule.main中，
  // 当从协程执行结束或者从协程主动yield时，swapcontext才会返回。
  swapcontext(&schedule.main, &routine->ctx);
  schedule.isMasterCoroutine = true;
  return Success;
}

int CoroutineResumeById(Schedule& schedule, int id) {
  assert(schedule.isMasterCoroutine);
  assert(id >= 0 && id < schedule.coroutineCnt);
  Coroutine* routine = schedule.coroutines[id];
  // 挂起状态或者就绪状态的的协程才可以唤醒
  if (routine->state != Suspend && routine->state != Ready) return NotRunnable;
  routine->state = Run;
  schedule.runningCoroutineId = id;
  // 从主协程切换到协程编号为id的协程中执行，并把当前执行上下文保存到schedule.main中，
  // 当从协程执行结束或者从协程主动yield时，swapcontext才会返回。
  swapcontext(&schedule.main, &routine->ctx);
  schedule.isMasterCoroutine = true;
  return Success;
}

int ScheduleInit(Schedule& schedule, int coroutineCnt, int stackSize) {
  assert(coroutineCnt > 0 && coroutineCnt <= kMaxCoroutineSize);  // 最多创建kMaxCoroutineSize个协程
  schedule.stackSize = stackSize;
  schedule.isMasterCoroutine = true;
  schedule.coroutineCnt = coroutineCnt;
  schedule.runningCoroutineId = kInvalidRoutineId;
  for (int i = 0; i < coroutineCnt; i++) {
    schedule.coroutines[i] = new Coroutine;
    schedule.coroutines[i]->state = Idle;
    schedule.coroutines[i]->stack = nullptr;
  }
  return 0;
}

bool ScheduleRunning(Schedule& schedule) {
  assert(schedule.isMasterCoroutine);
  if (schedule.runningCoroutineId != kInvalidRoutineId) return true;
  for (int i = 0; i < schedule.coroutineCnt; i++) {
    if (schedule.coroutines[i]->state != Idle) return true;
  }
  return false;
}

void ScheduleClean(Schedule& schedule) {
  assert(schedule.isMasterCoroutine);
  for (int i = 0; i < schedule.coroutineCnt; i++) {
    delete[] schedule.coroutines[i]->stack;
    delete schedule.coroutines[i];
  }
}
}  // namespace MyCoroutine
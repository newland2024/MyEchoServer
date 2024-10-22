#pragma once

#include "common.h"
#include "mycoroutine.h"

namespace MyCoroutine {
// 协程Channel
template <typename Type> 
class Channel {
public:
  Channel(Schedule &schedule, int64_t buffer_size = 0) : schedule_(schedule) {
    schedule_.CoSemaphoreInit(co_channel_.idle, buffer_size);
    schedule_.CoSemaphoreInit(co_channel_.fill);
  }
  ~Channel() {
    schedule_.CoSemaphoreClear(co_channel_.idle);
    schedule_.CoSemaphoreClear(co_channel_.fill);
  }
  void Send(Type *data) {
    // 等待缓冲区空闲
    schedule_.CoSemaphoreWait(co_channel_.idle);
    co_channel_.buffer.push_back(data);
    // 发送缓存区被填充信号
    schedule_.CoSemaphorePost(co_channel_.fill);
  }
  Type *Receive() {
    Type *data;
    // 等待缓存区被填充
    schedule_.CoSemaphoreWait(co_channel_.fill);
    data = (Type *)co_channel_.buffer.front();
    co_channel_.buffer.pop_front();
    // 发送缓冲区空闲信号
    schedule_.CoSemaphorePost(co_channel_.idle);
    return data;
  }

private:
  CoChannel co_channel_;
  Schedule &schedule_;
};
} // namespace MyCoroutine

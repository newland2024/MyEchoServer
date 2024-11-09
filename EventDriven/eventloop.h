#pragma once

#include <sys/epoll.h>
#include <unistd.h>

#include <cstdint>
#include <list>
#include <string>
#include <unordered_map>
#include <utility>

#include "epollctl.hpp"
#include "event.hpp"
#include "socket.hpp"
#include "timer.hpp"

using namespace std;

namespace EventDriven {
class EventLoop {
 public:
  EventLoop();

  template <typename Function, typename... Args>
  void TcpReadStart(Event *event, Function &&handler, Args &&...args) {
    EventSetUp(event, EventType::kRead);
    event->handler = std::bind(std::forward<Function>(handler), std::forward<Args>(args)...);
    EpollCtl::AddReadEvent(event->epoll_fd, event->fd, event);
  }

  template <typename Function, typename... Args>
  void TcpWriteStart(Event *event, Function &&handler, Args &&...args) {
    EventSetUp(event, EventType::kWrite);
    event->handler = std::bind(std::forward<Function>(handler), std::forward<Args>(args)...);
    EpollCtl::AddWriteEvent(event->epoll_fd, event->fd, event);
  }

  template <typename Function, typename... Args>
  void TcpModToReadStart(Event *event, Function &&handler, Args &&...args) {
    EventSetUp(event, EventType::kRead);
    event->handler = std::bind(std::forward<Function>(handler), std::forward<Args>(args)...);
    EpollCtl::ModToReadEvent(event->epoll_fd, event->fd, event);
  }

  template <typename Function, Event *event, typename... Args>
  void TcpModToWriteStart(Function &&handler, Args &&...args) {
    EventSetUp(event, EventType::kWrite);
    event->handler = std::bind(std::forward<Function>(handler), std::forward<Args>(args)...);
    EpollCtl::ModToWriteEvent(event->epoll_fd, event->fd, event);
  }

  template <typename Function, typename... Args>
  uint64_t TimerStart(int64_t time_out_ms, Function &&handler, Args &&...args) {
    return timer_.Register(time_out_ms, forward<Function>(handler), forward<Args>(args)...);
  }

  void Run();
  void Stop();

 private:
  void EventSetUp(Event *event, EventType event_type);

 private:
  Timer timer_;      // 定时器
  int epoll_fd_;     // epoll的fd
  bool is_running_;  // 是否运行中
};
}  // namespace EventDriven
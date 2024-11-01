#pragma once

#include "epollctl.hpp"
#include "event.hpp"
#include "socket.hpp"
#include "timer.hpp"
#include <cstdint>
#include <list>
#include <string>
#include <sys/epoll.h>
#include <unistd.h>
#include <unordered_map>
#include <utility>

using namespace std;

namespace EventDriven {
class EventLoop {
public:
  EventLoop();
  ~EventLoop();

  template <typename Function, typename... Args>
  void TcpReadStart(int fd, Function &&handler, Args &&...args) {
    Event *event = createEvent(EventType::kRead, fd);
    auto temp = std::bind(forward<Function>(handler), event, std::placeholders::_1);
    auto can_read = std::bind(temp, forward<Args>(args)...);
    event->handler = can_read;
    EpollCtl::AddReadEvent(event->epoll_fd, event->fd, event);
  }

  template <typename Function, typename... Args>
  void TcpWriteStart(int fd, Function &&handler, Args &&...args) {
    Event *event = createEvent(EventType::kWrite, fd);
    auto temp = std::bind(forward<Function>(handler), event, std::placeholders::_1);
    auto can_write = std::bind(temp, forward<Args>(args)...);
    event->handler = can_write;
    EpollCtl::AddWriteEvent(event->epoll_fd, event->fd, event);
  }

  template <typename Function, typename... Args>
  void TcpModToReadStart(int fd, Function &&handler, Args &&...args) {
    Event *event = createEvent(EventType::kRead, fd);
    auto temp = std::bind(forward<Function>(handler), event, std::placeholders::_1);
    auto can_read = std::bind(temp, forward<Args>(args)...);
    event->handler = can_read;
    EpollCtl::ModToReadEvent(event->epoll_fd, event->fd, event);
  }

  template <typename Function, typename... Args>
  void TcpModToWriteStart(int fd, Function &&handler, Args &&...args) {
    Event *event = createEvent(EventType::kWrite, fd);
    auto temp = std::bind(forward<Function>(handler), event, std::placeholders::_1);
    auto can_write = std::bind(temp, forward<Args>(args)...);
    event->handler = can_write;
    EpollCtl::ModToWriteEvent(event->epoll_fd, event->fd, event);
  }

  template <typename Function, typename... Args>
  uint64_t TimerStart(int64_t time_out_ms, Function &&handler, Args &&...args) {
    return timer_.Register(time_out_ms, forward<Function>(handler),
                           forward<Args>(args)...);
  }

  void Run();
  void Stop();

private:
  Event *createEvent(EventType event_type, int fd,
                     function<void(Event *event)> handler);

private:
  Timer timer_;                 // 定时器
  int epoll_fd_;                // epoll的fd
  bool is_running_;             // 是否运行中
  list<Event *> listen_events_; // 为监听而创建的事件对象列表
};
} // namespace EventDriven
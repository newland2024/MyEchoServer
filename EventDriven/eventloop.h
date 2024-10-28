#pragma once

#include "epollctl.hpp"
#include "event.hpp"
#include "timer.hpp"
#include "socket.hpp"
#include <cstdint>
#include <string>
#include <sys/epoll.h>
#include <unistd.h>
#include <unordered_map>
#include <utility>
#include <list>

using namespace std;

namespace EventDriven {
class EventLoop {
public:
  EventLoop();
  ~EventLoop();
  
  void TcpListenStart(string ip, uint16_t port, function<void(Event * event)> accept_client);
  void TcpConnectStart(string ip, uint16_t port, int64_t time_out_ms, function<void(Event *event)> client_connect);
  void TcpReadStart(int fd, int64_t time_out_ms, function<void(Event *event)> can_read);
  void TcpWriteStart(int fd, int64_t time_out_ms, function<void(Event *event)> can_write);

  void UnixSocketListenStart(string path, int64_t time_out_ms, function<void(Event *event)> accept_client);
  void UnixSocketConnectStart(string path, int64_t time_out_ms, function<void(Event *event)> client_connect);
  void UnixSocketReadStart(int fd, int64_t time_out_ms, function<void(Event *event)> can_read);
  void UinxSocketWriteStart(int fd, int64_t time_out_ms, function<void(Event *event)> can_write);

  template <typename Function, typename... Args>
  uint64_t TimerStart(int64_t time_out_ms, Function && handler, Args &&...args) {
    return timer_.Register(time_out_ms, forward<Function>(handler), forward<Args>(args)...);
  }

  void Run();
  void Stop();

private:
  Timer timer_;                 // 定时器
  int epoll_fd_;                // epoll的fd
  bool is_running_;             // 是否运行中
  list<Event *> listen_events_; // 为监听而创建的事件对象列表
};
} // namespace EventDriven
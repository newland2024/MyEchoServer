#include "eventloop.h"

namespace EventDriven {
void EventLoop::UnixSocketListenStart(
    string path, int64_t time_out_ms,
    function<void(Event *event)> accept_client) {
  // TODO
}
void EventLoop::UnixSocketConnectStart(
    string path, int64_t time_out_ms,
    function<void(Event *event)> client_connect) {
  // TODO
}
void EventLoop::UnixSocketReadStart(int fd, int64_t time_out_ms,
                                    function<void(Event *event)> can_read) {
  // TODO
}
void EventLoop::UinxSocketWriteStart(int fd, int64_t time_out_ms,
                                     function<void(Event *event)> can_write) {
  // TODO
}
} // namespace EventDriven
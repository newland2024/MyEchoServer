#pragma once

#include "utils.hpp"

namespace MyEcho {
class Conn {
 public:
  Conn(int fd, int epoll_fd, bool is_multi_io) : fd_(fd), epoll_fd_(epoll_fd), is_multi_io_(is_multi_io) {}
  Conn(int fd, bool is_multi_io) : fd_(fd), epoll_fd_(0), is_multi_io_(is_multi_io) {}
  bool Read() {
    do {
      if (codec_.GetDecodeStatus() == FINISH) return true;
      ssize_t ret = read(fd_, codec_.Data(), codec_.Len());
      if (ret == 0) {
        perror("peer close connection");
        return false;
      }
      if (ret < 0) {
        if (EINTR == errno) continue;
        if (EAGAIN == errno or EWOULDBLOCK == errno) return true;
        perror("read failed");
        return false;
      }
      codec_.DeCode((size_t)ret);
    } while (is_multi_io_);
    return true;
  }
  bool Write() {
    do {
      if (send_len_ == pkt_.UseLen()) return true;
      ssize_t ret = write(fd_, pkt_.Data() + send_len_, pkt_.UseLen() - send_len_);
      if (ret < 0) {
        if (EINTR == errno) continue;
        if (EAGAIN == errno && EWOULDBLOCK == errno) return true;
        perror("write failed");
        return false;
      }
      send_len_ += ret;
    } while (is_multi_io_);
    return true;
  }
  bool OneMessage() {
    bool get_one{false};
    std::string* temp = codec_.GetMessage();
    if (temp) {
      get_one = true;
      message_ = *temp;
      delete temp;
    }
    return get_one;
  }
  void EnCode() { codec_.EnCode(message_, pkt_); }
  bool FinishWrite() { return send_len_ == pkt_.UseLen(); }
  int Fd() { return fd_; }
  int EpollFd() { return epoll_fd_; }
  void Reset() {
    codec_.Reset();
    send_len_ = 0;
    pkt_.Reset();
  }
  bool IsUnixSocket() { return is_unix_socket_; }
  void SetUnixSocket() { is_unix_socket_ = true; }

 private:
  int fd_{0};  // 关联的fd
  int epoll_fd_{0};  // 关联的epoll实例的fd
  bool is_multi_io_;  // 是否做多次io，直到返回EAGAIN或者EWOULDBLOCK
  size_t send_len_{0};  // 要发送的应答数据的长度
  std::string message_;  // 对于EchoServer来说，即是获取的请求消息，也是要发送的应答消息
  Packet pkt_;  // 发送应答消息的二进制数据包
  Codec codec_;  // EchoServer协议的编解码
  bool is_unix_socket_{false};
};
}  // namespace MyEcho
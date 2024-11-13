#pragma once

#include <string>

#include "Coroutine/mycoroutine.h"
#include "EventDriven/eventloop.h"
#include "EventDriven/socket.hpp"
#include "common/codec.hpp"
#include "common/packet.hpp"

namespace BenchMark2 {
class Defer {
 public:
  Defer(std::function<void(void)> func) : func_(func) {}
  ~Defer() { func_(); }

 private:
  std::function<void(void)> func_;
};

class Client {
 public:
  Client(MyCoroutine::Schedule &schedule, EventDriven::EventLoop &event_loop, std::string ip, int port,
         std::string echo_message, int64_t &temp_rate_limit)
      : schedule_(schedule), event_loop_(event_loop), temp_rate_limit_(temp_rate_limit) {
    cid_ = schedule_.CoroutineCreate(Client::Run, std::ref(*this), ip, port, echo_message);
  }
  static void Run(Client &client, std::string ip, int port,
                  std::string echo_message) {  // 启动整个请求循环，在从协程中执行
    while (client.IsRunning()) {
      client.temp_rate_limit_--;
      if (client.temp_rate_limit_ <= 0) {  // 已经触达每秒的限频，则暂停请求
        client.is_stop_ = true;
        client.schedule_.CoroutineYield();
      }
      if (client.success_count_ < 3) {
        // 创建连接
        client.TryConnect(ip, port);
        // 发起请求
        EventDriven::Event event(client.fd_);
        client.event_loop_.TcpWriteStart(&event, EventCallBack, std::ref(client.schedule_), client.cid_);
        client.SendRequest(echo_message);
        // 接收应答
        client.event_loop_.TcpModToReadStart(&event, EventCallBack, std::ref(client.schedule_), client.cid_);
        client.RecvResponse(echo_message);
        client.event_loop_.TcpEventClear(client.fd_);
      }
    }
  }

  static void EventCallBack(MyCoroutine::Schedule &schedule, int32_t cid) { schedule.CoroutineResume(cid); }

  static void TimeOutCallBack(MyCoroutine::Schedule &schedule, int32_t cid, bool &is_time_out) {
    is_time_out = true;
    schedule.CoroutineResume(cid);
  }

  void TryConnect(std::string ip, int port) {
    if (fd_ >= 0) {
      return;
    }
    if (CoConnect(ip, port, 100)) {  // 建立连接，超时时间100ms
      EventDriven::Socket::SetCloseWithRst(fd_);
      return;
    }
    // 执行到这里，连接失败
    close(fd_);
    fd_ = -1;
    return;
  }

  void SendRequest(const std::string &echo_message) {
    if (fd_ < 0) {
      return;
    }
    MyEcho::Codec codec;
    MyEcho::Packet pkt;
    codec.EnCode(echo_message, pkt);
    size_t sendLen = 0;
    bool send_result = true;
    while (sendLen != pkt.UseLen()) {  // 写操作
      ssize_t ret = CoWrite(pkt.Data() + sendLen, pkt.UseLen() - sendLen, 1000);
      if (ret < 0) {
        if (EINTR == errno) continue;  // 被中断，可以重启写操作
        send_result = false;
        break;
      }
      sendLen += ret;
    }
    if (not send_result) {
      close(fd_);
      fd_ = -1;
      // TODO 统计相关
      return;
    }
    // TODO 统计相关
  }

  void RecvResponse(const std::string &echo_message) {
    if (fd_ < 0) {
      return;
    }
    MyEcho::Codec codec;
    bool recv_result = true;
    string *resp_message{nullptr};
    while (true) {
      ssize_t ret = CoRead(codec.Data(), codec.Len(), 1000);
      if (0 == ret) {
        recv_result = false;
        break;
      }
      if (ret < 0) {
        if (EINTR == errno) continue;  // 被中断，可以重启读操作
        if (EAGAIN == errno) {
          recv_result = false;
          break;
        }
      }
      codec.DeCode(ret);  // 解析请求数据
      resp_message = codec.GetMessage();
      if (resp_message) {  // 解析出一个完整的请求
        break;
      }
    }
    if (not recv_result) {
      close(fd_);
      fd_ = -1;
      // TODO 统计相关
      return;
    }
    if (echo_message != *resp_message) {
      // TODO
    } else {
      success_count_++;
      cout << "echo req success. " << endl;
    }
    delete resp_message;
    // TODO 统计相关
  }

  bool CoConnect(std::string ip, int port, int64_t time_out_ms) {
    int ret = EventDriven::Socket::Connect(ip, port, fd_);
    if (0 == ret) {  // 创建连接成功
      return true;
    }
    if (ret == EINPROGRESS) {
      bool is_time_out{false};
      uint64_t timer_id = event_loop_.TimerStart(time_out_ms, TimeOutCallBack, std::ref(schedule_), cid_, std::ref(is_time_out));
      EventDriven::Event event(fd_);
      event_loop_.TcpWriteStart(&event, EventCallBack, std::ref(schedule_),
                                cid_);
      Defer defer([this, &is_time_out, timer_id]() {
        event_loop_.TcpEventClear(fd_);
        if (not is_time_out) {
          event_loop_.TimerCancel(timer_id);
        }
      });
      schedule_.CoroutineYield();
      if (is_time_out) { // 连接超时了
        return false;
      }
      return EventDriven::Socket::IsConnectSuccess(fd_);
    }
    // 执行到这里连接失败
    return false;
  }

  ssize_t CoRead(uint8_t *buf, size_t size, int64_t time_out_ms) {
    bool is_time_out{false};
    uint64_t timer_id =
        event_loop_.TimerStart(time_out_ms, TimeOutCallBack, std::ref(schedule_), cid_, std::ref(is_time_out));
    Defer defer([&is_time_out, this, timer_id]() {
      if (is_time_out) {  // 读超时了
        event_loop_.TcpEventClear(fd_);
      } else {
        event_loop_.TimerCancel(timer_id);
      }
    });
    while (true) {
      ssize_t ret = read(fd_, buf, size);
      if (ret >= 0) return ret;                       // 读成功
      if (EINTR == errno) continue;                   // 调用被中断，则直接重启read调用
      if (EAGAIN == errno or EWOULDBLOCK == errno) {  // 暂时不可读
        schedule_.CoroutineYield();                   // 让出cpu，切换到主协程，等待下一次数据可读
        if (is_time_out) {                            // 读超时了
          errno = EAGAIN;
          return -1;  // 读超时，返回-1，并把errno设置为EAGAIN
        }
        continue;
      }
      return ret;  // 读失败
    }
  }

  ssize_t CoWrite(uint8_t *buf, size_t size, int64_t time_out_ms) {
    bool is_time_out{false};
    uint64_t timer_id =
        event_loop_.TimerStart(time_out_ms, TimeOutCallBack, std::ref(schedule_), cid_, std::ref(is_time_out));
    Defer defer([&is_time_out, this, timer_id]() {
      if (is_time_out) {  // 写超时了
        event_loop_.TcpEventClear(fd_);
      } else {
        event_loop_.TimerCancel(timer_id);
      }
    });
    while (true) {
      ssize_t ret = write(fd_, buf, size);
      if (ret >= 0) return ret;                       // 写成功
      if (EINTR == errno) continue;                   // 调用被中断，则直接重启write调用
      if (EAGAIN == errno or EWOULDBLOCK == errno) {  // 暂时不可写
        schedule_.CoroutineYield();                   // 让出cpu，切换到主协程，等待下一次数据可写
        if (is_time_out) {                            // 写超时了
          errno = EAGAIN;
          return -1;  // 写超时，返回-1，并把errno设置为EAGAIN
        }
        continue;
      }
      return ret;  // 写失败
    }
  }

  void InitStart() { schedule_.CoroutineResume(cid_); }
  void ReStart() {
    if (not is_stop_) {
      return;
    }
    is_stop_ = false;
    schedule_.CoroutineResume(cid_);
  }
  void Stop() { is_running_ = false; }
  void IsRunning() { return is_running_; }
  

 private:
  MyCoroutine::Schedule &schedule_;
  EventDriven::EventLoop &event_loop_;
  int32_t cid_;
  int64_t &temp_rate_limit_;
  bool is_stop_{false};
  int fd_{-1};
  int64_t success_count_{0};
  bool is_running_{true};
};
}  // namespace BenchMark2
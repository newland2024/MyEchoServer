#pragma once

#include <string>

#include "Coroutine/mycoroutine.h"
#include "EventDriven/eventloop.h"
#include "EventDriven/socket.hpp"
#include "common/codec.hpp"
#include "common/packet.hpp"
#include "common/percentile.hpp"
#include "common/stat.hpp"
#include "common/utils.hpp"

namespace BenchMark2 {
class Defer {
 public:
  Defer(std::function<void(void)> func) : func_(func) {}
  ~Defer() { func_(); }

 private:
  std::function<void(void)> func_;
};

typedef struct ClientStat {
  int64_t try_connect_count{0};      // 尝试连接的次数
  int64_t success_count{0};          // 成功次数
  int64_t failure_count{0};          // 失败次数（包括读写失败和连接失败）
  int64_t read_failure_count{0};     // 细分的失败统计，读失败数
  int64_t write_failure_count{0};    // 细分的失败统计，写失败数
  int64_t connect_failure_count{0};  // 细分的失败统计，连接失败数
} ClientStat;

class Client {
 public:
  Client(MyCoroutine::Schedule &schedule, EventDriven::EventLoop &event_loop, std::string ip, int port,
         std::string echo_message, int64_t max_req_count, int64_t &temp_rate_limit, SumStat &sum_stat,
         PctStat &pct_stat, Percentile &percentile)
      : schedule_(schedule), event_loop_(event_loop), max_req_count_(max_req_count), temp_rate_limit_(temp_rate_limit),
        sum_stat_(sum_stat), pct_stat_(pct_stat), percentile_(percentile) {
    cid_ = schedule_.CoroutineCreate(Client::Run, std::ref(*this), ip, port, echo_message);
  }
  static void Run(Client &client, std::string ip, int port,
                  std::string echo_message) {  // 启动整个请求循环，在从协程中执行
    while (client.IsRunning()) {
      client.temp_rate_limit_--;
      if (client.temp_rate_limit_ < 0) {  // 已经触达每秒的限频，则暂停请求
        client.is_stop_ = true;
        client.schedule_.CoroutineYield();
        continue;
      }
      // 创建连接
      client.TryConnect(ip, port);
      int64_t req_begin_time{0};
      // 发起请求
      if (client.fd_ > 0) {
        req_begin_time = MyEcho::GetCurrentTimeUs();
        EventDriven::Event event(client.fd_);
        client.event_loop_.TcpWriteStart(&event, EventCallBack, std::ref(client.schedule_), client.cid_);
        client.SendRequest(echo_message);
      }
      // 接收应答
      if (client.fd_ > 0) {
        EventDriven::Event event(client.fd_);
        client.event_loop_.TcpModToReadStart(&event, EventCallBack, std::ref(client.schedule_), client.cid_);
        client.RecvResponse(req_begin_time, echo_message);
        client.event_loop_.TcpEventClear(client.fd_);
      }
    }
    assert(client.stat_.failure_count ==
           (client.stat_.connect_failure_count + client.stat_.read_failure_count + client.stat_.write_failure_count));
    client.sum_stat_.DoStat(client.stat_.success_count, client.stat_.failure_count, client.stat_.read_failure_count,
                            client.stat_.write_failure_count, client.stat_.connect_failure_count,
                            client.stat_.try_connect_count);
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
    int64_t begin_time = MyEcho::GetCurrentTimeUs();
    stat_.try_connect_count++;
    // 建立连接，超时时间100ms
    if (CoConnect(ip, port, 100)) {
      percentile_.ConnectSpendTimeStat(MyEcho::GetCurrentTimeUs() - begin_time);  // 统计连接超时时间
      EventDriven::Socket::SetCloseWithRst(fd_);
      return;
    }
    // 执行到这里，连接失败
    stat_.failure_count++;
    stat_.connect_failure_count++;
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
      stat_.failure_count++;
      stat_.write_failure_count++;
      return;
    }
  }

  void RecvResponse(int64_t req_begin_time, const std::string &echo_message) {
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
      stat_.failure_count++;
      stat_.read_failure_count++;
      return;
    }
    assert(echo_message == *resp_message);
    delete resp_message;
    stat_.success_count++;
    percentile_.InterfaceSpendTimeStat(MyEcho::GetCurrentTimeUs() - req_begin_time);
    double pct50{0}, pct95{0}, pct99{0}, pct999{0};
    if (percentile_.TryPrintSpendTimePctData(pct50, pct95, pct99, pct999)) {
      pct_stat_.InterfaceSpendTimeStat(pct50, pct95, pct99, pct999);
    }
    if (stat_.success_count % max_req_count_ == 0) {  // 达到单次连接最大请求数
      close(fd_);
      fd_ = -1;
    }
  }

  bool CoConnect(std::string ip, int port, int64_t time_out_ms) {
    int ret = EventDriven::Socket::Connect(ip, port, fd_);
    if (0 == ret) {  // 创建连接成功
      return true;
    }
    if (ret == EINPROGRESS) {
      bool is_time_out{false};
      uint64_t timer_id =
          event_loop_.TimerStart(time_out_ms, TimeOutCallBack, std::ref(schedule_), cid_, std::ref(is_time_out));
      EventDriven::Event event(fd_);
      event_loop_.TcpWriteStart(&event, EventCallBack, std::ref(schedule_), cid_);
      Defer defer([this, &is_time_out, timer_id]() {
        event_loop_.TcpEventClear(fd_);
        if (not is_time_out) {
          event_loop_.TimerCancel(timer_id);
        }
      });
      schedule_.CoroutineYield();
      if (is_time_out) {  // 连接超时了
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
  void SetFinish() { is_running_ = false; }
  bool IsRunning() { return is_running_; }

  int64_t GetCurrentTimeUs() {
    struct timeval current;
    gettimeofday(&current, NULL);
    return current.tv_sec * 1000000 + current.tv_usec;  // 计算运行的时间，单位微秒
  }

 private:
  MyCoroutine::Schedule &schedule_;
  EventDriven::EventLoop &event_loop_;
  int fd_{-1};
  int32_t cid_{-1};
  int64_t max_req_count_;
  int64_t &temp_rate_limit_;
  bool is_stop_{false};
  bool is_running_{true};
  ClientStat stat_;
  SumStat &sum_stat_;       // 汇总统计（全局）
  PctStat &pct_stat_;       // pct统计（全局）
  Percentile &percentile_;  // 用于统计请求耗时的pctxx数值（线程各自一份）
};
}  // namespace BenchMark2
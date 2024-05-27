#pragma once

#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include <iostream>
#include <string>

#include "../common/codec.hpp"
#include "../common/epollctl.hpp"
#include "../common/utils.hpp"
#include "percentile.hpp"
#include "stat.hpp"

using namespace std;

namespace BenchMark {
enum ClientStatus {
  Init = 1,  // 初始状态
  Connecting = 2,  // 连接中
  ConnectSuccess = 3,  // 连接成功
  SendRequest = 4,  // 发送请求
  RecvResponse = 5,  // 接收应答
  Success = 6,  // 成功处理完一次请求
  Failure = 7,  // 失败（每个状态都可能跳转到这个状态）
  Finish = 8,  // 客户端read返回了0，或者执行完了指定次数的请求
  Stop = 9,  // 因为限频而停止发送请求
};

class EchoClient {
 public:
  EchoClient(int epoll_fd, Percentile* percentile, bool is_debug, int max_req_count, int64_t* temp_rate_limit,
             PctStat* pct_stat)
      : epoll_fd_(epoll_fd),
        percentile_(percentile),
        is_debug_(is_debug),
        max_req_count_(max_req_count),
        temp_rate_limit_(temp_rate_limit),
        pct_stat_(pct_stat) {
    if (max_req_count_ <= 0) {
      max_req_count_ = 100;
    }
    if (is_debug_) {
      cout << "max_req_count=" << max_req_count_ << endl;
    }
  }
  int Fd() { return fd_; }
  std::string GetStatus() {
    if (Init == status_) {
      return "Init";
    }
    if (Connecting == status_) {
      return "Connecting";
    }
    if (ConnectSuccess == status_) {
      return "ConnectSuccess";
    }
    if (SendRequest == status_) {
      return "SendRequest";
    }
    if (RecvResponse == status_) {
      return "RecvResponse";
    }
    if (Success == status_) {
      return "Success";
    }
    if (Failure == status_) {
      return "Failure";
    }
    if (Finish == status_) {
      return "Finish";
    }
    return "Unknow";
  }
  void SetEchoMessage(const std::string& message) {
    send_message_ = message;
    codec_.EnCode(message, send_pkt_);
  }
  void GetDealStat(int64_t& success_count, int64_t& failure_count, int64_t& connect_failure_count,
                   int64_t& read_failure_count, int64_t& write_failure_count, int64_t& try_connect_count) {
    try_connect_count += try_connect_count_;
    success_count += success_count_;
    failure_count += failure_count_;
    connect_failure_count += connect_failure_count_;
    read_failure_count += read_failure_count_;
    write_failure_count += write_failure_count_;
  }
  bool IsValid() {
    if (Failure == status_ || Finish == status_) {
      return false;
    }
    bool is_valid = true;
    // connect超时，超时时间100ms
    if (Connecting == status_) {
      if ((GetCurrentTimeUs() - last_connect_time_us_) / 1000 >= 100) {
        is_valid = false;
        failure_count_++;
        connect_failure_count_++;
      }
    }
    // 写超时，超时时间1000ms
    if (SendRequest == status_ && (GetCurrentTimeUs() - last_send_req_time_us_) / 1000 >= 1000) {
      is_valid = false;
      failure_count_++;
      write_failure_count_++;
    }
    // 读超时，超时时间1000ms
    if (RecvResponse == status_ && (GetCurrentTimeUs() - last_recv_resp_time_us_) / 1000 >= 1000) {
      is_valid = false;
      failure_count_++;
      read_failure_count_++;
    }
    if (not is_valid) {
      ClearEvent(epoll_fd_, fd_);
    }
    return is_valid;
  }
  void TryRestart() {
    if (status_ != Stop) {
      return;
    }
    if (*temp_rate_limit_ <= 0) {
      return;
    }
    status_ = SendRequest;
    last_send_req_time_us_ = GetCurrentTimeUs();
    AddWriteEvent(epoll_fd_, fd_, this);
  }
  void Connect(const std::string& ip, int64_t port) {
    try_connect_count_++;
    fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ < 0) {
      status_ = Failure;
      failure_count_++;
      connect_failure_count_++;
      return;
    }
    MyEcho::SetNotBlock(fd_);  // 设置成非阻塞的
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(int16_t(port));
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    int ret = connect(fd_, (struct sockaddr*)&addr, sizeof(addr));
    if (0 == ret) {
      status_ = ConnectSuccess;
      AddWriteEvent(epoll_fd_, fd_, this);  // 监控可写事件
      if (is_debug_) {
        cout << "connect success" << endl;
      }
      return;
    }
    if (errno == EINPROGRESS) {
      status_ = Connecting;
      last_connect_time_us_ = GetCurrentTimeUs();
      AddWriteEvent(epoll_fd_, fd_, this);  // 监控可写事件
      return;
    }
    status_ = Failure;
    failure_count_++;
    connect_failure_count_++;
    return;
  }
  bool Deal() {  // 本函数实现状态机的流转
    bool result{false};
    if (is_debug_) {
      cout << "deal before status_ = " << GetStatus() << endl;
    }
    if (Connecting == status_) {  // 状态转移分支 -> (ConnectSuccess, Failure)
      result = checkConnect();
    } else if (ConnectSuccess == status_) {  // 状态转移分支 -> (SendRequest, RecvResponse, Stop, Failure)
      result = sendRequest();
    } else if (SendRequest == status_) {  // 状态转移分支 -> (SendRequest, RecvResponse, Stop, Failure)
      result = sendRequest();
    } else if (RecvResponse == status_) {  // 状态转移分支 -> (RecvResponse, Finish, Success, Failure)
      result = recvResponse();
    } else if (Success == status_) {  // 状态转移分支 -> (SendRequest, Finish, Failure)
      result = dealSuccess();
    }
    if (not result) {
      status_ = Failure;
      failure_count_++;
      ClearEvent(epoll_fd_, fd_);
    }
    if (is_debug_) {
      cout << "deal after status_ = " << GetStatus() << endl;
    }
    return result;
  }

 private:
  void setLinger() {
    struct linger lin;
    lin.l_onoff = 1;
    lin.l_linger = 0;
    // 设置调用close关闭tcp连接时，直接发送RST包，tcp连接直接复位，进入到closed状态。
    assert(0 == setsockopt(fd_, SOL_SOCKET, SO_LINGER, &lin, sizeof(lin)));
  }
  bool checkConnect() {
    int err = 0;
    socklen_t errLen = sizeof(err);
    assert(0 == getsockopt(fd_, SOL_SOCKET, SO_ERROR, &err, &errLen));
    if (0 == err) {
      setLinger();
      status_ = ConnectSuccess;
      int64_t connect_spend_time = GetCurrentTimeUs() - last_connect_time_us_;
      percentile_->ConnectSpendTimeStat(connect_spend_time);
      if (is_debug_) {
        cout << "connect success. connect_spend_time=" << connect_spend_time << endl;
      }
    } else {
      connect_failure_count_++;
    }
    return 0 == err;
  }
  bool sendRequest() {
    if (*temp_rate_limit_ <= 0) {
      status_ = Stop;
      ClearEvent(epoll_fd_, fd_, false);
      return true;
    }
    (*temp_rate_limit_)--;
    status_ = SendRequest;
    last_send_req_time_us_ = GetCurrentTimeUs();
    ssize_t ret = write(fd_, send_pkt_.Data() + send_len_, send_pkt_.UseLen() - send_len_);
    if (ret < 0) {
      if (EINTR == errno || EAGAIN == errno || EWOULDBLOCK == errno) return true;
      write_failure_count_++;
      return false;
    }
    send_len_ += ret;
    if (send_len_ == send_pkt_.UseLen()) {
      status_ = RecvResponse;
      ModToReadEvent(epoll_fd_, fd_, this);
      last_recv_resp_time_us_ = GetCurrentTimeUs();
    }
    return true;
  }

  bool recvResponse() {
    ssize_t ret = read(fd_, codec_.Data(), codec_.Len());
    last_recv_resp_time_us_ = GetCurrentTimeUs();
    if (ret == 0) {  // 对端关闭连接
      status_ = Finish;
      ClearEvent(epoll_fd_, fd_);
      return true;
    }
    if (ret < 0) {
      if (EINTR == errno || EAGAIN == errno || EWOULDBLOCK == errno) return true;
      read_failure_count_++;
      return false;
    }
    codec_.DeCode(ret);
    std::string* recv_message = codec_.GetMessage();
    if (recv_message != nullptr) {
      if (*recv_message != send_message_) {
        delete recv_message;
        return false;
      }
      status_ = Success;
      ModToWriteEvent(epoll_fd_, fd_, this);
      delete recv_message;
    }
    return true;
  }
  bool dealSuccess() {
    int64_t spend_time_us = last_recv_resp_time_us_ - last_send_req_time_us_;
    send_len_ = 0;
    last_send_req_time_us_ = 0;
    last_recv_resp_time_us_ = 0;
    success_count_++;  // 统计请求成功数
    status_ = SendRequest;
    last_send_req_time_us_ = GetCurrentTimeUs();
    codec_.Reset();
    percentile_->InterfaceSpendTimeStat(spend_time_us);
    double pct50{0}, pct95{0}, pct99{0}, pct999{0};
    if (percentile_->TryPrintSpendTimePctData(pct50, pct95, pct99, pct999)) {
      pct_stat_->InterfaceSpendTimeStat(pct50, pct95, pct99, pct999);
    }
    // 完成的请求数，已经达到最大设置的数
    if (success_count_ >= max_req_count_) {
      status_ = Finish;
      ClearEvent(epoll_fd_, fd_);
    }
    return true;
  }

  int64_t GetCurrentTimeUs() {
    struct timeval current;
    gettimeofday(&current, NULL);
    return current.tv_sec * 1000000 + current.tv_usec;  //计算运行的时间，单位微秒
  }

 private:
  int fd_{-1};  // 客户端关联的fd
  int epoll_fd_{-1};  // 关联的epoll_fd
  ClientStatus status_{Init};  // 客户端状态机
  std::string send_message_;  // 发送的消息
  size_t send_len_{0};  // 完成发送的数据长度
  MyEcho::Codec codec_;  // 用于请求的编解码
  MyEcho::Packet send_pkt_;  // 发送的数据包
  int64_t last_connect_time_us_{0};  // 最近一次connect时间，单位us
  int64_t last_send_req_time_us_{0};  // 最近一次发送请求时间，单位us
  int64_t last_recv_resp_time_us_{0};  // 最近一次接受应答时间，单位us
  Percentile* percentile_{nullptr};
  bool is_debug_;  // 是否调试模式
  int max_req_count_{0};  // 客户端最多执行多少次请求
  int64_t* temp_rate_limit_{nullptr};  // 请求限频临时变量
  int64_t failure_count_{0};  // 失败次数
  int64_t success_count_{0};  // 成功次数
  int64_t read_failure_count_{0};  // 细分的失败统计，读失败数
  int64_t write_failure_count_{0};  // 细分的失败统计，写失败数
  int64_t connect_failure_count_{0};  // 细分的失败统计，连接失败数
  int64_t try_connect_count_{0};  // 尝试连接的次数
  PctStat* pct_stat_{nullptr};  // pct统计
};
}  // namespace BenchMark
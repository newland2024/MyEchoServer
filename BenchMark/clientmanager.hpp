#pragma once

#include <mutex>
#include <string>

#include "client.hpp"
#include "percentile.hpp"
#include "stat.hpp"
#include "timer.hpp"

namespace BenchMark {
class ClientManager {
 public:
  ClientManager(const std::string& ip, int64_t port, int epoll_fd, Timer* timer, int count, const std::string& message,
                int64_t max_req_count, bool is_debug, bool* is_running, int64_t rate_limit, SumStat* sum_stat,
                PctStat* pct_stat)
      : ip_(ip),
        port_(port),
        epoll_fd_(epoll_fd),
        timer_(timer),
        count_(count),
        message_(message),
        max_req_count_(max_req_count),
        is_debug_(is_debug),
        is_running_(is_running),
        rate_limit_(rate_limit),
        sum_stat_(sum_stat),
        pct_stat_(pct_stat) {
    temp_rate_limit_ = rate_limit_;
    clients_ = new EchoClient*[count];
    for (int i = 0; i < count; i++) {
      clients_[i] = newClient();
      clients_[i]->Connect(ip_, port_);
    }
  }
  ~ClientManager() {
    for (int i = 0; i < count_; i++) {
      if (clients_[i]) delete clients_[i];
    }
    delete[] clients_;
  }
  Timer* GetTimer() { return timer_; }
  void SetExit() { *is_running_ = false; }
  void GetStatData() {
    for (int i = 0; i < count_; i++) {
      if (clients_[i]->IsValid()) {
        getDealStat(clients_[i]);
      }
    }
  }

  void CheckClientStatusAndDeal() {  // 检查客户端连接状态，连接失效的会重新建立连接
    int32_t create_client_count = 0;
    for (int i = 0; i < count_; i++) {
      if (clients_[i]->IsValid()) {
        clients_[i]->TryRestart();  // 尝试重启请求
        continue;
      }
      getDealStat(clients_[i]);
      delete clients_[i];
      create_client_count++;
      clients_[i] = newClient();
      clients_[i]->Connect(ip_, port_);
    }
    if (is_debug_) {
      cout << "create_client_count=" << create_client_count << endl;
    }
  }
  void RateLimitRefresh() { temp_rate_limit_ = rate_limit_; }

 private:
  void getDealStat(EchoClient* client) {
    int64_t success_count{0};
    int64_t failure_count{0};
    int64_t connect_failure_count{0};
    int64_t read_failure_count{0};
    int64_t write_failure_count{0};
    int64_t try_connect_count{0};
    client->GetDealStat(success_count, failure_count, connect_failure_count, read_failure_count, write_failure_count,
                        try_connect_count);
    assert(failure_count <= 1 && connect_failure_count <= 1 && read_failure_count <= 1 && write_failure_count <= 1 &&
           try_connect_count <= 1);
    assert(failure_count == (connect_failure_count + read_failure_count + write_failure_count));
    sum_stat_->DoStat(success_count, failure_count, read_failure_count, write_failure_count, connect_failure_count,
                      try_connect_count);
  }
  EchoClient* newClient() {
    EchoClient* client = nullptr;
    client = new EchoClient(epoll_fd_, &percentile_, is_debug_, max_req_count_, &temp_rate_limit_, pct_stat_);
    client->SetEchoMessage(message_);
    return client;
  }

 private:
  std::string ip_;  // 要压测服务监听的ip
  int64_t port_;  // 要压测服务监听的端口
  int epoll_fd_;  // epoll_fd
  Timer* timer_;  // 定时器
  EchoClient** clients_;  // 客户端连接池
  int count_;  // 客户端连接池大小
  std::string message_;  // 要发送的消息
  int64_t max_req_count_;  // 最大请求次数
  bool is_debug_;  // 是否调试模式
  Percentile percentile_;  // 用于统计请求耗时的pctxx数值
  bool* is_running_;  // 是否运行中
  int64_t rate_limit_;  // 请求限流
  int64_t temp_rate_limit_;  // 请求限流临时变量
  SumStat* sum_stat_;  // 汇总统计
  PctStat* pct_stat_;  // pct统计
};
}  // namespace BenchMark
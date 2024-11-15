#pragma once

#include <string>

#include "Coroutine/mycoroutine.h"
#include "client.hpp"
#include "common/percentile.hpp"
#include "common/stat.hpp"

namespace BenchMark2 {
class ClientManager {
 public:
  ClientManager(MyCoroutine::Schedule &schedule, EventDriven::EventLoop &event_loop, int64_t client_count,
                int64_t max_req_count, std::string ip, int port, std::string echo_message, int64_t rate_limit,
                SumStat &sum_stat, PctStat &pct_stat)
      : schedule_(schedule), client_count_(client_count), rate_limit_(rate_limit), temp_rate_limit_(rate_limit),
        sum_stat_(sum_stat), pct_stat_(pct_stat) {
    clients_ = new Client *[client_count];
    for (int64_t i = 0; i < client_count_; i++) {
      clients_[i] = new Client(schedule, event_loop, ip, port, echo_message, max_req_count, temp_rate_limit_, sum_stat_,
                               pct_stat_, percentile_);
    }
  }
  ~ClientManager() {
    for (int64_t i = 0; i < client_count_; i++) {
      delete clients_[i];
    }
    delete[] clients_;
  }

  void InitStart() {
    for (int64_t i = 0; i < client_count_; i++) {
      clients_[i]->InitStart();
    }
  }
  void RateLimitRefreshAndReStart() {
    temp_rate_limit_ = rate_limit_;
    for (int64_t i = 0; i < client_count_; i++) {
      clients_[i]->ReStart();
    }
  }
  void SetFinish() {
    for (int64_t i = 0; i < client_count_; i++) {
      clients_[i]->SetFinish();
    }
  }

 private:
  MyCoroutine::Schedule &schedule_;
  Client **clients_;
  int64_t client_count_;
  int64_t rate_limit_;       // 请求限流值
  int64_t temp_rate_limit_;  // 请求限流临时变量
  SumStat &sum_stat_;        // 汇总统计（全局）
  PctStat &pct_stat_;        // pct统计（全局）
  Percentile percentile_;    // 用于统计请求耗时的pctxx数值（线程各自一份）
};
}  // namespace BenchMark2
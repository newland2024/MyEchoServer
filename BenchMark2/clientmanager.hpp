#pragma once

#include "client.hpp"
#include "Coroutine/mycoroutine.h"
#include <string>

namespace BenchMark2 {
class ClientManager {
public:
  ClientManager(MyCoroutine::Schedule &schedule, EventLoop &event_loop,
                int64_t client_count, std::string ip, int port,
                int64_t rate_limit)
      : schedule_(schedule), client_count_(client_count),
        rate_limit_(rate_limit), temp_rate_limit_(rate_limit) {
    clients_ = new Client *[client_count];
    for (int64_t i = 0; i < client_count_; i++) {
      clients_[i] =
          new Client(schedule, event_loop, ip, port, temp_rate_limit_);
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
  void ReStart() {
    for (int64_t i = 0; i < client_count_; i++) {
      clients_[i]->ReStart();
    }
  }
  void RateLimitRefresh() { temp_rate_limit_ = rate_limit_; }

private:
  MyCoroutine::Schedule schedule_;
  Client **clients_;
  int64_t client_count_;
  int64_t rate_limit_;       // 请求限流值
  int64_t temp_rate_limit_;  // 请求限流临时变量
};
} // namespace BenchMark2
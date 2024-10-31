#pragma once

#include "client.hpp"
#include "Coroutine/mycoroutine.h"
#include <string>

namespace BenchMark2 {
class ClientManager {
public:
  ClientManager(MyCoroutine::Schedule &schedule, int client_count, std::string ip, int port)
      : schedule_(schedule), client_count_(client_count) {
    clients_ = new Client *[client_count];
    for (int i = 0; i < client_count_; i++) {
      clients_[i] = new Client(schedule, ip, port);
    }
  }
  ~ClientManager() {
    for (int i = 0; i < client_count_; i++) {
      delete clients_[i];
    }
    delete[] clients_;
  }

private:
  MyCoroutine::Schedule schedule_;
  Client **clients_;
  int client_count_;
};
} // namespace BenchMark2
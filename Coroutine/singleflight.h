#pragma once

#include "common.h"
#include "mycoroutine.h"

namespace MyCoroutine {
// 协程SingleFlight
class SingleFlight {
 public:
  SingleFlight(Schedule &schedule) : schedule_(schedule) { schedule_.CoSingleFlightInit(co_single_flight_); }
  ~SingleFlight() { schedule_.CoSingleFlightClear(co_single_flight_); }

  template <typename Function, typename... Args>
  void Do(string key, Function &&func, Args &&...args) {
    schedule_.CoSingleFlightDo(co_single_flight_, key, std::forward<Function>(func), std::forward<Args>(args)...);
  }

 private:
  CoSingleFlight co_single_flight_;
  Schedule &schedule_;
};
}  // namespace MyCoroutine

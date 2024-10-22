#include "callonce.h"

namespace MyCoroutine {
void Schedule::CoSingleFlightInit(CoSingleFlight& single_flight) {
  single_flight.state = SingleFlightState::kInit;  // SingleFlight状态
}

void Schedule::CoSingleFlightClear(CoSingleFlight& single_flight) { single_flights_.erase(single_flight.key); }

int Schedule::CoSingleFlightResume() {
  assert(is_master_);
  int count = 0;
  for (auto item : single_flights_) {
    if (item.second->state != SingleFlightState::kFinish) continue;  // 没执行完，不需要唤醒其他从协程
    if (item.second->suspend_cid_set.size() <= 0) {
      // 状态自动扭转成kInit，这样相同key的SingleFlight在执行完之后，就可以再次执行（这个是和CallOnce的最大区别）
      item.second->state = SingleFlightState::kInit;
      continue;
    }
    auto cid_set = item.second->suspend_cid_set;
    // 唤醒所有等待的从协程
    for (const auto cid : cid_set) {
      CoroutineResume(cid);
      count++;
    }
    assert(item.second->suspend_cid_set.size() == 0);
    // 状态自动扭转成kInit，这样相同key的SingleFlight在执行完之后，就可以再次执行（这个是和CallOnce的最大区别）
    item.second->state = SingleFlightState::kInit;
  }
  return count;
}
}  // namespace MyCoroutine

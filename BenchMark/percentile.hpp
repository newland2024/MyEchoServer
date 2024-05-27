#pragma once

#include <algorithm>
#include <iostream>
#include <mutex>
#include <numeric>
#include <vector>

using namespace std;

namespace BenchMark {
class Percentile {
 public:
  void ConnectSpendTimeStat(int64_t value) { connect_spend_time_stat_data_.push_back(value); }
  void InterfaceSpendTimeStat(int64_t value) { interface_spend_time_stat_data_.push_back(value); }
  bool TryPrintSpendTimePctData(double &interface_pct50, double &interface_pct95, double &interface_pct99,
                                double &interface_pct999) {
    if (interface_spend_time_stat_data_.size() < 100000) return false;
    std::sort(connect_spend_time_stat_data_.begin(), connect_spend_time_stat_data_.end());
    std::sort(interface_spend_time_stat_data_.begin(), interface_spend_time_stat_data_.end());
    getInterfacePctValue(0.50, interface_pct50);
    getInterfacePctValue(0.95, interface_pct95);
    getInterfacePctValue(0.99, interface_pct99);
    getInterfacePctValue(0.999, interface_pct999);
    static std::mutex static_mutex;
    {
      double connect_pct50{0}, connect_pct95{0}, connect_pct99{0}, connect_pct999{0};
      std::lock_guard<std::mutex> guard(static_mutex);  // 对终端的输出和统计pctxx数据都是临界区
      cout << "per " << std::to_string(interface_spend_time_stat_data_.size()) << " req stat data -> ";
      cout << "interface[pct50=" << interface_pct50 << "us,pct95=" << interface_pct95 << "us,pct99=" << interface_pct99
           << "us,pct999=" << interface_pct999 << "us]";
      if (getConnectPctValue(0.50, connect_pct50) && getConnectPctValue(0.95, connect_pct95) &&
          getConnectPctValue(0.99, connect_pct99) && getConnectPctValue(0.999, connect_pct999)) {
        cout << ",connect[pct50=" << connect_pct50 << "us,pct95=" << connect_pct95 << "us,pct99=" << connect_pct99
             << "us,pct999=" << connect_pct999 << "us]" << endl;
      } else {
        cout << endl;
      }
    }
    connect_spend_time_stat_data_.clear();
    interface_spend_time_stat_data_.clear();
    return true;
  }

 private:
  bool getInterfacePctValue(double pct, double &pctValue) {
    if (interface_spend_time_stat_data_.size() <= 0) {
      return false;
    }
    double x = (interface_spend_time_stat_data_.size() - 1) * pct;
    int32_t i = (int32_t)x;
    double j = x - i;
    pctValue = (1 - j) * interface_spend_time_stat_data_[i] + j * interface_spend_time_stat_data_[i + 1];
    return true;
  }
  bool getConnectPctValue(double pct, double &pctValue) {
    if (connect_spend_time_stat_data_.size() <= 0) {
      return false;
    }
    double x = (connect_spend_time_stat_data_.size() - 1) * pct;
    int32_t i = (int32_t)x;
    double j = x - i;
    pctValue = (1 - j) * connect_spend_time_stat_data_[i] + j * connect_spend_time_stat_data_[i + 1];
    return true;
  }

 private:
  std::vector<int64_t> connect_spend_time_stat_data_;
  std::vector<int64_t> interface_spend_time_stat_data_;
};

}  // namespace BenchMark
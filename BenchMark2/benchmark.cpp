#include <iostream>
#include <string>
#include <thread>

#include "Coroutine/mycoroutine.h"
#include "EventDriven/eventloop.h"
#include "clientmanager.hpp"
#include "common/cmdline.h"
#include "common/epollctl.hpp"
#include "common/percentile.hpp"
#include "common/stat.hpp"

constexpr char kGreenBegin[] = "\033[32m";
constexpr char kColorEnd[] = "\033[0m";

string ip;
int64_t port;
int64_t thread_count;
int64_t pkt_size;
int64_t client_count;
int64_t run_time;
int64_t max_req_count;
int64_t rate_limit;
bool is_debug;

void Usage() {
  cout << "BenchMark2 -ip 0.0.0.0 -port 1688 -thread_count 1 -max_req_count "
          "100000 -pkt_size 1024 -client_count 200 "
          "-run_time 60 -rate_limit 10000 -debug"
       << endl;
  cout << "options:" << endl;
  cout << "    -h,--help                      print usage" << endl;
  cout << "    -ip,--ip                       service listen ip" << endl;
  cout << "    -port,--port                   service listen port" << endl;
  cout << "    -thread_count,--thread_count   run thread count" << endl;
  cout << "    -max_req_count,--max_req_count one connection max req count" << endl;
  cout << "    -pkt_size,--pkt_size           size of send packet, unit is byte" << endl;
  cout << "    -client_count,--client_count   count of client" << endl;
  cout << "    -run_time,--run_time           run time, unit is second" << endl;
  cout << "    -rate_limit,--rate_limit       rate limit, unit is qps/second" << endl;
  cout << "    -debug,--debug                 debug mode, more info print" << endl;
  cout << endl;
}

void InitStart(BenchMark2::ClientManager& client_manager) { client_manager.InitStart(); }

void RateLimitRefresh(BenchMark2::ClientManager& client_manager, EventDriven::EventLoop& event_loop) {
  client_manager.RateLimitRefreshAndReStart();
  // 重新注册定时器
  event_loop.TimerStart(1000, RateLimitRefresh, std::ref(client_manager), std::ref(event_loop));
}

void StopHandler(EventDriven::EventLoop& event_loop, BenchMark2::ClientManager& client_manager,
                 MyCoroutine::Schedule& schedule) {
  client_manager.SetFinish();  // 停止客户端请求发送，让每个客户端的协程陆续执行完毕。
  if (schedule.IsFinish()) {
    event_loop.SetFinish();  // 所以协程都执行完退出之后，再停止事件的循环
    cout << "SetFinish" << endl;
  } else {
    cout << "after 1 second, try set finish again" << endl;
    // 隔1秒再尝试退出
    event_loop.TimerStart(1000, StopHandler, std::ref(event_loop), std::ref(client_manager),
                          std::ref(schedule));  // 退出事件循环定时器
  }
}

void Handler(SumStat& sum_stat, PctStat& pct_stat) {
  std::string echo_message(pkt_size + 1, 'B');
  EventDriven::EventLoop event_loop;
  MyCoroutine::Schedule schedule(1000);
  BenchMark2::ClientManager client_manager(schedule, event_loop, client_count, ip, port, echo_message, rate_limit,
                                           sum_stat, pct_stat);
  event_loop.TimerStart(1, InitStart, std::ref(client_manager));  // 只调用一次，用于初始化启动客户端
  event_loop.TimerStart(1000, RateLimitRefresh, std::ref(client_manager), std::ref(event_loop));  // 每秒刷新一下限流值
  event_loop.TimerStart(run_time * 1000, StopHandler, std::ref(event_loop), std::ref(client_manager),
                        std::ref(schedule));  // 退出事件循环定时器
  event_loop.Run();
}

void PrintStatData(SumStat& sum_stat, PctStat& pct_stat) {
  cout << kGreenBegin << "--- benchmark statistics ---" << kColorEnd << endl;
  pct_stat.PrintPctAvgData();
  sum_stat.PrintStatData(client_count, run_time);
}

int main(int argc, char* argv[]) {
  CmdLine::StrOptRequired(&ip, "ip");
  CmdLine::Int64OptRequired(&port, "port");
  CmdLine::Int64OptRequired(&thread_count, "thread_count");
  CmdLine::Int64OptRequired(&max_req_count, "max_req_count");
  CmdLine::Int64OptRequired(&pkt_size, "pkt_size");
  CmdLine::Int64OptRequired(&client_count, "client_count");
  CmdLine::Int64OptRequired(&run_time, "run_time");
  CmdLine::Int64OptRequired(&rate_limit, "rate_limit");
  CmdLine::BoolOpt(&is_debug, "debug");
  CmdLine::SetUsage(Usage);
  CmdLine::Parse(argc, argv);
  thread_count = thread_count > 10 ? 10 : thread_count;
  std::thread threads[10];
  constexpr key_t kShmKey = 888888;  // 分配共享内存的key。
  SumStat sum_stat(kShmKey);         // 原子操作，也是线程安全的
  PctStat pct_stat;                  // 线程安全
  for (int64_t i = 0; i < thread_count; i++) {
    threads[i] = std::thread(Handler, std::ref(sum_stat), std::ref(pct_stat));
  }
  for (int64_t i = 0; i < thread_count; i++) {
    threads[i].join();
  }
  PrintStatData(sum_stat, pct_stat);
  return 0;
}
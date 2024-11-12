#include <iostream>
#include <string>
#include <thread>

#include "Coroutine/mycoroutine.h"
#include "EventDriven/eventloop.h"
#include "clientmanager.hpp"
#include "common/cmdline.h"
#include "common/epollctl.hpp"

string ip;
int64_t port;
int64_t thread_count;
int64_t pkt_size;
int64_t client_count;
int64_t run_time;
int64_t max_req_count;
int64_t rate_limit;
bool is_debug;

void usage() {
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
  client_manager.RateLimitRefresh();
  client_manager.ReStart();
  // 重新注册定时器
  event_loop.TimerStart(1000, RateLimitRefresh, std::ref(client_manager), std::ref(event_loop));
}

void StopHandler(EventDriven::EventLoop& event_loop) {
  event_loop.Stop();  // 停止事件循环
}

void Handler() {
  std::string echo_message(pkt_size + 1, 'B');
  EventDriven::EventLoop event_loop;
  MyCoroutine::Schedule schedule(client_count);
  BenchMark2::ClientManager client_manager(schedule, event_loop, client_count, ip, port, echo_message, rate_limit);
  event_loop.TimerStart(1, InitStart, std::ref(client_manager));  // 只调用一次，用于初始化启动客户端
  event_loop.TimerStart(1000, RateLimitRefresh, std::ref(client_manager), std::ref(event_loop));  // 每秒刷新一下限流值
  event_loop.TimerStart(run_time * 1000, StopHandler, std::ref(event_loop));  // 设置定时器，运行时间一到，就退出事件循环
  event_loop.Run();
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
  CmdLine::SetUsage(usage);
  CmdLine::Parse(argc, argv);
  thread_count = thread_count > 10 ? 10 : thread_count;
  std::thread threads[10];
  //   constexpr key_t kShmKey = 12345; // 分配共享内存的key。
  //   SumStat sum_stat(kShmKey);
  //   PctStat pct_stat;
  for (int64_t i = 0; i < thread_count; i++) {
    threads[i] = std::thread(Handler);
  }
  for (int64_t i = 0; i < thread_count; i++) {
    threads[i].join();
  }
  //   printStatData(sum_stat, pct_stat);
  return 0;
}
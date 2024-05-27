#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <mutex>
#include <thread>

#include "../../common/cmdline.h"
#include "../../common/utils.hpp"

using namespace std;
using namespace MyEcho;

std::mutex Mutex;

void handlerClient(int client_fd) {
  string msg;
  while (true) {
    if (not RecvMsg(client_fd, msg)) {
      return;
    }
    if (not SendMsg(client_fd, msg)) {
      return;
    }
  }
}

void handler(int sock_fd) {
  while (true) {
    int client_fd;
    // follower等待获取锁，成为leader
    {
      std::lock_guard<std::mutex> guard(Mutex);
      client_fd = accept(sock_fd, NULL, 0);  // 获取锁，并获取客户端的连接
      if (client_fd < 0) {
        perror("accept failed");
        continue;
      }
    }
    handlerClient(client_fd);  // 处理每个客户端请求
    close(client_fd);
  }
}

void usage() {
  cout << "LeaderAndFollower -ip 0.0.0.0 -port 1688" << endl;
  cout << "options:" << endl;
  cout << "    -h,--help      print usage" << endl;
  cout << "    -ip,--ip       listen ip" << endl;
  cout << "    -port,--port   listen port" << endl;
  cout << endl;
}

int main(int argc, char* argv[]) {
  string ip;
  int64_t port;
  CmdLine::StrOptRequired(&ip, "ip");
  CmdLine::Int64OptRequired(&port, "port");
  CmdLine::SetUsage(usage);
  CmdLine::Parse(argc, argv);
  int sock_fd = CreateListenSocket(ip, port, false);
  if (sock_fd < 0) {
    return -1;
  }
  for (int i = 0; i < GetNProcs(); i++) {
    std::thread(handler, sock_fd).detach();  // 这里需要调用detach，让创建的线程独立运行
  }
  while (true) sleep(1);  // 主进程陷入死循环
  return 0;
}
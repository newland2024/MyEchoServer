#include <sys/socket.h>
#include <unistd.h>

#include <iostream>

#include "../../common/cmdline.h"
#include "../../common/utils.hpp"

using namespace std;
using namespace MyEcho;

void handlerClient(int client_fd, int64_t& count) {
  string msg;
  while (true) {
    if (not RecvMsg(client_fd, msg)) {
      return;
    }
    if (not SendMsg(client_fd, msg)) {
      return;
    }
    count++;
  }
}

void handler(int worker_id, string ip, int64_t port) {
  // 开启SO_REUSEPORT选项
  int sock_fd = CreateListenSocket(ip, port, true);
  if (sock_fd < 0) {
    return;
  }
  int64_t count = 0;
  while (true) {
    int client_fd = accept(sock_fd, NULL, 0);
    if (client_fd < 0) {
      perror("accept failed");
      continue;
    }
    handlerClient(client_fd, count);
    close(client_fd);
    if (count >= 10000) {
      cout << "worker_id[" << worker_id << "] deal_1w_request" << endl;
      count = 0;
    }
  }
}

void usage() {
  cout << "ProcessPool2 -ip 0.0.0.0 -port 1688" << endl;
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
  for (int i = 0; i < GetNProcs(); i++) {
    pid_t pid = fork();
    if (pid < 0) {
      perror("fork failed");
      continue;
    }
    if (0 == pid) {
      handler(i, ip, port);  // 子进程陷入死循环，处理客户端请求
      exit(0);
    }
  }
  while (true) sleep(1);  // 父进程陷入死循环
  return 0;
}
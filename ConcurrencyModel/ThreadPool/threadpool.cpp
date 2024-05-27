#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <thread>

#include "../../common/cmdline.h"
#include "../../common/utils.hpp"

using namespace std;
using namespace MyEcho;

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
void handler(string ip, int64_t port) {
  // 开启SO_REUSEPORT选项
  int sock_fd = CreateListenSocket(ip, port, true);
  if (sock_fd < 0) {
    return;
  }
  while (true) {
    int client_fd = accept(sock_fd, NULL, 0);
    if (client_fd < 0) {
      perror("accept failed");
      continue;
    }
    handlerClient(client_fd);
    close(client_fd);
  }
}

void usage() {
  cout << "ThreadPool -ip 0.0.0.0 -port 1688" << endl;
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
    std::thread(handler, ip, port).detach();  // 这里需要调用detach，让创建的线程独立运行
  }
  while (true) sleep(1);  // 主线程陷入死循环
  return 0;
}
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
      break;
    }
    if (not SendMsg(client_fd, msg)) {
      break;
    }
  }
  close(client_fd);
}

void usage() {
  cout << "MultiThread -ip 0.0.0.0 -port 1688" << endl;
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
  while (true) {
    int client_fd = accept(sock_fd, NULL, 0);
    if (client_fd < 0) {
      perror("accept failed");
      continue;
    }
    std::thread(handlerClient, client_fd).detach();  // 这里需要调用detach，让创建的线程独立运行
  }
  return 0;
}
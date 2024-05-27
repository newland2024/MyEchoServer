#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>

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

void childExitSignalHandler() {
  struct sigaction act;
  act.sa_handler = SIG_IGN;  //设置信号处理函数，这里忽略子进程的退出信号
  sigemptyset(&act.sa_mask);  //信号屏蔽设置为空
  act.sa_flags = 0;  //标志位设置为0
  sigaction(SIGCHLD, &act, NULL);
}

void usage() {
  cout << "MultiProcess -ip 0.0.0.0 -port 1688" << endl;
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
  childExitSignalHandler();  // 这里需要忽略子进程退出信号，否则会导致大量的僵尸进程，服务后续无法再创建子进程
  while (true) {
    int client_fd = accept(sock_fd, NULL, 0);
    if (client_fd < 0) {
      perror("accept failed");
      continue;
    }
    pid_t pid = fork();
    if (pid == -1) {
      close(client_fd);
      perror("fork failed");
      continue;
    }
    if (pid == 0) {  // 子进程
      handlerClient(client_fd);
      close(client_fd);
      exit(0);  // 处理完请求，子进程直接退出
    } else {
      close(client_fd);  // 父进程直接关闭客户端连接，否则文件描述符会泄露
    }
  }
  return 0;
}

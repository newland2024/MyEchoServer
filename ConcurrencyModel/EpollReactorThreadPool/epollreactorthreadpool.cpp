#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <thread>

#include "../../common/cmdline.h"
#include "../../common/conn.hpp"
#include "../../common/epollctl.hpp"

using namespace std;
using namespace MyEcho;

void handler(string ip, int64_t port) {
  int sock_fd = CreateListenSocket(ip, port, true);
  if (sock_fd < 0) {
    return;
  }
  epoll_event events[2048];
  int epoll_fd = epoll_create(1);
  if (epoll_fd < 0) {
    perror("epoll_create failed");
    return;
  }
  Conn conn(sock_fd, epoll_fd, true);
  SetNotBlock(sock_fd);
  AddReadEvent(&conn);
  while (true) {
    int num = epoll_wait(epoll_fd, events, 2048, -1);
    if (num < 0) {
      perror("epoll_wait failed");
      continue;
    }
    for (int i = 0; i < num; i++) {
      Conn *conn = (Conn *)events[i].data.ptr;
      if (conn->Fd() == sock_fd) {
        LoopAccept(sock_fd, 2048, [epoll_fd](int client_fd) {
          Conn *conn = new Conn(client_fd, epoll_fd, true);
          SetNotBlock(client_fd);
          AddReadEvent(conn);  // 监听可读事件
        });
        continue;
      }
      auto releaseConn = [&conn]() {
        ClearEvent(conn);
        delete conn;
      };
      if (events[i].events & EPOLLIN) {  // 可读
        if (not conn->Read()) {  // 执行读失败
          releaseConn();
          continue;
        }
        if (conn->OneMessage()) {  // 判断是否要触发写事件
          conn->EnCode();
          ModToWriteEvent(conn);  // 修改成只监控可写事件
        }
      }
      if (events[i].events & EPOLLOUT) {  // 可写
        if (not conn->Write()) {  // 执行写失败
          releaseConn();
          continue;
        }
        if (conn->FinishWrite()) {  // 完成了请求的应答写，则可以释放连接
          conn->Reset();
          ModToReadEvent(conn);  // 修改成只监控可读事件
        }
      }
    }
  }
}

void usage() {
  cout << "EpollReactorThreadPool -ip 0.0.0.0 -port 1688 -poolsize 8" << endl;
  cout << "options:" << endl;
  cout << "    -h,--help      print usage" << endl;
  cout << "    -ip,--ip       listen ip" << endl;
  cout << "    -port,--port   listen port" << endl;
  cout << "    -poolsize,--poolsize   pool size" << endl;
  cout << endl;
}

int main(int argc, char *argv[]) {
  string ip;
  int64_t port;
  int64_t pool_size;
  CmdLine::StrOptRequired(&ip, "ip");
  CmdLine::Int64OptRequired(&port, "port");
  CmdLine::Int64OptRequired(&pool_size, "poolsize");
  CmdLine::SetUsage(usage);
  CmdLine::Parse(argc, argv);
  pool_size = pool_size > GetNProcs() ? GetNProcs() : pool_size;
  for (int i = 0; i < pool_size; i++) {
    std::thread(handler, ip, port).detach();  // 这里需要调用detach，让创建的线程独立运行
  }
  while (true) sleep(1);  // 主线程陷入死循环
  return 0;
}
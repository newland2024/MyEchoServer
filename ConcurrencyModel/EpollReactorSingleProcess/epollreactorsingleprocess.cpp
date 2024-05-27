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

#include "../../common/cmdline.h"
#include "../../common/epollctl.hpp"

using namespace std;
using namespace MyEcho;

void usage() {
  cout << "EpollReactorSingleProcess -ip 0.0.0.0 -port 1688 -multiio -la -writefirst" << endl;
  cout << "options:" << endl;
  cout << "    -h,--help      print usage" << endl;
  cout << "    -ip,--ip       listen ip" << endl;
  cout << "    -port,--port   listen port" << endl;
  cout << "    -multiio,--multiio  multi io" << endl;
  cout << "    -la,--la       loop accept" << endl;
  cout << "    -writefirst--writefirst write first" << endl;
  cout << endl;
}

int main(int argc, char *argv[]) {
  string ip;
  int64_t port;
  bool is_multi_io;
  bool is_loop_accept;
  bool is_write_first;
  CmdLine::StrOptRequired(&ip, "ip");
  CmdLine::Int64OptRequired(&port, "port");
  CmdLine::BoolOpt(&is_multi_io, "multiio");
  CmdLine::BoolOpt(&is_loop_accept, "la");
  CmdLine::BoolOpt(&is_write_first, "writefirst");
  CmdLine::SetUsage(usage);
  CmdLine::Parse(argc, argv);
  cout << "is_loop_accept = " << is_loop_accept << endl;
  cout << "is_multi_io = " << is_multi_io << endl;
  cout << "is_write_first = " << is_write_first << endl;
  int sock_fd = CreateListenSocket(ip, port, false);
  if (sock_fd < 0) {
    return -1;
  }
  epoll_event events[2048];
  int epoll_fd = epoll_create(1);
  if (epoll_fd < 0) {
    perror("epoll_create failed");
    return -1;
  }
  Conn conn(sock_fd, epoll_fd, is_multi_io);
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
        int max_conn = is_loop_accept ? 2048 : 1;
        LoopAccept(sock_fd, max_conn, [epoll_fd, is_multi_io](int client_fd) {
          Conn *conn = new Conn(client_fd, epoll_fd, is_multi_io);
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
          if (is_write_first) {  // 判断是否要先写数据
            if (not conn->Write()) {
              releaseConn();
              continue;
            }
          }
          if (conn->FinishWrite()) {
            conn->Reset();
            ModToReadEvent(conn);  // 修改成只监控可读事件
          } else {
            ModToWriteEvent(conn);  // 修改成只监控可写事件
          }
        }
      }
      if (events[i].events & EPOLLOUT) {  // 可写
        if (not conn->Write()) {  // 执行写失败
          releaseConn();
          continue;
        }
        if (conn->FinishWrite()) {  // 完成了请求的应答写
          conn->Reset();
          ModToReadEvent(conn);  // 修改成只监控可读事件
        }
      }
    }
  }
  return 0;
}
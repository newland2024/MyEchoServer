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

int *EpollFd;

int createEpoll() {
  int epoll_fd = epoll_create(1);
  assert(epoll_fd > 0);
  return epoll_fd;
}

void addToSubReactor(int &index, int sub_reactor_count, int client_fd) {
  index++;
  index %= sub_reactor_count;
  // 轮询的方式添加到subReactor线程中
  Conn *conn = new Conn(client_fd, EpollFd[index], true);
  AddReadEvent(conn);  // 监听可读事件
}

void mainReactor(string ip, int64_t port, int64_t sub_reactor_count, bool is_main_read) {
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
  int index = 0;
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
      if (conn->Fd() == sock_fd) {  // 有客户端的连接到来了
        LoopAccept(sock_fd, 2048, [&index, is_main_read, epoll_fd, sub_reactor_count](int client_fd) {
          SetNotBlock(client_fd);
          if (is_main_read) {
            Conn *conn = new Conn(client_fd, epoll_fd, true);
            AddReadEvent(conn);  // 在mainReactor线程中监听可读事件
          } else {
            addToSubReactor(index, sub_reactor_count, client_fd);
          }
        });
        continue;
      }
      // 客户端有数据可读，则把连接迁移到subReactor线程中管理
      ClearEvent(conn, false);
      addToSubReactor(index, sub_reactor_count, conn->Fd());
      delete conn;
    }
  }
}

void subReactor(int thread_id) {
  epoll_event events[2048];
  int epoll_fd = EpollFd[thread_id];
  while (true) {
    int num = epoll_wait(epoll_fd, events, 2048, -1);
    if (num < 0) {
      perror("epoll_wait failed");
      continue;
    }
    for (int i = 0; i < num; i++) {
      Conn *conn = (Conn *)events[i].data.ptr;
      auto releaseConn = [&conn]() {
        ClearEvent(conn);
        delete conn;
      };
      if (events[i].events & EPOLLIN) {  // 可读
        if (not conn->Read()) {  // 执行非阻塞读
          releaseConn();
          continue;
        }
        if (conn->OneMessage()) {  // 判断是否要触发写事件
          conn->EnCode();
          ModToWriteEvent(conn);  // 修改成只监控可写事件
        }
      }
      if (events[i].events & EPOLLOUT) {  // 可写
        if (not conn->Write()) {  // 执行非阻塞写
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
  cout << "EpollReactorThreadPoolMS -ip 0.0.0.0 -port 1688 -main 3 -sub 8 -mainread" << endl;
  cout << "options:" << endl;
  cout << "    -h,--help      print usage" << endl;
  cout << "    -ip,--ip       listen ip" << endl;
  cout << "    -port,--port   listen port" << endl;
  cout << "    -main,--main   mainReactor count" << endl;
  cout << "    -sub,--sub     subReactor count" << endl;
  cout << "    -mainread,--mainread mainReactor read" << endl;
  cout << endl;
}

int main(int argc, char *argv[]) {
  string ip;
  int64_t port;
  int64_t main_reactor_count;
  int64_t sub_reactor_count;
  bool is_main_read;
  CmdLine::StrOptRequired(&ip, "ip");
  CmdLine::Int64OptRequired(&port, "port");
  CmdLine::Int64OptRequired(&main_reactor_count, "main");
  CmdLine::Int64OptRequired(&sub_reactor_count, "sub");
  CmdLine::BoolOpt(&is_main_read, "mainread");
  CmdLine::SetUsage(usage);
  CmdLine::Parse(argc, argv);
  cout << "is_main_read=" << is_main_read << endl;
  main_reactor_count = main_reactor_count > GetNProcs() ? GetNProcs() : main_reactor_count;
  sub_reactor_count = sub_reactor_count > GetNProcs() ? GetNProcs() : sub_reactor_count;
  EpollFd = new int[sub_reactor_count];
  for (int i = 0; i < sub_reactor_count; i++) {
    EpollFd[i] = createEpoll();
    std::thread(subReactor, i).detach();  // 这里需要调用detach，让创建的线程独立运行
  }
  for (int i = 0; i < main_reactor_count; i++) {
    std::thread(mainReactor, ip, port, sub_reactor_count, is_main_read)
        .detach();  // 这里需要调用detach，让创建的线程独立运行
  }
  while (true) sleep(1);  // 主线程陷入死循环
  return 0;
}
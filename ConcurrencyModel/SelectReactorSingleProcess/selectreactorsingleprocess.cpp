#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include "../../common/cmdline.h"
#include "../../common/conn.hpp"

using namespace std;
using namespace MyEcho;

void updateSet(unordered_set<int> &read_fds, unordered_set<int> &write_fds, int &max_fd, int sock_fd, fd_set &read_set,
               fd_set &write_set) {
  max_fd = sock_fd;
  FD_ZERO(&read_set);
  FD_ZERO(&write_set);
  for (const auto &read_fd : read_fds) {
    if (read_fd > max_fd) {
      max_fd = read_fd;
    }
    FD_SET(read_fd, &read_set);
  }
  for (const auto &write_fd : write_fds) {
    if (write_fd > max_fd) {
      max_fd = write_fd;
    }
    FD_SET(write_fd, &write_set);
  }
}

void usage() {
  cout << "SelectReactorSingleProcess -ip 0.0.0.0 -port 1688" << endl;
  cout << "options:" << endl;
  cout << "    -h,--help      print usage" << endl;
  cout << "    -ip,--ip       listen ip" << endl;
  cout << "    -port,--port   listen port" << endl;
  cout << endl;
}

int main(int argc, char *argv[]) {
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
  int max_fd;
  fd_set read_set;
  fd_set write_set;
  SetNotBlock(sock_fd);
  unordered_set<int> read_fds;
  unordered_set<int> write_fds;
  unordered_map<int, Conn *> conns;
  while (true) {
    read_fds.insert(sock_fd);
    updateSet(read_fds, write_fds, max_fd, sock_fd, read_set, write_set);
    int ret = select(max_fd + 1, &read_set, &write_set, nullptr, nullptr);
    if (ret <= 0) {
      if (ret < 0) perror("select failed");
      continue;
    }
    unordered_set<int> temp = read_fds;
    for (const auto &fd : temp) {
      if (not FD_ISSET(fd, &read_set)) {
        continue;
      }
      if (fd == sock_fd) {  // 监听的sock_fd可读，则表示有新的链接
        LoopAccept(sock_fd, 1024, [&read_fds, &conns](int client_fd) {
          if (client_fd >= FD_SETSIZE) {  // 大于FD_SETSIZE的值，则不支持
            close(client_fd);
            return;
          }
          read_fds.insert(client_fd);  // 新增到要监听的fd集合中
          conns[client_fd] = new Conn(client_fd, true);
        });
        continue;
      }
      // 执行到这里，表明可读
      Conn *conn = conns[fd];
      if (not conn->Read()) {  // 执行读失败
        delete conn;
        conns.erase(fd);
        read_fds.erase(fd);
        close(fd);
        continue;
      }
      if (conn->OneMessage()) {  // 判断是否要触发写事件
        conn->EnCode();
        read_fds.erase(fd);
        write_fds.insert(fd);
      }
    }
    temp = write_fds;
    for (const auto &fd : temp) {
      if (not FD_ISSET(fd, &write_set)) {
        continue;
      }
      // 执行到这里，表明可写
      Conn *conn = conns[fd];
      if (not conn->Write()) {  // 执行写失败
        delete conn;
        conns.erase(fd);
        write_fds.erase(fd);
        close(fd);
        continue;
      }
      if (conn->FinishWrite()) {  // 完成了请求的应答写
        conn->Reset();
        write_fds.erase(fd);
        read_fds.insert(fd);
      }
    }
  }
  return 0;
}
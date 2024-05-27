#include <stdio.h>
#include <unistd.h>

#include <iostream>
#include <unordered_set>

#include "../../common/cmdline.h"
#include "../../common/utils.hpp"

using namespace std;
using namespace MyEcho;

void updateReadSet(unordered_set<int> &read_fds, int &max_fd, int sock_fd, fd_set &read_set) {
  max_fd = sock_fd;
  FD_ZERO(&read_set);
  for (const auto &read_fd : read_fds) {
    if (read_fd > max_fd) {
      max_fd = read_fd;
    }
    FD_SET(read_fd, &read_set);
  }
}

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

void usage() {
  cout << "Select -ip 0.0.0.0 -port 1688" << endl;
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
  SetNotBlock(sock_fd);
  unordered_set<int> read_fds;
  while (true) {
    read_fds.insert(sock_fd);
    updateReadSet(read_fds, max_fd, sock_fd, read_set);
    int ret = select(max_fd + 1, &read_set, NULL, NULL, NULL);
    if (ret <= 0) {
      if (ret < 0) perror("select failed");
      continue;
    }
    for (int i = 0; i <= max_fd; i++) {
      if (not FD_ISSET(i, &read_set)) {
        continue;
      }
      if (i == sock_fd) {  // 监听的sock_fd可读，则表示有新的链接
        LoopAccept(sock_fd, 1024, [&read_fds](int client_fd) {
          if (client_fd >= FD_SETSIZE) {  // 大于FD_SETSIZE的值，则不支持
            close(client_fd);
            return;
          }
          read_fds.insert(client_fd);  // 新增到要监听的fd集合中
        });
        continue;
      }
      handlerClient(i);
      read_fds.erase(i);
      close(i);
    }
  }
  return 0;
}

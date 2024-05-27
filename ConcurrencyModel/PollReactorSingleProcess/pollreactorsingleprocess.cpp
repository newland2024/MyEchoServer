#include <poll.h>
#include <stdio.h>
#include <unistd.h>

#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include "../../common/cmdline.h"
#include "../../common/conn.hpp"
#include "../../common/utils.hpp"

using namespace std;
using namespace MyEcho;

void updateFds(unordered_set<int> &read_fds, unordered_set<int> &write_fds, pollfd **fds, int &nfds) {
  if (*fds != nullptr) {
    delete[](*fds);
  }
  nfds = read_fds.size() + write_fds.size();
  *fds = new pollfd[nfds];
  int index = 0;
  for (const auto &read_fd : read_fds) {
    (*fds)[index].fd = read_fd;
    (*fds)[index].events = POLLIN;
    (*fds)[index].revents = 0;
    index++;
  }
  for (const auto &write_fd : write_fds) {
    (*fds)[index].fd = write_fd;
    (*fds)[index].events = POLLOUT;
    (*fds)[index].revents = 0;
    index++;
  }
}

void usage() {
  cout << "PollReactorSingleProcess -ip 0.0.0.0 -port 1688" << endl;
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
  int nfds = 0;
  pollfd *fds = nullptr;
  unordered_set<int> read_fds;
  unordered_set<int> write_fds;
  unordered_map<int, Conn *> conns;
  SetNotBlock(sock_fd);
  while (true) {
    read_fds.insert(sock_fd);
    updateFds(read_fds, write_fds, &fds, nfds);
    int ret = poll(fds, nfds, -1);
    if (ret <= 0) {
      if (ret < 0) perror("poll failed");
      continue;
    }
    for (int i = 0; i < nfds; i++) {
      if (fds[i].revents & POLLIN) {
        int fd = fds[i].fd;
        if (fd == sock_fd) {
          LoopAccept(sock_fd, 2048, [&read_fds, &conns](int client_fd) {
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
      if (fds[i].revents & POLLOUT) {  // 可写
        int fd = fds[i].fd;
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
  }
  return 0;
}

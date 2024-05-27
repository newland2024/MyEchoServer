#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <unistd.h>

#include <iostream>
#include <unordered_set>

#include "../../common/cmdline.h"
#include "../../common/utils.hpp"

using namespace std;
using namespace MyEcho;

void updateFds(std::unordered_set<int> &client_fds, pollfd **fds, int &nfds) {
  if (*fds != nullptr) {
    delete[](*fds);
  }
  nfds = client_fds.size();
  *fds = new pollfd[nfds];
  int index = 0;
  for (const auto &client_fd : client_fds) {
    (*fds)[index].fd = client_fd;
    (*fds)[index].events = POLLIN;
    (*fds)[index].revents = 0;
    index++;
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
  cout << "Poll -ip 0.0.0.0 -port 1688" << endl;
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
  std::unordered_set<int> client_fds;
  client_fds.insert(sock_fd);
  SetNotBlock(sock_fd);
  while (true) {
    updateFds(client_fds, &fds, nfds);
    int ret = poll(fds, nfds, -1);
    if (ret <= 0) {
      if (ret < 0) perror("poll failed");
      continue;
    }
    for (int i = 0; i < nfds; i++) {
      if (not(fds[i].revents & POLLIN)) {
        continue;
      }
      int current_fd = fds[i].fd;
      if (current_fd == sock_fd) {
        LoopAccept(sock_fd, 1024, [&client_fds](int client_fd) {
          client_fds.insert(client_fd);  // 新增到要监听的fd集合中
        });
        continue;
      }
      handlerClient(current_fd);
      client_fds.erase(current_fd);
      close(current_fd);
    }
  }
  return 0;
}

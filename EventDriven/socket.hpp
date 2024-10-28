#pragma once

#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <string>

namespace EventDriven {
class Socket {
 public:
  static int CreateListenSocket(std::string ip, uint16_t port) {
    sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
      perror("inet_pton failed");
      assert(0);
    }
    int socket_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);  // 直接创建非阻塞的 socket fd
    assert(socket_fd >= 0);
    int reuse = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) != 0) {
      perror("setsockopt failed");
      assert(0);
    }
    if (bind(socket_fd, (sockaddr *)&addr, sizeof(addr)) != 0) {
      perror("bind failed");
      assert(0);
    }
    if (listen(socket_fd, 2048) != 0) {
      perror("listen failed");
      assert(0);
    }
    return socket_fd;
  }
};
}  // namespace EventDriven
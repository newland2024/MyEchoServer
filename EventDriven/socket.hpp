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
  static int Connect(std::string ip, uint16_t port, int &fd) {
    fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (fd < 0) {
      return -1;
    }
    sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
      perror("inet_pton failed");
      assert(0);
    }
    int ret = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    if (0 == ret) {
      return 0;
    }
    return errno;
  }

  static void SetCloseWithRst(int fd) {
    struct linger lin;
    lin.l_onoff = 1;
    lin.l_linger = 0;
    // 设置调用close关闭tcp连接时，直接发送RST包，tcp连接直接复位，进入到closed状态。
    assert(0 == setsockopt(fd, SOL_SOCKET, SO_LINGER, &lin, sizeof(lin)));
  }
  
  static bool IsConnectSuccess(int fd) {
    int err = 0;
    socklen_t err_len = sizeof(err);
    assert(0 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &err_len));
    return 0 == err;
  }
};
}  // namespace EventDriven
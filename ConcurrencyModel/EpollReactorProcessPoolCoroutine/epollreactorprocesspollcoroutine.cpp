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
#include "../../common/coroutine.h"
#include "../../common/epollctl.hpp"

using namespace std;
using namespace MyEcho;

struct EventData {
  EventData(int fd, int epoll_fd) : fd_(fd), epoll_fd_(epoll_fd){};
  int fd_{0};
  int epoll_fd_{0};
  int cid_{MyCoroutine::kInvalidRoutineId};
  MyCoroutine::Schedule *schedule_{nullptr};
};

void EchoDeal(const std::string req_message, std::string &resp_message) { resp_message = req_message; }

void handlerClient(EventData *event_data) {
  auto releaseConn = [&event_data]() {
    ClearEvent(event_data->epoll_fd_, event_data->fd_);
    delete event_data;  // 释放内存
  };
  while (true) {
    ssize_t ret = 0;
    Codec codec;
    string *req_message{nullptr};
    string resp_message;
    while (true) {  // 读操作
      ret = read(event_data->fd_, codec.Data(), codec.Len());  // 一次最多读取100字节
      if (ret == 0) {
        perror("peer close connection");
        releaseConn();
        return;
      }
      if (ret < 0) {
        if (EINTR == errno) continue;  // 被中断，可以重启读操作
        if (EAGAIN == errno or EWOULDBLOCK == errno) {
          MyCoroutine::CoroutineYield(*event_data->schedule_);  // 让出cpu，切换到主协程，等待下一次数据可读
          continue;
        }
        perror("read failed");
        releaseConn();
        return;
      }
      codec.DeCode(ret);  // 解析请求数据
      req_message = codec.GetMessage();
      if (req_message) {  // 解析出一个完整的请求
        break;
      }
    }
    // 执行到这里说明已经读取到一个完整的请求
    EchoDeal(*req_message, resp_message);  // 业务handler的封装，这样协程的调用就对业务逻辑函数EchoDeal透明
    delete req_message;
    Packet pkt;
    codec.EnCode(resp_message, pkt);
    ModToWriteEvent(event_data->epoll_fd_, event_data->fd_, event_data);  // 监听可写事件。
    size_t sendLen = 0;
    while (sendLen != pkt.UseLen()) {  // 写操作
      ret = write(event_data->fd_, pkt.Data() + sendLen, pkt.UseLen() - sendLen);
      if (ret < 0) {
        if (EINTR == errno) continue;  // 被中断，可以重启写操作
        if (EAGAIN == errno or EWOULDBLOCK == errno) {
          MyCoroutine::CoroutineYield(*event_data->schedule_);  // 让出cpu，切换到主协程，等待下一次数据可写
          continue;
        }
        perror("write failed");
        releaseConn();
        return;
      }
      sendLen += ret;
    }
    ModToReadEvent(event_data->epoll_fd_, event_data->fd_, event_data);  // 监听可读事件。
  }
}

int handler(string ip, int64_t port) {
  int sock_fd = CreateListenSocket(ip, port, true);
  if (sock_fd < 0) {
    return -1;
  }
  epoll_event events[2048];
  int epoll_fd = epoll_create(1);
  if (epoll_fd < 0) {
    perror("epoll_create failed");
    return -1;
  }
  EventData event_data(sock_fd, epoll_fd);
  SetNotBlock(sock_fd);
  AddReadEvent(epoll_fd, sock_fd, &event_data);
  MyCoroutine::Schedule schedule;
  MyCoroutine::ScheduleInit(schedule, 5000);  // 协程池初始化
  int msec = -1;
  while (true) {
    int num = epoll_wait(epoll_fd, events, 2048, msec);
    if (num < 0) {
      perror("epoll_wait failed");
      continue;
    } else if (num == 0) {  // 没有事件了，下次调用epoll_wait大概率被挂起
      sleep(0);  // 这里直接sleep(0)让出cpu，大概率被挂起，这里主动让出cpu，可以减少一次epoll_wait的调用
      msec = -1;  // 大概率被挂起，故这里超时时间设置为-1
      continue;
    }
    msec = 0;  // 下次大概率还有事件，故msec设置为0
    for (int i = 0; i < num; i++) {
      EventData *event_data = (EventData *)events[i].data.ptr;
      if (event_data->fd_ == sock_fd) {
        LoopAccept(sock_fd, 2048, [epoll_fd](int client_fd) {
          EventData *event_data = new EventData(client_fd, epoll_fd);
          SetNotBlock(client_fd);
          AddReadEvent(epoll_fd, client_fd, event_data);  // 监听可读事件
        });
        continue;
      }
      if (event_data->cid_ == MyCoroutine::kInvalidRoutineId) {  // 第一次事件，则创建协程
        event_data->schedule_ = &schedule;
        event_data->cid_ = MyCoroutine::CoroutineCreate(schedule, handlerClient, event_data);  // 创建协程
        MyCoroutine::CoroutineResumeById(schedule, event_data->cid_);
      } else {
        MyCoroutine::CoroutineResumeById(schedule, event_data->cid_);  // 唤醒之前主动让出cpu的协程
      }
    }
  }
  return 0;
}

void usage() {
  cout << "EpollReactorProcessPoolCoroutine -ip 0.0.0.0 -port 1688 -poolsize 8" << endl;
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
    pid_t pid = fork();
    assert(pid != -1);
    if (0 == pid) {
      handler(ip, port);  // 子进程陷入死循环，处理客户端请求
      exit(0);
    }
  }
  while (true) sleep(1);  // 父进程陷入死循环
  return 0;
}
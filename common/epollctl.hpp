#pragma once

#include <sys/epoll.h>
#include <unistd.h>

#include "conn.hpp"

inline void AddReadEvent(MyEcho::Conn *conn, bool is_et = false, bool is_one_shot = false) {
  epoll_event event;
  event.data.ptr = (void *)conn;
  event.events = EPOLLIN;
  if (is_et) event.events |= EPOLLET;
  if (is_one_shot) event.events |= EPOLLONESHOT;
  assert(epoll_ctl(conn->EpollFd(), EPOLL_CTL_ADD, conn->Fd(), &event) != -1);
}

inline void AddReadEvent(int epoll_fd, int fd, void *user_data) {
  epoll_event event;
  event.data.ptr = user_data;
  event.events = EPOLLIN;
  assert(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) != -1);
}

inline void AddWriteEvent(int epoll_fd, int fd, void *user_data) {
  epoll_event event;
  event.data.ptr = user_data;
  event.events = EPOLLOUT;
  assert(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) != -1);
}

inline void ReStartReadEvent(MyEcho::Conn *conn) {
  epoll_event event;
  event.data.ptr = (void *)conn;
  event.events = EPOLLIN | EPOLLONESHOT;
  assert(epoll_ctl(conn->EpollFd(), EPOLL_CTL_MOD, conn->Fd(), &event) != -1);
}

inline void ModToWriteEvent(MyEcho::Conn *conn, bool is_et = false) {
  epoll_event event;
  event.data.ptr = (void *)conn;
  event.events = EPOLLOUT;
  if (is_et) event.events |= EPOLLET;
  assert(epoll_ctl(conn->EpollFd(), EPOLL_CTL_MOD, conn->Fd(), &event) != -1);
}

inline void ModToWriteEvent(int epoll_fd, int fd, void *user_data) {
  epoll_event event;
  event.data.ptr = user_data;
  event.events = EPOLLOUT;
  assert(epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event) != -1);
}

inline void ModToReadEvent(MyEcho::Conn *conn, bool is_et = false) {
  epoll_event event;
  event.data.ptr = (void *)conn;
  event.events = EPOLLIN;
  if (is_et) event.events |= EPOLLET;
  assert(epoll_ctl(conn->EpollFd(), EPOLL_CTL_MOD, conn->Fd(), &event) != -1);
}

inline void ModToReadEvent(int epoll_fd, int fd, void *user_data) {
  epoll_event event;
  event.data.ptr = user_data;
  event.events = EPOLLIN;
  assert(epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event) != -1);
}

inline void ClearEvent(MyEcho::Conn *conn, bool is_close = true) {
  assert(epoll_ctl(conn->EpollFd(), EPOLL_CTL_DEL, conn->Fd(), NULL) != -1);
  if (is_close) close(conn->Fd());  // close操作需要EPOLL_CTL_DEL之后调用，否则调用epoll_ctl()删除fd会失败
}

void ClearEvent(int epoll_fd, int fd, bool is_close = true) {
  assert(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) != -1);
  if (is_close) close(fd);  // close操作需要EPOLL_CTL_DEL之后调用，否则调用epoll_ctl()删除fd会失败
}

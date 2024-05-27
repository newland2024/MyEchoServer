# 1.概述
本仓库通过c++语言，使用了20种不同的并发模型实现了回显服务，并设计实现了简单的应用层协议。

# 2.目录结构
本仓库的目录结构如下所示。
```
.
├── BenchMark
│   ├── benchmark.cpp
│   ├── client.hpp
│   ├── clientmanager.hpp
│   ├── makefile
│   ├── percentile.hpp
│   ├── stat.hpp
│   └── timer.hpp
├── ConcurrencyModel
│   ├── Epoll
│   ├── EpollReactorProcessPoolCoroutine
│   ├── EpollReactorProcessPoolMS
│   ├── EpollReactorSingleProcess
│   ├── EpollReactorSingleProcessCoroutine
│   ├── EpollReactorSingleProcessET
│   ├── EpollReactorThreadPool
│   ├── EpollReactorThreadPoolHSHA
│   ├── EpollReactorThreadPoolMS
│   ├── LeaderAndFollower
│   ├── MultiProcess
│   ├── MultiThread
│   ├── Poll
│   ├── PollReactorSingleProcess
│   ├── ProcessPool1
│   ├── ProcessPool2
│   ├── Select
│   ├── SelectReactorSingleProcess
│   ├── SingleProcess
│   └── ThreadPool
├── common
│   ├── cmdline.cpp
│   ├── cmdline.h
│   ├── codec.hpp
│   ├── conn.hpp
│   ├── coroutine.cpp
│   ├── coroutine.h
│   ├── epollctl.hpp
│   ├── packet.hpp
│   └── utils.hpp
├── readme.md
└── test
    ├── codectest.cpp
    ├── coroutinetest.cpp
    ├── makefile
    ├── packettest.cpp
    ├── unittestcore.hpp
    └── unittestentry.cpp
```
相关的文件和目录的说明如下。
- BenchMark是基准性能压测工具的代码目录。
- ConcurrencyModel是20种不同并发模型的代码目录。
- common是公共代码的目录。
- test目录为单元测试代码的目录。

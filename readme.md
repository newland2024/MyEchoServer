# 1.概述
本仓库通过C++11语言，使用了23种不同的并发模型实现了回显服务，并设计实现了简单的应用层协议。

# 2.目录结构
相关的文件和目录的说明如下。
- BenchMark是基准性能压测工具的代码目录。
- BenchMark2是协程版的基准性能压测工具的代码目录。
- common是公共代码的目录。
- ConcurrencyModel是23种不同并发模型的代码目录。
- Coroutine是协程库实现的代码目录。
- EventDriven是事件驱动库的简单封装，在BenchMark2中会使用到。
- test目录为单元测试代码的目录。

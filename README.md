# muduo_linux
该项目深度借鉴了陈硕先生的muduo网络库，目前处于完善阶段。
使用c++11编写，依赖于标准库。

这是一个多线程服务器网络库。
多路I/O复用模型为epoll。

2019/5/10 更新了基于allocator的简易内存池。
2019/5/17 更新了基于timerfd的定时器。
2019/5/24 更新了日志系统。

一个典型的使用场景为：
tcpserver占用一个线程处理连接；
多个线程处理tcpconnection的epoll事件；
asynclogging占用一个线程处理日志写入磁盘。

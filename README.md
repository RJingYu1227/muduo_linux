# muduo_linux
该项目深度借鉴了陈硕先生的muduo网络库，目前还在开发中（主要目的是学习）。

这是一个多线程服务器。
多路I/O复用模型为epoll。
目前一个线程负责处理连接，一个或多个线程负责I/O。

tcpserver线程负责新建tcpconnection，完成相应的初始化，然后通知I/O线程将其channel注册到I/O线程的epoller当中。
线程间通信通过eventloop的runInLoop和eventfd实现，需要加锁。
I/O线程负责监听并处理tcpconnection的各种epoll_event，处理过程中会调用自定义的回调函数。
如果是eventfd对应的channel返回则会开始执行通过runInLoop添加到函数队列中的函数，需要加锁。

2019/5/10 更新了内存池。

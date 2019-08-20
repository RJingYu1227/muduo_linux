# muduo_linux
该项目深度借鉴了陈硕先生的muduo网络库，目前处于修补与完善阶段。
使用c++11编写，依赖于标准库。

2019/5/10 更新了基于allocator的简易内存池。
2019/5/17 更新了基于timerfd的定时器。
2019/5/24 更新了日志系统。
2019/6/21 更新了一个http测试用例。
2019/7/23 全新的asynclogging。
2019/8/20 更新了基于ucontext的coroutine。

特征：
1.Reactor模式
2.主线程为处理tcpserver的线程，线程池为处理tcpconnection的线程，附加线程为日志线程
3.一个连接建立的过程为：
poller监听到listenfd可读-->在loop循环中handleevent-->accept4，getIoloop，从memorypool中获取地址-->构造tcpconnection-->推送到对应的Ioloop-->Ioloop完成fd的注册
4.主动断开连接方式：shutdown/forceclose(withdelay)

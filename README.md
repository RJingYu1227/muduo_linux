# muduo_linux  
使用c++11编写，依赖于标准库，支持linux平台。  
  
特性：  
1. 异步日志基于disruptor的思想，多生产者-单消费者  
2. coroutine为非对称式协程，每一个线程都有一个协程调用栈，支持共享栈（但此种情况不能跨线程）  
3. 实现了两种基于I/O事件（epoll监听，reactor模式）的协程调度器  
4. coloop类似于传统的eventloop，one loop per thread  
5. coservice灵感来源于对称多处理机和boost的io_service  
6. net文件夹下的代码模仿muduo实现  
7. 包含一些对低层api的封装（以类的形式）  
8. 测试用例主要是一个http服务器，经过wrk测试，测试结果还未整理  

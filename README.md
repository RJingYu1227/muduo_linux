# muduo_linux
该项目深度借鉴了陈硕先生的muduo网络库，目前处于修补与完善阶段。
使用c++11编写，依赖于标准库。

2019/5/10 更新了基于allocator的简易内存池。
2019/5/17 更新了基于timerfd的定时器。
2019/5/24 更新了日志系统。
2019/6/21 更新了一个http测试用例。
2019/7/23 全新的asynclogging。
2019/8/20 更新了基于ucontext的coroutine。
2019/10/25 更新了对mysql相关api的简单封装。

注意：
目前出现了一个重大bug，在由coroutine::create产生的协程当中操作浮点寄存器会导致进程无异常退出。
gdb调试信息为[Inferior 1 (process xxxx) exited with code 0222]。

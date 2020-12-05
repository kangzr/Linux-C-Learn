### Libevent

Libevent是一个轻量级的基于事件驱动的高性能的开源网络库，支持多个平台，对多个平台的I/O复用技术进行了封装，Linux下集成poll, epoll；Windows集成了IOCP，但性能不如boost.asio:

[参考](https://www.cnblogs.com/nearmeng/p/4043548.html)

Reactor模式，是一种事件驱动机制。应用程序需要提供相应的接口并注册到Reactor上，如果相应的事件发生，Reactor将主动调用应用程序注册的接口，这些接口又称为“回调函数”。

libevent支持三种事件：网络IO，定时器和信号，定时器采用最小堆(Min Heap)，网络IO和信号采用双向链表

![libevent_managment](..\pic\libevent_managment.png)



##### libevent中利用函数指针实现多态，封装多中IO多路复用的技术

以下代码展示了如何利用函数指针实现多态

```c
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct event_base;

// 可理解为抽象基类
struct eventop {
    char * name;
    void * (*int) (struct event_base *);
};

struct event_base {
    struct eventtop * evsel;
};

static void *
epoll_init(struct event_base * base)
{
    printf("epoll_init\n");
}

// 可理解为继承eventop的子类
struct eventop epollops = {
    "epoll",
    epoll_init
};

int main(){
    struct event_base * base;
    base = malloc(sizeof(struct event_base));
    memset(&base->evsel, 0, sizeof(struct eventop));
    base->evsel = &epollops;
    base->evsel->init(base);  // 实际会调用epollops的epoll_init函数
    return 0;
}
```











### Libuv

高性能，事件驱动的IO库，跨平台



### Libev

高性能的事件循环库，支持多种事件类型，与此类似的事件循环库还有 libevent等









##### lighttpd














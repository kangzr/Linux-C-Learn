### 

服务器能够承载这些fd(最基本条件)：(reactor.c 能否同时承载100w的fd？)

1. 5w以上的响应请求
2. 对数据库的操作
3. 磁盘的操作
4. CPU占用率  <60%
5. 内存占用率  <80%

cat /proc/sys/net/ipv4/tcp_wmem

cat /proc/sys/fs/file-max

1. 操作系统fd的限制
2. file-max  : `sysctl -a | grep file-max`

3. Segment fault core dumped
4. 端口问题 



send(fd, )， 怎么通过fd把数据发出去？通过fd找到对应的五元组发送。

fd --> tcp **五元组** (sip, dip , sport, dport, proto) 标识



客户端与服务器，进入一个”阻塞“状态

1. 随机端口满了, 1024 - 65535
2. fd上限

解决： `nf_conntrack_max`的限制，限制客户端和服务端建立连接的第三次握手的限制。

​			线上是没有这个问题的，因为每个客户端IP地址不一样。



如何将客户端，每1000个链接的耗时变短，让服务器每秒可接入的数量更多？

- 多个accept放在一个线程
- 多个accept分配在不同的线程
- 多个accept分配在不同的进程 --> nginx



epoll 多线程， 网络IO的模型？

- 单线程同步：NTP
- 多线程同步：Natty
- 纯异步：Redis，HAProxy
- 半同步半异步：Natty
- 多进程同步：fastcgi
- 多线程异步：memcached
- 多进程异步：nginx
- 每请求每进程/线程：Apache/CGI
- 微进程框架：erlang/go/lua
- 协程框架：libco/ntyco



C10k(1w并发) --> c1000k(1百万并发) -->c10M

影响服务器性能的因素(cpu, 内存，网络，操作系统，应用程序)

epoll 突破c10k  



网络服务专题部分主要学（select/poll/epoll --> reactor --> 并发量测试）



1. 阅读和实现epoll
2. 阅读redis，**libevent**，libev，libuv的 reactor实现原理，

reactor + 线程池

### libevent源码  ---> reactor

### redis源码 -->reactor   ae.h ae.c

---









网络IO，会涉及到两个系统对象，一个是用户空间调用IO的进程/线程，另一个是内核空间的内核系统，比如发生IO操作read时，会经历两个阶段

- 等待数据准备就绪
- 将数据从内核copy到进程/线程中。

因为以上两个阶段各有不同，出现多种网络IO模型。

对于高并发编程，网络连接上的消息处理分为两个阶段：

- 等待消息准备好
- 消息处理

使用默认阻塞套接字时（一个线程捆绑处理一个连接），往往把两个阶段合而为一，这样线程就得休眠来等待消息准备好，导致高并发下线程会频繁的睡眠、唤醒，从而影响CPU的使用率

高并发编程方法当然时把两个阶段分开处理。即，等消息准备好的代码段和与处理消息的代码段分离。解决办法：

1. 套接字设置为非阻塞的
2. 线程主动查询消息是否准备好

这就是传送中的IO多路复用：处理等待消息准备好这件事，它可以同时处理多个连接。

作为一个高性能服务器，通常需要考虑处理三类事件：IO事件，定时事件及信号

#### Reactor模型

普通函数调用机制：程序调用某函数-->函数执行-->程序等待-->函数将结果和控制权返回给程序-->程序继续处理。

Reactor模型一种事件驱动机制，应用程序不主动调用某个API处理，**而是提供响应的接口并注册到Reactor上**，如果相应事件发生，Reactor主动调用应用程序注册的接口，这些接口又称为回调函数。

将所有要处理的IO事件注册到一个中心IO多路复用器上，同时主线程/进程阻塞在**多路复用器**上。一旦有IO事件到来或准备就绪，多路复用器返回并将事件先注册到相应IO事件分发到对应的处理中。

Reactor模型三个重要组件：

- 多路复用器：操作系统提供，在linux上一般是select，pool, epoll等系统调用
- 事件分发器：将多路复用器中返回的就绪事件分到对应的处理函数中
- 事件处理器：负责处理特定事件的处理函数

![reactor_io](..\pic\reactor_io.png)



具体流程：

- 注册读就绪事件和相应的事件处理器
- 事件分离器等待事件
- 事件到来，激活分离器，分离器调用事件对应的处理器
- 事件处理器完成实际的读操作，处理读到的数据，注册新得事件，然后返回控制权。

Reactor模式是编写高性能网络服务器得必备技术之一，具有以下优点：

- 响应快，不必为单个同步事件所阻塞，虽然reactor本身依然同步
- 编程简单，最大程度避免复杂的多线程及同步问题，避免多线程/进程的切换开销
- 可扩展性，可以方便的通过增加reactor实例个数来充分利用cpu资源
- 可复用性，reactor框架本身与具体事件处理逻辑无关，具有很高的复用性

**代码实现：**

```c
typedef int CALLBACK(int, int, void*);

struct event_data {
    int fd;
    int events;
    void *arg;
    int (*callback)(int fd, int events, void *arg);
    int status;
    char buffer[BUFFER_LENGTH];
    int length;
};

// reactor 结构体
struct reactor {
    int epfd;
    struct event_data *events;
};

int reactor_init(struct reactor *r) {
    if (r == NULL) return -1;
    memset(r, 0, sizeof(struct reactor));
    r->epfd = epoll_create(1);
    if (r->epfd <= 0){
        return -2;
    }
    r->events = (struct event_data*)malloc(1024 * sizeof(struct event_data));
    if (r->events){
        close(r->epfd);
        return -3;
    }
    return 0;
}

int reactor_addlistener(struct reactor *r, int sockfd, CALLBACK * acceptor) {
    if (r == NULL) return -1;
    if (r->events == NULL) return -1;
    set_event(&r->events[sockfd], sockfd, acceptor, r);
    add_event(r->epfd, EPOLLIN, &r->events[sockfd]);
    return 0;
}

int reactor_run(struct reactor *r) {
    if (r == NULL || r->epfd < 0 || r->events == NULL)
        return -1;
    struct epoll_event events[1024 + 1];
    while (1) {
        int nready = epoll_wait(r->epfd, events, 1024, 1000);
        if (nready < 0){
            continue;
        }
        for (i = 0; i < nready; i++) {
            struct event_data *ev = (struct event_data*)events[i].data.ptr;
            if ((events[i].events & EPOLLIN) && (ev->events & EPOLLIN))
                ev->callback(ev->fd, events[i].events, ev->arg);
            if ((events[i].events & EPOLLOUT) && (ev->events & EPOLLOUT))
                ev->callback(ev->fd, events[i].events, ev->arg);
        }
    }
    return 0;
}

int reactor_destroy(struct reactor *r) {
    close(r->epid);
    free(r->events);
    return 0;
}

int init_sock(short port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(fd, F_SETFL, O_NONBLOCK);
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(serer_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (listen(fd, 20) < 0) {
        printf("listen failed: %s\n", strerror(errno));
    }
    return fd;
}

int main(int argc, char *argv[]) {
    unsigned short port = 8888;
    int sockfd = init_sock(port);
    struct reactor *r = (struct reactor *)malloc(sizeof(struct reactor));
    reactor_init(r);
    reactor_addlistener(r, sockfd, accept_cb);
    reactor_run(r);
    reactor_destroy(r);
    close(sockfd);
    return 0;
}
```














































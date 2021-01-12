#### Reactor和Proactor模型

对于高并发编程，网络连接上的消息处理分为两个阶段：1. 等待消息准备好；2. 消息处理

使用默认阻塞套接字时（一个线程捆绑处理一个连接），往往把两个阶段合而为一，因此线程就得休眠来等待消息准备好，导致高并发下线程会频繁的睡眠、唤醒，从而影响CPU的使用率

高并发编程方法当然时把两个阶段分开处理。即，等消息准备好的代码段和与处理消息的代码段分离。解决办法（IO多路复用模型）：

1. 套接字设置为非阻塞的
2. 线程主动查询消息是否准备好

> 作为一个高性能服务器，通常需要考虑处理三类事件：IO事件，定时事件及信号

Reactor和Proactor为两种IO多路复用模型；



Reactor：能收了跟我说一声

Proactor：你给我收十个字节，收好了跟我说一声



##### 1. Reactor模型

普通函数调用机制：程序调用某函数-->函数执行-->程序等待-->函数将结果和控制权返回给程序-->程序继续处理。

Reactor模型一种事件驱动机制：程序不主动调用某个API处理，而是提供相应的接口并注册到Reactor上，如果相应事件发生，Reactor主动调用应用程序注册的接口，这些接口又称为回调函数。

将所有要处理的IO事件注册到一个中心IO多路复用器上，同时主线程/进程阻塞在多路复用器上。一旦有IO事件到来或准备就绪，多路复用器返回并将事件先注册到相应IO事件分发到对应的处理中。

Reactor模型三个重要组件：

- 多路复用器：操作系统提供，在linux上一般是select，pool, epoll等系统调用
- 事件分离器（Event Demultiplexer）：将多路复用器中返回的就绪事件分到对应的处理函数中；
- 事件处理器（Event Handler）：负责处理特定事件的处理函数

![reactor_io](..\pic\reactor_io.png)

两个与事件分离器有关的莫模型：Reactor和Proactor，前者采用同步IO，后者采用异步IO

Reactor中负责等待socket等待就绪，然后将就绪事件传递给对应的事件处理器，最后由处理器完成实际读写工作。

Reactor实例模型：

![my_reactor](..\pic\my_reactor.png)



具体流程：

- 注册读就绪事件和相应的事件处理器
- 事件(多路)分离器(Event Demultiplexer)等待事件
- 事件到来，激活分离器，分离器调用事件对应的处理器
- 事件处理器完成实际的读操作，处理读到的数据，注册新得事件，然后返回控制权。

Reactor模式是编写高性能网络服务器得必备技术之一，具有以下优点：

- 响应快，不必为单个同步事件所阻塞，**虽然reactor本身依然同步**
- 编程简单，最大程度避免复杂的多线程及同步问题，避免多线程/进程的切换开销
- 可扩展性，可以方便的通过增加reactor实例个数来充分利用cpu资源（一个核心对应一个reactor，nginx）
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



##### Proactor模型

事件分离器只负责发起异步读写操作，IO操作本身由操作系统来完成，需要将用户定义的数据缓冲区地址和数据大小传给操作系统，其才能从中得到写出操作所需数据，或写入socket读到的数据。事件分离器捕获IO操作完成事件并传递给对应事件处理器，由事件处理器获取IO操作结果，并对数据进行处理。



<img src="..\pic\proactor.png" alt="proactor" style="zoom:60%;" />

1. 处理器发起异步操作，并关注IO完成事件
2. 事件分离器等待操作完成事件
3. 分离器等待过程中，操作系统利用并行的内核现场执行实际的IO操作，并将结果数据存入用户自定义缓冲区，最后通知事件分离器读操作完成
4. IO完成后，通过事件分离器呼唤处理器
5. 事件处理器处理用户自定义缓冲区中的数据，然后启动一个新的异步操作，并将控制权返回事件分离器



Proactor模型最大的特点是使用异步IO，所有IO操作都交给系统体统的异步IO接口执行。工作线程仅负责业务逻辑。



![my_proactor](..\pic\my_proactor.png)

增加了编程复杂度，但给工作线程带来了更高效率。Windows中没有epoll机制，提供IOCP来支持高并发，由于操作系统作了较好的优化，windows较常采用Proactor的模型利用完成端口来实现服务器。linux2.6后也有aio接口，但是效果不理想，不如epoll性能。linux5.1后引入`io_uring`(一套全新的syscall，一套全新的async API，更高的性能，更好的兼容性，来迎接高IOPS，高吞吐量的未来)





事件驱动epoll模式与异步IO模式有啥区别？

epoll_wait()是一个阻塞操作，直到有事件触发时通知处理，并将event分发到不同的function处理

AIO：把回调函数的地址传给系统，系统在操作完成(数据读取完成)后调用回调函数



---



fd --> tcp 五元组 (sip, dip , sport, dport, proto) 标识

客户端与服务器，进入一个”阻塞“状态：1. 客户端随机端口满了, 1024 - 65535；2. 服务端fd上限

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

C10k(1w并发) --> c1000k(1百万并发) -->c10M（epoll 突破c10k  ）

影响服务器性能的因素(cpu, 内存，网络，操作系统，应用程序)

网络服务专题部分主要学（select/poll/epoll --> reactor --> 并发量测试）

1. 阅读和实现epoll
2. 阅读redis，**libevent**，**libev**，libuv的 reactor实现原理，




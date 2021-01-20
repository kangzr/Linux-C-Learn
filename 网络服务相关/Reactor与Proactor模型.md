#### 前言

在网络模型一文中，我们介绍了多种网络IO模型，其中包括IO多路复用模型，在此基础上，本文将介绍两种高性能IO模型：Reactor和Proactor，Reactor应用于同步IO，Proactor应用于异步IO。

#### Reactor模型

普通函数调用流程如下：

![reactor_normal](D:\MyGit\Linux-C-Learn\pic\reactor_normal.png)

Reactor模型一种事件驱动（回调）机制：程序不主动调用某个API处理，而是相应事件发生，Reactor主动调用应用程序注册的接口进行处理。

中心思想：将所有要处理的IO事件注册到一个IO多路复用器上，同时主线程/进程阻塞在多路复用器上。一旦有IO事件到来或准备就绪，多路复用器返回并调用相应的事件处理函数。

<img src="..\pic\my_reactor.png" alt="my_reactor" style="zoom:75%;" />

接下来将具体介绍三种Reactor模型；

##### Reactor单线程模型（Single threaded version）

<img src="D:\MyGit\Linux-C-Learn\pic\reactor1.png" alt="reactor1" style="zoom:75%;" />

Reactor模型三个重要组件：

- 多路复用器(Reactor)：负责监听注册进来的IO事件（对应图中Reactor）
- 事件分离器（Event Demultiplexer）：将多路复用器中返回的就绪事件分到对应的处理函数中；（对应图中dispatch）
- 事件处理器（Event Handler）：负责处理特定事件的处理函数（对应上图read,decode,compute,encode,send等）

该模型中，Reactor线程**既要**负责监听新连接的到来，**又要**dispatch请求到handler中；

消息处理流程如下：

- Reactor通过epoll监听连接事件，收到事件后通过dispatch转发
- 如果是连接建立的事件，则有acceptor接受连接，并创建handler处理后续事件
- 如果不是建立连接事件，Reactor会分发调用handler来响应
- handler完成read->业务处理->send的完整业务流程

以下为一个简化版的Reactor单线程模型

```c
// reactor初始化 创建多路复用器
int reactor_init(struct reactor *r) {
    r->epfd = epoll_create(1);
}
// reactor驱动
int reactor_run(struct reactor *r) {
    if (r == NULL || r->epfd < 0 || r->events == NULL)
        return -1;
    struct epoll_event events[1024 + 1];
    while (1) {  // 死循环
        // 有事件到来时通过epoll_wait返回
        int nready = epoll_wait(r->epfd, events, 1024, 1000);
        if (nready < 0){
            continue;
        }
        for (i = 0; i < nready; i++) {
            struct event_data *ev = (struct event_data*)events[i].data.ptr;
            if (events[i].events & EPOLLIN)  // available for read
                ev->callback(ev->fd, events[i].events, ev->arg);  // 调用读事件的回调函数处理
            if (events[i].events & EPOLLOUT)  //available for write
                ev->callback(ev->fd, events[i].events, ev->arg);  // 调用写事件的回到函数处理
        }
    }
    return 0;
}
```

Reactor单线程模型只适用于小容量应用场景，但对于高负载，高并发场景并不适用，主要原因：

- 无法充分利用CPU核心（只能利用一个CPU核心），无法满足海量数据处理
- 高并发时，Reactor线程过载后处理速度会变慢，导致大量客户端连接超时，导致大量挤压信息和处理超时，成为性能瓶颈
- 一旦Reactor线程意外中断或者进入死循环，会导致整个系统通信模块不可用，造成节点故障

为了解决上述问题，演进出单Reactor多线程模型（线程池）



##### Reactor多线程模型（Multithreaded Designs）

<img src="D:\MyGit\Linux-C-Learn\pic\reactor_2.png" alt="reactor_2" style="zoom:75%;" />

消息处理流程

- Reactor通过epoll监听连接事件，收到事件后通过dispatch转发
- 如果是连接建立的事件，则有acceptor接受连接，并创建handler处理后续事件；
- 如果不是建立连接事件，则Reactor会分发调用连接对应的Handler来响应。
- Handler只负责响应事件，不做具体业务处理，通过Read读取数据后，会分发给后面的Worker线程池进行业务处理。
- Worker线程池会分配独立的线程完成真正的业务处理，如何将响应结果发给Handler进行处理。
- Handler收到响应结果后通过send将响应结果返回给Client。



相对于第一种模型来说，在处理业务逻辑，也就是获取到IO的读写事件之后，交由线程池来处理，handler收到响应后通过send将响应结果返回给客户端。这样可以降低Reactor的性能开销，从而更专注的做事件分发工作了，提升整个应用的吞吐量。

但是这个模型存在的问题：

- 多线程数据共享和访问比较复杂。如果子线程完成业务处理后，把结果传递给主线程Reactor进行发送，就会涉及共享数据的互斥和保护机制。
- Reactor承担所有事件的监听和响应，只在主线程中运行，可能会存在性能问题。例如并发百万客户端连接，或者服务端需要对客户端握手进行安全认证，但是认证本身非常损耗性能。

为了解决性能问题，产生了第三种主从Reactor多线程模型。

##### 主从Reactor多线程模型（Multiple Reactors）

<img src="D:\MyGit\Linux-C-Learn\pic\reactor_main.png" alt="reactor_main" style="zoom:75%;" />

将Reactor分为两部分：

- mainReactor负责监听server socket，用来处理网络IO连接建立操作，将建立的socketChannel指定注册给subReactor。
- subReactor主要做和建立起来的socket做数据交互和事件业务处理操作。通常，subReactor个数上可与CPU个数等同。

Nginx、Swoole、Memcached和Netty都是采用这种实现。

消息处理流程：

- 从主线程池中随机选择一个Reactor线程作为acceptor线程，用于绑定监听端口，接收客户端连接
- acceptor线程接收客户端连接请求之后创建新的SocketChannel，将其注册到主线程池的其它Reactor线程上，由其负责接入认证、IP黑白名单过滤、握手等操作
- 步骤2完成之后，业务层的链路正式建立，将SocketChannel从主线程池的Reactor线程的多路复用器上摘除，重新注册到Sub线程池的线程上，并创建一个Handler用于处理各种连接事件
- 当有新的事件发生时，SubReactor会调用连接对应的Handler进行响应
- Handler通过Read读取数据后，会分发给后面的Worker线程池进行业务处理
- Worker线程池会分配独立的线程完成真正的业务处理，如何将响应结果发给Handler进行处理
- Handler收到响应结果后通过Send将响应结果返回给Client



Reactor模式是编写高性能网络服务器得必备技术之一，具有以下优点：

- 响应快，不必为单个同步事件所阻塞，**虽然reactor本身依然同步**
- 编程简单，最大程度避免复杂的多线程及同步问题，避免多线程/进程的切换开销
- 可扩展性，可以方便的通过增加reactor实例个数来充分利用cpu资源（一个核心对应一个reactor，nginx）
- 可复用性，reactor框架本身与具体事件处理逻辑无关，具有很高的复用性





---



##### Proactor模型

事件分离器只负责发起异步读写操作，IO操作本身由操作系统来完成，需要将**用户定义的数据缓冲区地址和数据大小**传给操作系统，其才能从中得到写出操作所需数据，或写入socket读到的数据。事件分离器捕获IO操作完成事件并传递给对应事件处理器，由事件处理器获取IO操作结果，并对数据进行处理。

工作流程：

1. 处理器发起异步操作，并关注IO完成事件
2. 事件分离器等待操作完成事件
3. 分离器等待过程中，操作系统利用并行的内核现场执行实际的IO操作，**并将结果数据存入用户自定义缓冲区**，最后通知事件分离器读操作完成
4. IO完成后，通过事件分离器呼唤处理器
5. 事件处理器处理用户自定义缓冲区中的数据，然后**启动一个新的异步操作**，并将控制权返回事件分离器

Proactor模型最大的特点是使用异步IO，所有IO操作都交给系统体统的异步IO接口执行。工作线程仅负责业务逻辑。

优点：给工作线程带来了更高的效率；

缺点：

- 编程复杂性：异步操作流程的事件的初始化和事件完成在时间和空间上都是相互分离的，异步编程更加复杂；
- 内存使用：缓冲区在读写操作的时间段内必须保持住，可能造成不确定性，并且每个并发操作都要求有独立的缓存；Reactor模型在读写准备好前不需要开辟缓存
- 操作系统支持：Windows通过IOCP实现真正的异步IO；linux2.6后也有aio接口，但是效果不理想，不如epoll性能。linux5.1后引入`io_uring`(一套全新的syscall，一套全新的async API，更高的性能，更好的兼容性，来迎接高IOPS，高吞吐量的未来)



Reactor和Proactor的相同点都是通过回调方式通知有IO事件到来，并由handler进行处理，不同点在于Proactor回调handler时IO操作已经完成（异步），Reactor回调handler时需要先进行IO操作（同步）。

例如：Reactor中注册读事件，当fd可读时，调用者需要自己调用read读取数据，而Proactor在注册读事件时，同时会提供一个buffer用于存储读取的数据，那么Proactor通过回掉函数通知用户时，用户无需再调用read读取数据，数据已经再buffer中了。



参考：

[nio](http://gee.cs.oswego.edu/dl/cpjslides/nio.pdf)

[Reactor 和 Proactor](https://cloud.tencent.com/developer/article/1488120)

C10k(1w并发) --> c1000k(1百万并发) -->c10M（epoll 突破c10k  ）

影响服务器性能的因素(cpu, 内存，网络，操作系统，应用程序)

网络服务专题部分主要学（select/poll/epoll --> reactor --> 并发量测试）

1. 阅读和实现epoll
2. 阅读redis，**libevent**，**libev**，libuv的 reactor实现原理，

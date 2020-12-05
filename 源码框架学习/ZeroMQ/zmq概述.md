#### zmq概述

[参考](https://www.cnblogs.com/chenny7/p/6245236.html)

对传统网络，是一个种基于消息队列的多线程网络库，提供的套接字可在多种协议种传输消息，如线程间，进程间，TCP，广播等;

ZMQ介于应用层和传输层之间（按照TCP/IP划分）（ZMQ是一个简单好用的传输层，像框架一样的一个 socket library），是一个可伸缩层，可并行运行，分散在分布式系统间；

ZMQ 的明确目标是成为标准网络协议栈的一部分，之后进入 Linux 内核；

ZMQ不是单独的服务，而是一个嵌入式库，它封装了网络通信，消息队列，线程调度等功能，向上层提供简洁的API，应用程序通过加载库文件，调用API函数来实现高性能网络通信。

![zeromq_pos](..\..\pic\zeromq_pos.png)



**IO线程**：ZMQ根据用户调用`zmq_init`函数时传入的参数，创建对应数量的IO线程，每个IO线程都有与之绑定的Poller，Poller采用经典的Reactor模式实现；Poller根据不同操作系统平台使用不同的网络IO模型（select，poll，epoll，devpoll，kequeue等），所有IO操作都是异步的，线程不会被阻塞。

**主线程**：与IO线程通过Mail Box传递消息进行通信。

**Server**：在主线程创建`zmq_listener`，通过MailBox发消息的形式将其绑定到IO线程，IO线程把zmq_listener添加到Poller中用以监听读事件

**Client**：在主线程中创建`zmq_connecter`，通过MailBox发消息的形式将其绑定到IO线程，IO线程把zmq_connecter添加到Poller中用以监听写事件

**Session**：Client与Server第一次通信时，会创建`zmq_init`来发送identity，用以进行认证；认证结束后，双方会为此次连接创建Session，以后双方就通过Session进行通信

**Pipe**：每个Session都会关联到相应的读写管道，主线程收发消息只是分别从管道中读/写数据，Session并不实际跟Kernel交换IO数据，而是通过plugin到Session中的Engine来与kernel交换IO数据。

![zeromq_arch](..\..\pic\zeromq_arch.png)





io线程怎么创建

pipe_t  zmq::ypipe_t 双向 （两个）**处理数据**

tcp_connector_t：对连接的封装

signaler_t   主线程(poll)，io线程(epoll_wait)

mailbox_t  双向 （两个）**处理命令**

stream_engine_t: :out_event







zmq api大部分都是异步调用

zmq_connect  非阻塞 （真正执行连接的是io线程，不是main线程）

zmq_send 非阻塞

zmq_recv 阻塞



Engine，绑定fd，真正收发数据的地方



零拷贝问题：不同函数直接的调用，不产生数据的copy



##### msg_t

- vmsg短消息机制：栈分配
- lmsg长消息机制：堆分配
- 零拷贝



一个io线程对应一个mailbox（io_thread_t和socket_base_t都有mailbox）

- 客户端：每个连接都对应一个engine，connect成功的时候创建
- 服务端：每个连接都是对应一个engine，accept成功的时候创建

服务器已经断开：

- 连接不成功加入到定时器：tcp_connector_t::add_connect_timer（默认100ms）
- 还是不成功，重选定时器：tcp_connector_t::add_connect_timer



##### mailbox_t

`_cpipe`封装无锁队列，zmq中的无锁队列仅支持单写单读



用户API都是在main主线程调用



send_plug



调式zmq：`./configure --enable-debug`



可用于实现多种模式：

##### 请求-应答模式

##### 发布(PUB)-订阅(SUB)模型，

##### 管道模式

















```c
// Receive 0MQ string from socket and convert into C string
// Caller must free returned string. Returns NULL if the context is being terminated.
static char *
s_recv(void * socket) {
    char buffer[256];
    int size = zmq_recv(socket, buffer, 255, 0);
    if (size == -1)
        return NULL;
    if (size > 255)
        sizes = 255;
    buffer[size]  = '\0';
    return strdup(buffer, sizeof(buffer) - 1);
}

// Convert C string to 0MQ string and send to socket
static int
s_send(void * socket, char * string) {
    int size = zmq_send(socket, string, strlen(string), 0);
    return size;
}
```













gdb

```shell
# 锁定线程不切换 / 解锁
set scheduler-locking on/off 

# 禁止断点
disable 6

# r 重启

# bt 打印调用栈
```


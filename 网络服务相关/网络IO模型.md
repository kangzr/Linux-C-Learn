#### 网络IO模型

网络IO涉及用户空间和内核空间，一般会经历两个阶段：

- 一阶段：等待数据准备就绪，即等待网络数据被copy到内核缓冲区（wait for data）
- 二阶段：将数据从内核缓冲区copy到用户缓冲区（copy data from kernel to user）

上述数据准备就绪可理解为socket中有数据到来，根据以上两阶段的不同，出现多种网络IO模型，接下来将根据这两阶段来进行分析。



##### 1.  阻塞IO（Blocking IO）

linux中socket默认blocking，从下图可以看出，用户进程全程阻塞直到两阶段完成，即，一阶段等待数据会block，二阶段将数据从内核copy到用户空间也会block，只有copy完数据后内核返回，用户进程才会解除block状态，重新运行。

**结论：阻塞IO，两阶段都阻塞。**

![](..\pic\io_blocking.png)



##### 2.  非阻塞IO (Non-blocking IO)

可使用`fcntl`将socket设置为`NON-BLOCKING`(`fcntl(fd, F_SETFL, O_NONBLOCK);`)，使其变为非阻塞。如下图，用户进程recvfrom时，如果没有数据，则直接返回，因此一阶段不会阻塞用户进程。但是，用户进程需要不断的询问kernel数据是否准备好（会造成CPU空转浪费资源，因此很少使用）。当数据准备好时，用户进程会阻塞直到数据从内核空间copy到用户空间完成（二阶段），内核返回结果。

**结论：非阻塞IO一阶段不阻塞，二阶段阻塞。**

<img src="..\pic\nonblocking_io.png" alt="nonblocking_io" style="zoom:100%;" />

图中recvfrom 返回值含义：

- EWOULDBLOCK, 无数据
- 大于0，接收数据完毕，返回值即收到的字节数
- 等于0，连接已经正常断开
- 等于-1，errno==EAGAIN表示recv操作未完成，errno!=EAGAIN表示recv操作遇到系统错误errno.



##### 3. IO多路复用 (IO multiplexing-select/poll/epoll)

也称为事件驱动IO(event driven IO)，通过使用select/poll/epoll系统调用，可在单个进程/线程中同时监听多个网络连接的(socket fd)，一旦有事件触发则进行相应处理，其中select/epoll是阻塞的。（IO多路复用后面会用专门文章来详细讲解）

**结论：两阶段都处于阻塞状态，优点是单个线程可同时监听和处理多个网络连接**

<img src="..\pic\io_multiplexing.png" alt="io_multiplexing" style="zoom:100%;" />

>  如果连接数不是很高的话，使用select/epoll的web server不一定比使用multi-threading + blocking IO的web server性能更好，可能延迟更大。因为前者需要两个系统调用(select/epoll + read)，而后者只有一个(read)。但是在连接数很多的情况下，select/epoll的优势就凸显出来了。（高效事件驱动模型libevent，libev库）



##### 4. 信号驱动IO (signal driven IO, SIGIO)

通过sigaction系统调用，建立起signal-driven I/O的socket，并绑定一个信号处理函数；sigaction不会阻塞，立即返回。

当数据准备好，内核就为进程产生一个SIGIO信号，随后在信号处理函数中调用recvfrom接收数据。

**结论：一阶段不阻塞，二阶段阻塞**

<img src="D:\MyGit\Linux-C-Learn\pic\sigio.png" alt="sigio" style="zoom:120%;" />

以上四种模型都有一个共同点：二阶段阻塞，也就是在真正IO操作(recvfrom)的时候需要用户进程参与，因此以上四种模型均称为同步IO模型。



##### 5. 异步IO （Asynchronous IO）

POSIX中提供了异步IO的接口aio_read和aio_write，如下图，内核收到用户进程的aio_read之后会立即返回，不会阻塞，aio_read会给内核传递文件描述符，缓冲区指针，缓冲区大小，文件偏移等；当数据准备好，内核直接将数据copy到用户空间，copy完后给用户进程发送一个信号，进行用户数据异步处理（aio_read）。因此，异步IO中用户进程是不需要参与数据从内核空间copy到用户空间这个过程的，也即二阶段不阻塞。

**结论：两阶段都不阻塞**

<img src="..\pic\aysnchronous_io.png" alt="aysnchronous_io" style="zoom:100%;" />



##### 上述五种IO模型对比

从上述分析可以得知，阻塞和非阻塞的区别在于内核数据还没准备好时，用户进程是否会阻塞（一阶段是否阻塞）；同步与异步的区别在于当数据从内核copy到用户空间时，用户进程是否会阻塞/参与（二阶段是否阻塞）。以下为五种IO模型的对比图，可以清晰看到各种模型各个阶段的阻塞情况。

![five_ios](..\pic\five_ios.png)



以下为Richard Stevens对同步和异步IO的描述，可以把I/O operation是否阻塞看作为两者的区别。

> A synchronous I/O operation causes the requesting process to be blocked until that I/O operation completes;
>
> An asynchronous I/O operation does not cause the requesting process to be blocked;

还有一个概念需要区分：异步IO和IO异步操作，IO异步操作其实是属于同步IO模型。



##### 6.  io_uring

POSIX中提供的异步IO接口aio_read和aio_write性能一般，几乎形同虚设，很少被用到，性能不如Epoll等IO多路复用模型。

Linux 5.1引入了一个重大feature：io_uring，由block IO大神Jens Axboe开发，这意味着Linux native aio的时代即将称为过去，io_uring的异步IO新时代即将开启。

以下贴出Jens Axboe的测试数据

![io_uring_data](..\pic\io_uring_data.png)

从以上数据看出，在非Polling模式下，io_uring性能提升不大，但是polling模式下，io_uring性能远远超出libaio，并接近spdk



io_uring围绕高效进行设计，采用一对共享内存ringbuffer用于应用和内核间通信，避免内存拷贝和系统调用（感觉这应该是io_uring最精髓的地方)：

- 提交队列(SQ)：应用是IO提交的生产者，内核为消费者
- 完成队列(CQ)：内核是完成事件的生产者，应用是消费者



**io_uring系统调用相关接口**

```c
// 1. 初始化阶段
// The io_uring_setup() system call sets up a submission queue (SQ) and completion queue (CQ)
// returns a file descriptor which can be used to perform subsequent operations on the io_uring instance
// The submission and completion queues are shared between userspace and the kernel
// which eliminates the need to copy data when initiating and completing I/O
// 其中SQ, CQ为ringbuffer.
// io_setup返回一个fd，应用程序使用这个fd进行mmap，和kernel共享一块内存
int io_uring_setup(u32 entries, struct io_uring_params *p);
```

```c
// IO提交的做法是找到一个空闲的 SQE，根据请求设置 SQE，并将这个 SQE 的索引放到 SQ 中
// SQ 是一个典型的 RingBuffer，有 head，tail 两个成员，如果 head == tail，意味着队列为空。
// SQE 设置完成后，需要修改 SQ 的 tail，以表示向 RingBuffer 中插入一个请求。
int io_uring_enter(unsigned int fd, unsigned int to_submit,
                unsigned int min_complete, unsigned int flags,
                sigset_t *sig);
```

```c
/* * register files or user buffers for asynchronous I/O * * The io_uring_register() system call registers user buffers or files for use in an io_uring(7) instance * referenced by fd. Registering files or user buffers allows the kernel to take long term references to * internal data structures or create long term mappings of application memory, greatly reducing * per-I/O overhead. */
int io_uring_register(unsigned int fd, unsigned int opcode,
                void *arg, unsigned int nr_args)
```



**liburing**

为方便使用，Jens Axboe还开发了一套liburing库，用户不必了解诸多细节，简单example如下

```c
/* setup io_uring and do mmap */
io_uring_queue_init(ENTRIES, &ring, 0);
/* get an sqe and fill in a READV operation */
sqe = io_uring_get_sqe(&ring);
io_uring_prep_readv(sqe, fd, &iovec, 1, offset);
/* tell the kernel we have an sqe ready for consumption */
io_uring_submit(&ring);
/* wait for the sqe to complete */
io_uring_wait_cqe(&ring, &cqe);
/* read and process cqe event */
io_uring_cqe_seen(&ring, cqe);
/* tear down */
io_uring_queue_exit(&ring);
```



通过全新的设计，共享内存，IO 过程不需要系统调用，由内核完成 IO 的提交， 以及 IO completion polling 机制，实现了高IOPS，高 Bandwidth。相比 kernel bypass，这种 native 的方式显得友好一些。



参考：《UNIX Network Programming Volume.1.3rd.Edition》 --- Richard Stevens

[io_uring1](https://kernel.taobao.org/2019/06/io_uring-a-new-linux-asynchronous-io-API/)

[io_uring2](https://cloud.tencent.com/developer/article/1458912)
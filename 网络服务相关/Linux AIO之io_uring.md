Linux Kernel即将在5.1版本中加入重大feature：`io_uring`，一套全新的syscall，一套全新的aync API，更高的性能，更好的兼容性，来迎接高IOPS，高吞吐量的未来。

io_uring特性

- 用户态和内核态通过ringbuffer来进行通信，io_uring设计了一对共享ring_buffer，针对提交队列(SQ)，用户为IO提交的生产者，内核为消费者；针对完成队列(CQ)，内核是完成事件的生产者，用户为消费者
- IO 提交和收割可以 offload 给 Kernel，且提交和完成不需要经过系统调用（system call）
- 支持 Block 层的 Polling 模式
- 通过提前注册用户态内存地址，减少地址映射的开销

```c
// syscall
/* * setup a context for performing asynchronous I/O * * The io_uring_setup() system call sets up a submission queue (SQ) and completion queue (CQ) with * at least entries entries, and returns a file descriptor which can be used to perform subsequent * operations on the io_uring instance.The submission and completion queues are shared between * userspace and the kernel, which eliminates the need to copy data when initiating and completing * I/O. */
int io_uring_setup(u32 entries, struct io_uring_params *p);

/* * initiate and/or complete asynchronous I/O * * io_uring_enter() is used to initiate and complete I/O using the shared submission and completion * queues setup by a call to io_uring_setup(2). A single call can both submit new I/O and wait for * completions of I/O initiated by this call or previous calls to io_uring_enter(). */
int io_uring_enter(unsigned int fd, unsigned int to_submit,
                unsigned int min_complete, unsigned int flags,
                sigset_t *sig);

/* * register files or user buffers for asynchronous I/O * * The io_uring_register() system call registers user buffers or files for use in an io_uring(7) instance * referenced by fd. Registering files or user buffers allows the kernel to take long term references to * internal data structures or create long term mappings of application memory, greatly reducing * per-I/O overhead. */
int io_uring_register(unsigned int fd, unsigned int opcode,
                void *arg, unsigned int nr_args)
```







[参考1](https://zhuanlan.zhihu.com/p/62682475)

[参考2](https://kernel.taobao.org/2019/06/io_uring-a-new-linux-asynchronous-io-API/)
- 初始化 init：
- commit：向服务端提交请求
- callback：另外线程处理返回结果
- destroy



实现这样一个框架：叫做协程

一个commit和对应callback在一个调度单元中

commit请求，让出cpu，epoll_wait检测有数据，则resume返回cpu；

（IO密集型，协程可以抗住；如果是计算密集型则不行；）

---



#### 进程，线程与协程

进程：最小的资源管理单元；

​		   切换者：OS；切换内容：页全局目录+内核栈+硬件上下文；切换内容需保存到内核栈中；切换过程：用户态--内核态--用户态



线程：最小的执行单元/CPU调度基本单元，从属于进程，是程序的实际执行者，每个线程有自己的栈空间

​			切换者：OS；切换内容：内核栈+硬件上下文；切换内容需保存到内核栈中；用户态--内核态--用户态



多进程程序安全性高，进程切换开销大，数据同步效率低；多线程程序维护成本高，需要用锁来解决资源竞争问题，线程切换开销小，数据同步效率高；



协程：`A coroutine is a function that can suspend its execution (yield) until the given yieldInstruction finishes`

​	协程不是被操作系统内核管理，而是由用户态程序控制，因此不需要像线程那样进行上下文切换，这就是为什么协程的开销

​	远小于线程的开销

​	切换者：用户态/程序；切换内容：硬件上下文；切换内容保存到用户栈或堆；切换不需要进入内核态



**协程适用于IO密集型任务**，计算密集型还得多线程

优势：

- 跨平台，跨体系架构
- 无需线程上下文切换的开销
- 无需原子操作锁定以及同步的开销
- 方便切换控制流，简化编程模型
- 高并发+高扩展+低成本：一个CPU支持上万协程不是问题（线程就不行）

缺点：

- 无法利用直接多核资源：协程本制上是单线程，不能同时将单个CPU的多个核用上，但是可以和进程配合，将进程挂在指定核心上，便可充分利用核心
- 进行阻塞操作时会阻塞掉整个程序



Q： **为什么要有协程？解决了什么问题？其存在必要性？**

A： **同步性能不如异步，异步不如同步编程直观**

理解同步与异步概念（查看http的异步实现）

客户端

同步：等待结果返回，每秒请求数量不会很多

异步：不需要等待结果返回

服务端：

同步：epoll_wait与recv send在同一个流程

异步：不在一个流程（IO异步操作不是真正的异步；区别异步IO叫法aio，windows iocp）

IO同步操作：管理方便，程序整体逻辑清晰，响应时间长，性能差（epoll_wait与recv send在同一个流程）

IO异步操作：多个线程共同管理，响应时间短，性能好；



---



##### 1. 如何实现一个协程?

每次send或recv之前进行切换，再由调度器来处理epoll_wait的流程

封装接口：

- 协程创建

  新协程创建后加入到调度器的就绪队列

  ```c
  int nty_coroutine_create(nty_coroutine **new_co, proc_coroutine func, void *arg);
  /*
   *  nty_coroutine new_co: 传入空的协程对象
   *  proc_coroutine func：协程被调度时，执行该函数
   *  arg: 需要传入新协程中的参数
   */
  // 新协程创建后加入到调度器的就绪队列
   TAILQ_INSERT_TAIL(&co->sched->ready, co, ready_next);
  ```

- 协程调度器的运行

  ```c
  void nty_schedule_run(void)
  ```

- POSI异步封装API

  ```c
  int nty_socket(int domain, int type, int protocol);
  int nty_accept(int fd, struct sockaddr *addr, socklen_t *len);
  int nty_recv(int fd, void *buf, int length);
  int nty_send(int fd, const void *buf, int length);
  int nty_close(int fd)
  ```



##### 2. 实现IO异步操作

```c
while (1) {
    int nready = epoll_wait(epfd, events, EVENT_SIZE, -1);
    
    for (i = 0;i < nready;i ++) {
        int sockfd = events[i].data.fd;
        if (sockfd == listenfd) {
            int connfd = accept(listenfd, xxx, xxx);
            setnonblock(connfd);
            ev.events = EPOLLIN | EPOLLET;
            ev.data.fd = connfd;
            epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);
        } else {
            // (recv, send)之前先DEL再ADD，保证多个上下文不会同时对一个IO进行操作，协程IO异步操作正式采用此模式
            epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, NULL);
            recv(sockfd, buffer, length, 0);
            
            send(sockfd, buffer, length, 0);
            epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, NULL);
        }
    }
}
```



![coroutine_nty](pic\coroutine_nty.png)

- 将sockfd添加到epoll管理中
- 进行上下文环境切换，由协程上下文yield到调度器的上下文
- 调度器获取下一个协程上下文，resume新的协程

![coroutine_time](pic\coroutine_time.png)

##### 回调协程的子过程

CPU有一个非常重要的寄存器叫eip，用来存储CPU运行下一条指令的地址，可以把回调函数的地址存储到eip，将对应参数存储到相应的参数寄存器

```c
void _exec(nty_coroutine *co) {
    co->func(co->arg);  // 子过程回调
}

void nty_coroutine_init(nty_coroutine *co) {
    // ctx 协程上下文
    co->ctx.edi = (void*) co;
    co->ctx.eip = (void*) _exec;  // 设置回调函数入口
    // 当实现上下文切换时，会执行入口函数_exec，_exec调用子过程func
}
```



##### 3. 协程原语

**create创建协程**

create没有exit，协程一旦创建就不能由用户自己销毁，必须以子进程执行结束自动销毁。

```c
int nty_coroutine_create(nty_coroutine **new_co, proc_coroutine func, void *arg) {
    nty_schedule *sched = nty_coroutine_get_sched();
    // 如调度器不存在，则创建；调度器为全局单例，存储在线程的私有空间pthread_setspecific
    if (sched == NULL) {
        nty_schedule_create(0);
        sched = nty_coroutine_get_sched();
    }
    // 分配一个coroutine内存空间，设置其栈空间、栈大小、初始状态、创建时间，子过程回调以及参数
    nty_coroutine *co = calloc(1, sizeof(nty_coroutine));
    co->sched = sched;
    co->stack_size = sched->stack_size;
    co->status = BIT(NTY_COROUTINE_STATUS_NEW);
    co->func = func;
    co->arg = arg;
    *new_co = co;
    // 加入就绪队列
    TAILQ_INSERT_TAIL(&co->sched->ready, co, ready_next);
}
```

**yield让出cpu**

```c
// 调用后该函数不会立即返回，而是切换到最近执行resume的上下文，由调度器统一选择resume的协程
void nty_coroutine_yield(nty_coroutine *co) {
    // co 当前运行协程实例
    co->ops = 0;
    _switch(&co->sched->ctx, &co->ctx);
}
```

**resume恢复协程的运行权**

```c
// 调用该函数后不会立即返回，而是切换到相应协程的yield位置。
int nty_coroutine_resume(nty_coroutine *co) {
    nty_schedule *sched = nty_coroutine_get_sched();
    sched->curr_thread = co;
    _switch(&co->ctx, &co->sched->ctx);  // 切换
    sched->curr_thread = NULL;
}
```

##### 协程切换switch

switch:switch方法使用汇编实现

上下文切换，将cpu的寄存器暂时保存，再将即将运行的协程的上下文寄存器，分别mov到相对应的寄存器，完成切换。

```c
// 1. store: 把寄存器的值保存到原有线程
// 2. load：把新线程的寄存器值加载到cpu中
int _switch(nty_cpu_ctx *new_ctx, nty_cpu_ctx *cur_ctx);
/*
 * new_ctx: 即将运行的协程上下文
 * cur_ctx: 正在运行的协程上下文
 */

typedef struct _nty_cpu_ctx {
    void *esp;
    void *ebp;
    void *eip;
    void *edi;
    void *esi;
    void *ebx;
    void *r1;
    void *r2;
    void *r3;
    void *r4;
    void *r5;
} nty_cpu_ctx;
```



##### 4. 协程实现之定义

**运行体**

包含运行状态（就绪，睡眠，等待），运行体回调函数，回调参数，栈指针，栈大小

**运行体如何高效地在多种状态集合更换**

- 新创建协程后，加入就绪集合，等待调度器调度；
- 协程在运行完成后，进行IO操作，IO未准备好，则进入等待状态集合
- IO准备就绪，协程开始运行，后续sleep操作，进入睡眠状态集合

就绪集合：使用队列

睡眠集合：红黑树

等待集合：红黑树

![coroutine_set](pic\coroutine_set.png)

**调度器**

```c
typedef struct _nty_schedule {
    nty_cpu_ctx ctx;
    int eventfd;
    struct epoll_event eventlist[NTY_CO_MAX_EVENTS];
    int nevents;
  	nty_coroutine_queue ready;
    nty_coroutine_rbtree_sleep sleeping;
    nty_coroutine_rbtree_wait waiting;
} nty_schedule;

// 生产者消费者模型
while (1) {
    // 遍历睡眠集合，将满足条件的加入ready
    nty_coroutine *expired = NULL;
    while ((expired = sleep_tree_expired(sched)) != NULL) {
        TAILQ_ADD(&sched->ready, expired);
    }
    // 遍历等待集合，将满足条件的加入ready
    
    // 使用resume恢复ready的协程运行权
    while (!TAILQ_EMPTY(&sched->ready)) {
        nty_coroutine *ready = TAILQ_POP(sched->ready);
        resume(ready);
    }
}
```







##### 5. 协程多核实现

1. 借助线程 

   - 所有线程共用一个调度器：会出现线程之间互跳，（需要考虑加锁
   - 每个线程一个调度器：

   

2. 借助进程

   - 每个进程一个调度器：将进程绑定到cpu上（cpu粘合），充分利用核心

3. 汇编来实现(也可)



##### 6. 协程性能如何？

比对于epoll+线程池慢一点，那为什么还会选择协程呢？

因为协程编程简单，好维护，并不是因为协程性能会高与epoll，其底层也是用epoll



协程之间的调度必须经过调度器



#### Q&A

Q：io事件的改变用`epoll_ctl(del) + epoll_ctl(ADD)`还是`epoll_ctl(MOD)`

A：正常选择MOD，红黑树集合不会改变，修改value



Q：1个协程也没有的情况？

A：调度器直接退出

Q：协程从哪里开始创建如何运行，入口函数如何进去？

A：coroutine_create() 加入调度器就绪队列













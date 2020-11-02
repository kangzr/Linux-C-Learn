`skynet`是一个轻量级后台服务器框架



### 多核开发解决方案？

- 多进程；

- 多线程
- `csp` go语言(协程`goroutine+channel`)（加强版的多线程解决方案）
- actor模型(加强版的多进程解决方案，通过指针进行消息传递，可以启动上千万个actor)



1. **多进程** 隔离性(运行环境)好，统一性(数据同步)比较差

   - socket
   - 共享内存
   - 管道
   - 信号量
   - `unix`域

   **统一性(数据同步)比较差**的解决方案（服务间socket通信问题：`zeromq, rpc`）

   - 消息队列`zeromq`：推拉模型，请求回应，监听发布模型，解决**最终一致性问题**
     - 协议为你
     - 断线重连
     - 进程启动顺序问题
     - 负载均衡问题：`fd % 4`
     - 数据同步问题
   - `rpc grpc`：**解决强一致性问题**
   - `zookeeper`：服务协调的问题（核心机制：数据模型+**监听机制**）
     - 配置项管理问题
     - 集群管理 （服务断开）（`redis`哨兵模式），选举master，解决高可用问题(一个服务器断开了，不影响业务)
     - 统一命名问题
     - 状态同步问题
     - 分布式锁问题：多个进程竞争有限资源，解决方案：
       - 数据中心的进程（数据中心存储了临界资源）
       - n个竞争资源的进程
       - 进程竞争资源操作：1. 向数据中心请求锁；2. 获取锁，执行相应的逻辑；3. 释放锁（如果在释放锁之前服务断开，解决：过期时间，额外线程 轮询释放锁，其它服务才有能继续使用）**而`zookeeper`解决方式是，可以立马知道服务挂掉，进而释放锁**
       - 数据中心：1. 记录锁，当前使用的对象；2. 主动推送

2. **多线程** 隔离性差(使用锁进行控制，锁类型多，粒度不好控制)，统一性强(数据同步，一个进程内)

   如何解决和平衡隔离性与统一性问题？

   **多线程通信**：

   - 消息队列
   - pipe
   - 锁

   网络模型：reactor，`proactor`

   并发模型：(多进程实体为进程，多线程实体为线程，actor模式实体为actor，go模式实体为`goroutine`)

   - `actor`：`actor`从语言层面抽象出进程的概念，以actor为并发实体,比如`erlang`（进程，语言层面解决），`skynet`（用框架解决actor），
     - 用于并行计算
     - actor是最基本的计算单元
     - 基于消息计算 （skynet回到函数）
     - actor之间相互隔离（skynet 内存块+lua虚拟机），通过消息(mailbox)进行沟通（skynet消息队列）
   - `csp`：go语言为代表，并发实体为`goroutine`



skynet中的actor模型，skynet中的服务即为actor

- 隔离的环境
- 回调函数，消耗消息，执行actor
- 消息队列  存储消息

skynet c+lua，每一个actor都有一块内存块

1. c actor 服务`service_logger.c`：`init, create, release, callback`, 一个内存块
2. lua服务：lua服务启动器`service_snlua.c`，隔离环境就是lua虚拟机，lua虚拟机

hook `jemalloc`中的分配内存函数



#### **actor运行以及消息调度**

全局消息队列：存储有消息的actor消息队列指针（这里需要加锁，每个格子加锁）

actor消息队列：存储专属actor的消息队列

	1. 取出actor消息队列
 	2. 取出消息，通过回调函数（消息）消费actor中的消息
 	3. 如果actor消息队列还有消息，就将actor消息队列放入全局消息队列的队尾

![skynet_msg](..\pic\skynet_msg.png)

**消息的生产和消费**

生产方式：

1. actor之间
2. 网络中产生
3. 定时器

消费：

1. actor回调

- 从消息队列取消息，并通过回调函数调用  自旋锁；worker线程执行逻辑，尽量不要休眠或切换，worker线程逻辑性比较强
- 没有消息时，使用条件变量进行休眠；定时器+网络线程将其唤醒；
- 为什么actor之间发送消息，为何不需要唤醒worker条件变量？因为actor之间发送消息，则至少有一个worker线程在工作



#### skynet网络层

1. 阻塞io与非阻塞io区别？

   - 阻塞什么？ 阻塞网络线程

   - 区别在于没有数据到达时，是否立刻返回；

     例如：调用recv/send，read/write时，网络中没有数据时阻塞IO会一直等待；非阻塞IO会立刻返回

   - 什么决定IO是否阻塞？产生fd时是否设置(fnctl)  NONBOLICK

2. redis skynet采用非阻塞的io



- socket read/write是不是都在一个线程
- epoll_data_t data数据存储时候，用来数据映射 （redis ：data.fd --> connect -->client-->process cmd，skynet使用data.ptr  == socket*）
- 网络事件状态机怎么做
- skynet作为客户端怎么处理



读 都在socket线程

写 spinlock try_spinlock()，拿到锁，（失败：通过管道将数据发送到socket线程，epoll_wait

如果socket线程没有对该io操作，那么直接在worker线程将数据发送





三种fd

1. listenfd
2. 客户端的连接fd
3. skynet作为客户端连接其它服务fd
4. 管道fd

reactor：将连接转化为事件处理，==》 用户处理数据 ==》 到actor










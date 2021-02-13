本文主要介绍skynet的设计理念和特点，对于具体实现细节暂不展开。

#### skynet是什么?

"skynet 是一个为网络游戏服务器设计的轻量框架。但它本身并没有任何为网络游戏业务而特别设计的部分，所以尽可以把它用于其它领域。" 	 -- skynet wiki

#### skynet设计的初衷是啥？

作为服务器，通常需要同时处理多份类似的业务。例如在网络游戏中，你需要同时向数千个用户提供服务；同时运作上百个副本，计算副本中的战斗、让 NPC 通过 AI 工作起来，等等。在单核年代，我们通常在 CPU 上轮流处理这些业务，给用户造成并行的假象。而现代计算机，则可以配置多达数十个核心，如何充分利用它们并行运作数千个相互独立的业务，是设计 skynet 的初衷。 -- skynet wiki

因此，skynet提供了一种多核并发的解决方案，充分利用了多核心优势。

多核并发的解决方案有：多进程，多线程，csp模型，actor模型。接下来简单介绍和对比这四种并发模型。

##### 多进程并发模型

并发实体：进程

进程间通信方式：socket，共享内存，管道，信号量，unix域等。

优点：隔离性好，因为每个进程都有自己独立的进程空间

缺点：统一性差，即数据同步比较麻烦；解决方案（消息队列zeromq解决最终一致性问题，rpc解决强一致性问题，zookeeper解决服务协调的问题）

##### 多线程并发模型

并发实体：线程

线程间通信方式：消息队列，管道，锁等

优点：统一性强，因为线程都在同一个进程内（这里的多线程是指同一进程内的多线程）

缺点：隔离性差，线程间共享了很多资源，并且可以轻易的访问其他线程的私有空间，需要使用锁来进行控制。（锁的类型选择和粒度控制都是比较难的）

##### csp并发模型

描述两个独立的并发实体通过共享的通讯 channel(管道)进行通信的并发模型。

Golang 借用CSP模型仅仅是借用了 process和channel这两个概念来实现自己的并发模型，process是在go语言上的表现就是 goroutine ，也是go并发执行的实体，每个实体之间是通过channel通讯来实现数据共享。（加强版多线程解决方案）

##### actor模型

并发实体当然是actor。那么actor是什么呢？其实actor是从语言层面抽象出来的进程的概念，erlang就是从语言层面来实现actor模型。（加强版多进程解决方案），actor模型有以下特点：

- 用于并行计算
- actor是最基本的计算单元
- 基于消息计算
- actor之间相互隔离，通过消息进行沟通

那么skynet也采用了actor模型，不过，不同于erlang，skynet是通过框架来实现actor模型。skynet使用内存块和lua虚拟机来进行环境隔离，actor之间通过消息队列进行沟通，通过指针的传递即可达到通信目的。

那么actor模型有哪些优势呢？我们可以启动上千万个actor并发实体，而进程/线程模型中并发实体个数是有限的。

#### skynet中actor的隔离与通信

因此，我门首先要理解的actor模型是什么。其实actor就是skynet中的服务，服务分为c服务和lua服务（main.lua就是一个actor），actor的结构组成如下：

- 隔离环境，内存块或lua虚拟机
- 回调函数，用于执行actor，消费消息
- 消息队列，用于存储消息

**隔离**

对于c服务隔离环境为内存块，lua服务隔离环境为lua虚拟机

```c
// service_logger.c 
// c服务隔离环境为内存块
struct logger {
	FILE * handle;
	char * filename;
	uint32_t starttime;
	int close;
};

// service_snlua.c 
// lua服务隔离环境为lua虚拟机
struct snlua {
	lua_State * L;
	struct skynet_context * ctx;
	size_t mem;
	size_t mem_report;
	size_t mem_limit;
	lua_State * activeL;
	volatile int trap;
};

// skynet_server.c
// context上下文隔离环境
struct skynet_context {
	void * instance;
	struct skynet_module * mod;
	void * cb_ud;
	skynet_cb cb;
	struct message_queue *queue;
  ...
};
```

lua一般用来做业务开发（lua服务），c一般实现底层框架以及一些计算密集型的业务（c服务）。**可以将skynet理解为一个简单的操作系统，可以用来调度数千个lua虚拟机（进程），让他们并行工作。**每个lua虚拟机都可以接收其他虚拟机发送过来的消息，以及对其他虚拟机发送消息。

**通信**

skynet中actor的运行和通信都通过消息来驱动：

- 全局消息队列：存储有消息的actor消息队列指针
- actor消息队列：存储专属actor的消息队列

如下图：

![skynet_msg](..\..\pic\skynet_msg.png)

工作流程：

- 从全局消息队列中取出actor消息队列，（这一步需要加锁，采用自旋锁，尽可能不让worker线程休眠，榨干cpu）
- 从actor消息队列中取出消息，并通过回调函数处理（消费actor中的消息）；因此不用担心一个服务同时被多个线程处理，即单个服务的执行，不存在并发，也即线程安全。
- 如果actor消息队列还有消息，将actor消息队列放入全局消息队列的队尾，起到公平调度

消息生产方式主要为：

- actor之间通信产生；
- 网络中产生
- 定时器产生

消息的消费方式只有一种，那就是通过回调函数进行消费。因为actor之间通信直接指针的传递，因此服务间的通信非常高效。

注意：actor之间发送消息是不需要唤醒worker条件变量的，因为actor之间发送消息，则至少有一个worker线程在工作。

skynet每个服务均有一个协程池，lua服务收到消息时，会优先去池子里取一个协程出来，这里为了理解方便，就视为收到一个消息，就创建一个协程吧



#### skynet中的线程

- timer线程：运行定时器
- socket线程，进行网络数据的收发
- worker线程：负责对消息队列进行调度
- monitor线程：用于检测节点内的消息是否堵住

线程间使用管道进行通信；

```c
// skynet_start.c
// skynet启动是会创建以上四种线程
static void
start(int thread) {
	...
	create_thread(&pid[0], thread_monitor, m);  // monitor线程
	create_thread(&pid[1], thread_timer, m);  // timer线程
	create_thread(&pid[2], thread_socket, m);  // socket线程
	// 根据权重创建worker线程
	static int weight[] = { 
		-1, -1, -1, -1, 0, 0, 0, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 
		2, 2, 2, 2, 2, 2, 2, 2, 
		3, 3, 3, 3, 3, 3, 3, 3, };
	struct worker_parm wp[thread];
	for (i=0;i<thread;i++) {
		wp[i].m = m;
		wp[i].id = i;
		if (i < sizeof(weight)/sizeof(weight[0])) {
			wp[i].weight= weight[i];
		} else {
			wp[i].weight = 0;
		}
		create_thread(&pid[i+3], thread_worker, &wp[i]);
	}
}
```



其中socket线程和worker线程通过pipe进行通信。服务模块要将数据，通过socket发送给客户端时，并不是将数据写入消息队列，而是通过pipe从worker线程发送给socket线程，并交由socket转发。



skynet作为游戏服务器时，我们编写的不同的业务逻辑，独立运行在不同的上下文环境，并且能通过某种方式，相互协作，共同服务于玩家。



skynet 业务是由lua来开发，与底层沟通以及计算密集的都需要用c



skynet向epoll进行注册：connected, clients, listened, pipe读端（worker线程往管道写端写数据，socket线程在管道读端读数据）

skynet中内存分配采用jemalloc。



以上，是为一个初学者对skynet的理解。



参考：

[skynet Wiki](https://github.com/cloudwu/skynet/wiki)

[云风BLOG](https://blog.codingnow.com/2012/09/the_design_of_skynet.html)

[skynet源码欣赏](https://manistein.github.io/blog/post/server/skynet/skynet%E6%BA%90%E7%A0%81%E8%B5%8F%E6%9E%90/)

[Golang CSP](https://www.jianshu.com/p/36e246c6153d)
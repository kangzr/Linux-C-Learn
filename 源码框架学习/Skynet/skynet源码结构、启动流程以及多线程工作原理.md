### 一.  Skynet目录结构

<img src="../pic/skynet_cont.png" alt="skynet_cont" style="zoom:50%;" />

- skynet-src: c语言写的skynet核心代码
- service-src: c语言编写的封装给Lua使用的服务，编译后生成的so文件在cservice中（如gate.so, harbor.so, logger.so, snlua.so）
- lualib-src: c语言编写的封装给lua使用的库，编译后生成的so文件在luaclib中（如bson.so, client.so, md5.so, skynet.so, sproto.so,lpeg.so），提供C层级的api调用，如调用socket模块的api，调用skynet消息发送，注册回调函数的api，甚至是对C服务的调用等，并导出lua接口，供lua层使用，可以视为lua调C的媒介。
- lualib: lua编写的库
- service:  lua写服务
- 3rd ：第三方库如jemalloc, lua等
- test：lua写的一些测试代码
- examples: 框架示例

就是上层允许调用下层，而下层不能直接调用上层的api，这样做层次清晰



### 二.  Skynet启动流程

1. 启动skynet方式：终端输入`./skynet examples/config`;

2. 启动入口函数为`skynet_main.c/main`，`examples/config`作为args[1]参数传入

   ```c
   // skynet_main.c/main
   // 主要功能：初始化全局信息，加载配置文件，调用skynet_start.c中的skynet_start函数
   int main(int argc, char *argv[]){
     // 配置文件
     const char * config_file = argv[1]; 
     // 初始化全局信息
     skynet_globalinit(); 
     skynet_env_init();
     // 加载配置文件
     struct skynet_config config;
     struct lua_State *L = luaL_newstate();
     luaL_openlibs(L);  // link lua lib
     luaL_loadbufferx(L, load_config...);
     ...
     lua_close(L);
     skynet_start(&config);
     return 0;
   }
   ```

3. 调用`skynet_start.c/skynet_start`函数

   ```c
   // skynet_start.c/skynet_start
   // 根据配置信息初始化全局信息，创建一个logger服务
   // 调用bootstrap函数创建一个snlua服务
   // 调用start函数创建一个检测线程，一个定时器线程，一个套接字线程以及多个工作线程
   void skynet_start(struct skynet_config * config){
     // 初始化工作
     skynet_harbor_init(config->harbor);
     skynet_handle_init(config->harbor);
     skynet_mq_init();  // 消息队列
     skynet_module_init(config->module_path);  // 模块
     skynet_timer_init();  // 定时器
     skynet_socket_init();  // socket
     struct skynet_context *ctx = skynet_context_new(...);  // 创建context上下文环境
     skynet_handle_namehandle(skynet_context_handle(ctx), "logger");  // logger服务
     bootstrap(ctx, config->bootstrap);  // 创建snlua服务
     start(config->thread); // 创建各种线程
   }
   ```

   

### 三.  Skynet多线程工作原理

skynet线程创建工作由上述`skynet_start.c/skynet_start/start`完成，主要有以下四类线程：

- monitor线程：监测所有的线程是否有卡死在某服务对某条消息的处理
- timer线程：运行定时器
- socket线程，进行网络数据的收发
- worker线程：负责对消息队列进行调度

```c
// skynet_start.c
static void
start(int thread) {
    pthread_t pid[thread+3];  // 另加三个线程：socket，timer，monitor
    // 创建共享monitor
    struct monitor *m = skynet_malloc(sizeof(*m));		
    // 为每个工作线程创建一个存储监测信息的结构体
    m->m = skynet_malloc(thread * sizeof(struct skynet_monitor *));
    for (int i=0;i<thread;i++) {
        m->m[i] = skynet_monitor_new();  // 为每个工作线程分配一块监测信息内存
    }
    
    // 初始化所有线程共享的互斥锁和条件变量
    if (pthread_mutex_init(&m->mutex, NULL)) {
        fprintf(stderr, "Init mutex error");
        exit(1);
    }
    
    if (pthread_cond_init(&m->cond, NULL)) {
        fprintf(stderr, "Init cond error");
        exit(1);
    }
    
    // 创建一个监测线程，一个定时器线程，一个套接字线程
    create_thread(&pid[0], thread_monitor, m);
    create_thread(&pid[1], thread_timer, m);
    create_thread(&pid[2], thread_socket, m);
    
    // 创建对应数量的工作线程
    // 避免过多的worker线程为了等待spinlock解锁，而陷入阻塞状态
    static int weight[] = {
        -1, -1, -1, -1, 0, 0, 0, 0,    // -1表示每个线程每次处理消息数量为1, 0表示处理所有信息
        1, 1, 1, 1, 1, 1, 1, 1,  	   // 1表示每个线程每次处理服务队列中的所有消息的1/2
        2, 2, 2, 2, 2, 2, 2, 2,		   // 2表示每个线程每次处理服务队列中的所有消息的1/4
        3, 3, 3, 3, 3, 3, 3, 3, 	   // 3表示每个线程每次处理服务队列中的所有消息的1/8
    };
    struct worker_parm wp[thread];
    for (i=0;i<thread;i++) {
        wp[i].m = m;  // 共享m
        wp[i].id = i;  // 线程id
        if (i < sizeof(weight)/sizeof(weight[0])) {
            wp[i].weight = weight[i];
        } else {
            wp[i].weight = 0;
        }
        create_thread(&pid[i+3], thread_worker, &wp[i]);  // 创建工作线程
    }
    
    // 等待所有线程结束
    for (i=0;i<thread+3;i++) {
        pthread_join(pid[i], NULL); 
    }
    
    // 释放资源
    free_monitor(m);
};
```

#### monitor线程

- 初始化该线程的key对应的私有数据块
  
- 每5s对所有工作线程进行一次监测
  
- 调用`skynet_monitor_check`函数监测每个线程是否有卡住在某条消息的处理

```c
static void *
  thread_monitor(void *p) {
  for (;;) {
    for (int i=0;i<n;i++) {
      skynet_monitor_check(m->m[i]);
    }
    for (i=0;i<5;i++) {
      sleep(1);
    }
  }
  return NULL;
}
//检测某个工作线程是否卡主在某一条消息
void skynet_monitor_check(struct skynet_monitor *sm) {
  if (sm->version == sm->check_version) {//当前处理的消息和已经监测到的消息是否相等
    if (sm->destination) {            //判断消息是否已经被处理
      skynet_context_endless(sm->destination);    //标记相应的服务陷入死循环
      skynet_error(NULL, "A message from [ :%08x ] to [ :%08x ] maybe in an endless loop (version = %d)", sm->source , sm->destination, sm->version);
    }
  } else {
    sm->check_version = sm->version;
  }
}
```

#### timer定时器线程

每隔2500微妙刷新计时、唤醒等待条件触发的工作线程并检查是否有终端关闭的信号，如果有则打开log文件，将log输出至文件中，在刷新计时中会对每个时刻的链表进行相应的处理

```c
//定时器线程运行函数
static void * thread_timer(void *p) {
  struct monitor * m = p;
  skynet_initthread(THREAD_TIMER);    //初始化该线程对应的私有数据块
  for (;;) {
    skynet_updatetime();            //刷新时间，详情请看定时器部分
    wakeup(m,m->count-1);            //唤醒等待条件触发的工作线程
    usleep(2500);                    //定时器线程挂起2500微秒
  }
  // wakeup socket thread
  skynet_socket_exit();                //正常结束套接字服务
  // wakeup all worker thread
  pthread_mutex_lock(&m->mutex);        //获得互斥锁
  m->quit = 1;                        //设置线程退出标志
  pthread_cond_broadcast(&m->cond);    //激活所有等待条件触发的线程
  pthread_mutex_unlock(&m->mutex);    //释放互斥锁
  return NULL;
}
//唤醒等待条件触发的线程
static void wakeup(struct monitor *m, int busy) {
  if (m->sleep >= m->count - busy) {
    // signal sleep worker, "spurious wakeup" is harmless
    //激活一个等待该条件的线程，存在多个等待线程时按入队顺序激活其中一个；
    pthread_cond_signal(&m->cond);
  }
}
```

#### socket套接字线程

处理所有的套接字上的事件，刚初始化用于处理命令，线程的退出也有命令控制，该线程确保所有的工作线程中至少有一条工作线程是处于运行状态的，确保可以处理套接字上的事件。

```c
//套接字线程运行函数
static void * thread_socket(void *p) {
  struct monitor * m = p;
  skynet_initthread(THREAD_SOCKET);    //初始化该线程对应的私有数据块
  for (;;) {
    //处理所有套接字上的事件，返回处理的结果，将处理的结果及结果信息转发给对应的服务
    int r = skynet_socket_poll(); 
    if (r==0)  //线程退出
      break;
    wakeup(m,0);  //如果所有工作线程都处于等待状态，则唤醒其中一个
  }
  return NULL;
}
```

#### worker工作线程

从全局队列中取出服务队列对其消息进行处理，其运行函数thread_worker的工作原理：首先初始化该线程的key对应的私有数据块，然后从全局队列中取出服务队列对其消息进行处理，最后当全局队列中没有服务队列信息时进入等待状态，等待定时器线程或套接字线程触发条件；

```c
//工作线程运行函数
static void * thread_worker(void *p) {
  struct worker_parm *wp = p;                //当前工作线程的参数
  int id = wp->id;                        //当前工作线程的序号
  int weight = wp->weight;
  struct monitor *m = wp->m;
  struct skynet_monitor *sm = m->m[id];
  skynet_initthread(THREAD_WORKER);        //初始化该线程对应的私有数据块
  struct message_queue * q = NULL;
  while (!m->quit) {
    q = skynet_context_message_dispatch(sm, q, weight);        //消息分发
    if (q == NULL) {//若全局队列中没有队列信息，尝试获得互斥锁，等待定时器线程或套接字线程触发
      //获得互斥锁，如该锁已被其他工作线程锁住或拥有，则该线程阻塞直到可以
      if (pthread_mutex_lock(&m->mutex) == 0) {
        ++ m->sleep;        //线程阻塞计数加1
        // "spurious wakeup" is harmless,
        // because skynet_context_message_dispatch() can be call at any time.
        if (!m->quit)
          pthread_cond_wait(&m->cond, &m->mutex);        //等待条件触发
        -- m->sleep;    //线程阻塞计数减1
        if (pthread_mutex_unlock(&m->mutex)) {        //释放互斥锁
          exit(1);
        }
      }
    }
  }
  return NULL;
}
```

#### 消息处理如何保证线程安全？

- 每条worker线程，从global_mq取出的次级消息队列都是唯一的，有且只有一个服务与之对应，取出之后，在该worker线程完成所有callback调用之前，不会push回global_mq中，也就是说，在这段时间内，其他worker线程不能拿到这个次级消息队列所对应的服务，并调用callback函数，也就是说一个服务不可能同时在多条worker线程内执行callback函数，从而保证了线程安全
- 不论是global_mq还是次级消息队列，在入队和出队操作时，都需加上spinlock，这样多个线程同时访问mq的时候，第一个访问者会进入临界区并锁住，其他线程会阻塞等待，直至该锁解除，这样也保证了线程安全。
- 我们在通过handle从skynet_context list里获取skynet_context的过程中（比如派发消息时，要要先获取skynet_context指针，再调用该服务的callback函数），需要加上一个读写锁，因为在skynet运作的过程中，获取skynet_context，比创建skynet_context的情况要多得多，因此这里用了读写锁

skynet为事件驱动运行，由socket和timeout两个线程驱动

#### 部分数据结构预览

```c
// skynet_handle.c
// 服务句柄handle：服务的唯一标识，对应一个skynet_context（服务隔离），
// 所有的skynet_context存放在handle_storage中
struct handle_storage {
	struct rwlock lock;
    uint32_t harbor;		//集群ID
    uint32_t handle_index;	// 当前句柄索引
    int slot_size;			// 槽位数组大小
    struct skynet_context ** slot;	// skynet_context数组
    ...
};

// skynet_start.c
// 用作定时器、监测、套接字和工作线程的运行函数都共享的参数
struct monitor {
    int count;  // 工作线程数量，读配置
    struct skynet_monitor **m;  // 为每个工作线程存储监测信息的结构体
    pthread_cond_t cond;  // 多线程同步机制中的条件变量
    pthread_mutext_t mutex;  // 多线程同步机制中的互斥锁
    int sleep;  // 记录处于阻塞状态的线程数量
    int quit;	// 标记线程是否退出
};

struct worker_parm {
    struct monitor *m;	// 所有线程共享的参数
    int id;	// 每个工作线程的序号
    int weight;	// 标记每个线程每次处理服务队列中的消息数量
};

// 为每个工作线程存储监测信息的结构体
// skynet_monitor.c
struct skynet_monitor {
    int version;	// 每修改一次source和destination +1
    int check_version;	// 用于和当前的version进行比较，防止线程卡死在某一条消息
    uint32_t source;	// 消息源，定位到发消息的服务
    uint32_t destination;	// 消息发往的目的地
};
```



[参考](https://zhongyiqun.gitbooks.io/skynet/content/12-skynetyuan-ma-mu-lu-jie-gou.html)
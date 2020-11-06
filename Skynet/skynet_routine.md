#### 一. Skynet启动流程

1. 启动skynet：终端输入`./skynet examples/config`；

2. 入口为`skynet_main.c`中`main`函数，`config`作为`argv[1]`参数传入

3. `main`函数主要实现功能：初始化全局信息，加载配置文件，调用`skynet_start.c`中`skynet_start`函数

   ```c
   // skynet_main.c
   int main(int argc, char *argv[]){
       
   }
   ```

4. `skynet_start`主要工作：

   - 根据配置信息初始化全局信息，创建一个logger服务
   - 调用bootstrap函数创建一个snlua服务
   - 调用start函数创建一个检测线程，一个定时器线程，一个套接字线程以及多个工作线程(配置)

   ```c
   // skynet_start.c
   void skynet_start(struct skynet_config * config){
       
   }
   ```

   

#### 二. 多线程工作原理

##### **1. 重要结构体**

```c
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

##### **2. `skynet_start`中start函数主要工作：**

- 分配一个所有线程共享的结构体monitor

- 为每个工作线程创建一个存储监测信息的结构体

- 初始化所有线程共享的互斥锁和条件变量

- 创建一个监测线程，一个定时器线程，一个套接字线程

- 创建对应数量的工作线程

- 等待所有线程结束

  ```c
  // skynet_start
  static void
  start(int thread) {
      pthread_t pid[thread+3];  // 另加三个线程：socket，timer，monitor
      // 创建共享monitor
      struct monitor *m = skynet_malloc(sizeof(*m));		
      memset(m, 0, sizeof(*m));
      m->count = thread;
      m->sleep = 0;
      
      // 为每个工作线程创建一个存储监测信息的结构体
      m->m = skynet_malloc(thread * sizeof(struct skynet_monitor *));
      int i;
      for (i=0;i<thread;i++) {
          m->m[i] = skynet_monitor_new();  // 分配监测信息内存
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
      static int weight[] = {
          -1, -1, -1, -1, 0, 0, 0, 0,    // -1表示每个线程每次处理消息数量为1, 0表示处理所有信息
          1, 1, 1, 1, 1, 1, 1, 1,  	   // 1表示每个线程每次处理服务队列中的所有消息的1/2
          2, 2, 2, 2, 2, 2, 2, 2,		   // 2表示每个线程每次处理服务队列中的所有消息的1/4
          3, 3, 3, 3, 3, 3, 3, 3, 	   // 3表示每个线程每次处理服务队列中的所有消息的1/8
      };
      struct worker_parm wp[thread];
      for (i=0;i<thread;i++) {
          wp[i].m = m;  // 共享m
          wp[i].id = i;
          if (i < sizeof(weight)/sizeof(weight[0])) {
              wp[i].weight = weight[i];
          } else {
              wp[i].weight = 0;
          }
          create_thread(&pid[i+3], thread_worker, &wp[i]);
      }
      
      // 等待所有线程结束
      for (i=0;i<thread+3;i++) {
          pthread_join(pid[i], NULL); 
      }
      
      // 释放资源
      free_monitor(m);
  };
  ```

  ##### 3. 四类线程运行函数以及相应功能

  **monitor监测线程**

  主要工作：监测所有的线程是否有卡死在某服务对某条消息的处理；

  - 初始化该线程的key对应的私有数据块

  - 每5s对所有工作线程进行一次监测

  - 调用`skynet_monitor_check`函数监测每个线程是否有卡住在某条消息的处理

    ```c
    // skynet_start.c
    static void *
    thread_monitor(void *p) {
        struct monitor * m = p;
        int i;
        int n = m->count;
        skynet_initthread(THREAD_MONITOR);
        for (;;) {
            for (i=0;i<n;i++) {
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
        if (sm->version == sm->check_version) {        //当前处理的消息和已经监测到的消息是否相等
            if (sm->destination) {            //判断消息是否已经被处理
                skynet_context_endless(sm->destination);    //标记相应的服务陷入死循环
                skynet_error(NULL, "A message from [ :%08x ] to [ :%08x ] maybe in an endless loop (version = %d)", sm->source , sm->destination, sm->version);
            }
        } else {
            sm->check_version = sm->version;
        }
    }
    ```

  **timer定时器线程**

  主要工作：每隔2500微妙刷新计时、唤醒等待条件触发的工作线程并检查是否有终端关闭的信号，如果有则打开log文件，将log输出至文件中，在刷新计时中会对每个时刻的链表进行相应的处理

  ```c
  //定时器线程运行函数
  static void * thread_timer(void *p) {
      struct monitor * m = p;
      skynet_initthread(THREAD_TIMER);    //初始化该线程对应的私有数据块
      for (;;) {
          skynet_updatetime();            //刷新时间，详情请看定时器部分
          CHECK_ABORT                        //检测总的服务数量，为0则break
          wakeup(m,m->count-1);            //唤醒等待条件触发的工作线程
          usleep(2500);                    //定时器线程挂起2500微秒
          if (SIG) {                        //如果触发终端关闭的信号SIGHUP，则打开log文件
              signal_hup();                //发送服务内部消息打开log文件，将log输出到文件
              SIG = 0;
          }
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
          pthread_cond_signal(&m->cond);    //激活一个等待该条件的线程，存在多个等待线程时按入队顺序激活其中一个；
      }
  }
  
  //发送服务内部消息打开log文件，将log输出到文件
  static void signal_hup() {
      // make log file reopen
  
      struct skynet_message smsg;
      smsg.source = 0;
      smsg.session = 0;
      smsg.data = NULL;
      smsg.sz = (size_t)PTYPE_SYSTEM << MESSAGE_TYPE_SHIFT;
      uint32_t logger = skynet_handle_findname("logger");        //查找logger服务信息的handle
      if (logger) {
          skynet_context_push(logger, &smsg);        //将消息添加到对应的服务队列
      }
  }
  ```

  **socket套接字线程**

  主要工作：处理所有的套接字上的事件，刚初始化用于处理命令，线程的推出也有命令控制，并且该线程确保所有的工作线程中至少有一条工作线程是处于运行状态的，确保可以处理套接字上的事件。

  ```c
  //套接字线程运行函数
  static void * thread_socket(void *p) {
      struct monitor * m = p;
      skynet_initthread(THREAD_SOCKET);    //初始化该线程对应的私有数据块
      for (;;) {
          int r = skynet_socket_poll();    //处理所有套接字上的事件，返回处理的结果，将处理的结果及结果信息转发给对应的服务
          if (r==0)                        //线程退出
              break;
          if (r<0) {
              CHECK_ABORT        //检测总的服务数量，为0则break
              continue;
          }
          wakeup(m,0);        //如果所有工作线程都处于等待状态，则唤醒其中一个
      }
      return NULL;
  }
  ```

  **worker工作线程**

  主要工作：从全局队列中取出服务队列对其消息进行处理，其运行函数thread_worker的工作原理为，首先初始化该线程的key对应的私有数据块，然后从全局队列中取出服务队列对其消息进行处理，最后当全局队列中没有服务队列信息时进入等待状态，等待定时器线程或套接字线程触发条件；

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
          if (q == NULL) {    //如果全局队列中没有服务队列信息，尝试获得互斥锁，等待定时器线程或套接字线程触发条件
              if (pthread_mutex_lock(&m->mutex) == 0) {    //获得互斥锁，如该锁已被其他工作线程锁住或拥有，则该线程阻塞直到可以
                  ++ m->sleep;        //线程阻塞计数加1
                  // "spurious wakeup" is harmless,
                  // because skynet_context_message_dispatch() can be call at any time.
                  if (!m->quit)
                      pthread_cond_wait(&m->cond, &m->mutex);        //等待条件触发
                  -- m->sleep;    //线程阻塞计数减1
                  if (pthread_mutex_unlock(&m->mutex)) {        //释放互斥锁
                      fprintf(stderr, "unlock mutex error");
                      exit(1);
                  }
              }
          }
      }
      return NULL;
  }
  ```







skynet为事件驱动运行，由socket和timeout两个线程驱动

- 服务句柄handle：服务的唯一标识，对应一个skynet_context，所有的skynet_context存放在handle_storage中

```c
// skynet_handle.c
struct handle_storage {
	struct rwlock lock;
    uint32_t harbor;		//集群ID
    uint32_t handle_index;	// 当前句柄索引
    int slot_size;			// 槽位数组大小
    struct skynet_context ** slot;	// skynet_context数组
    ...
};
```

- 服务模块skynet_context: 一个服务对应一个skynet_context，可理解为一个隔离环境(类似进程)
- 消息队列message_queue：消息队列链表，一个服务对应一个消息队列，所有的消息队列链接起来构成全局消息队列





##### 服务间消息通信

每一个服务都有一个独立的lua虚拟机(skynet_context)，逻辑上服务之间相互隔离，在skynet中服务之间可以通过skynet的消息调度机制来完成通信。

skynet中的服务基于actor模型设计出来，每个服务都可以接收消息、处理消息、发送应答消息

每条skynet消息由6部分构成：消息类型，session，发起服务地址，接收服务地址，消息c指针，消息长度

session能保证本服务中发出的消息是唯一的。消息于响应一一对应
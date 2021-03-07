## 线程池解决了什么问题？

首先我们需要了解为什么要设计线程池，其到底解决了什么问题。

**线程创建和销毁的开销是很大的**，需要为其分配内存，将其加入调度队列由操作系统进行调度。而线程池的目的就是**减少线程的频繁创建和销毁**，维持一定合理数量的线程，**“需要时取，用完时还”**。（连接池的目的也类似，其维持一定数量连接的缓存池，尽量重用已有的连接，减少创建和关闭连接的频率；）线程池和连接池在一定程度上缓解了**频繁调用IO接口带来的资源占用**。



## 线程池设计的基本思想

清楚了线程池设计的目的，接下来思考如何设计一个线程池。

线程池的**基本思想**为：**生产者-消费者模型**。使用两个链表分别表示**生产者**（待处理的工作任务`Jobs`）和**消费者**（包括所有线程`Threads`），并通过一些同步原语来协调二者之间的工作。如下图：

<img src="https://gitee.com/everydaycodingrun/pic/raw/master/OS/thread_pool.png" alt="thread_pool" style="zoom:75%;" />

线程池初始化时，会创建一定数量的线程并放入`Threads`链表中，每个线程处理函数开启一个死循环，通过**条件变量等待信号的到来**；当有新的任务到来时，会加入`Jobs`中，并同时**通过信号唤醒线程**处理相应任务。这就是一个简单的线程池设计思路。



## 线程池的实现

### 链表操作

因为涉及链表的使用，首先使用宏定义实现链表中节点的添加和删除，如下：

```c
// 链表操作
#define LL_ADD(item, list) do {			\
	item->prev = NULL:				   \
	item->next = list;				   \
	list = item;					  \
} while(0)

#define LL_REMOVE(item, list) do {						\
	if(item->next) item->next->prev = item->prev;		 \
	if(item->prev) item->prev->next = item->next;		 \
	if(item == list) list = item->next;					\
	item->next = item->prev = NULL;						\
}while(0)


```

### 数据结构

接下来需要实现链表中线程和工作任务的节点数据结构，以及管理所有线程、工作任务和同步原语的线程池数据结构，如下：

```c
// 线程
struct thread_data {
  	pthread_t thread;
    int terminate;
    struct thread_pool *pool;
    struct thread_data *next;
    struct thread_data *prev;
};

// 工作任务
struct job_data {
    void* user_data;
    void (*func)(struct job_data*);
    struct job_data *next;
    struct job_data *prev;
};

// 线程池
struct thread_pool {
  	struct thread_data *threads;
    struct job_data *jobs;
    pthread_mutex_t mutex;  /*互斥量*/
    pthread_cond_t cond;  /*信号量*/
};
```

### 线程的初始化

线程池初始化的流程，可以指定线程池中线程的数量，如下：

```c
// 创建线程池
int create_thread_pool(struct thread_pool *pool, int num_threads) {
	if (num_threads < 1) num_threads = 1;
    memset(pool, 0, sizeof(struct thread_pool));
    // 初始化锁+信号量
    pthread_mutex_t blank_mutex = PTHREAD_MUTEX_INITIALIZER;
    memcpy(&pool->mutex, &blank_mutex, sizeof(pthread_mutex_t));
    pthread_cond_t blank_cond = PTHREAD_COND_INITIALIZER;
    memcpy(&pool->cond, &blank_cond, sizeof(pthread_cond_t));
    // 创建工作线程
    int i;
    for(i = 0;i < num_threads;i ++){
        struct thread_data *td = (struct thread_data*)malloc(sizeof(*td));
        if(td == NULL) {
            perror("thread_data");
            exit(1);
        }
        td->pool = pool;
        int ret = pthread_create(&pool->thread, NULL, thread_func, (void*)td);
        if (ret) {
            perror("pthread_create");
            free(td);
            exit(1);
        }
        LL_ADD(td, pool->threads);
    }
    return 0;
}
```

实现每个线程创建后的回调函数`thread_func`，通过互斥量和条件变量来等待和处理工作任务，注意，在拿到对应的`Job`后，需要将其从`Jobs`链表中`REMOVE`，避免多个线程处理同一个任务，如下：

```c
// 线程创建后回调接口
static void * thread_func(void *ptr) {
    struct thread_data *td = (struct thread_data*)ptr;
    while (1) {
        pthread_mutex_lock(&td->pool->mutex);
        while(&td->pool->jobs == NULL) { // 没有任务
            if(td->terminate) break;  // 线程终止
            pthread_cond_wait(&td->pool->cond, &td->pool->mutex);
        }
        if(td->terminate){
            pthrad_mutex_unlock(&td->pool->mutex);
            exit(1);
        }
        struct job_data* job = td->pool->jobs;
        if(job != NULL) {
            LL_REMOVE(job, td->pool->jobs);
        }
        pthread_mutex_unlock(&td->pool->mutex);
        if(job == NULL) {
         	continue;
        }
        job->func(job);
    }
    free(td);
    pthread_exit(NULL);
}
```

### 任务的产生

介绍完线程池消费模型后，来介绍下任务的生产方式：1）需要获得互斥锁，2）将任务添加至`Jobs`链表中，3）通过`pthread_cond_signal`唤醒线程，4）释放互斥锁，一气呵成。

```c
// 生成任务
int push_job(struct thread_pool *pool, struct job_data *job) {
   	pthread_mutex_lock(&pool->mutex);
    LL_ADD(job, pool->jobs);
    pthread_cond_signal(&pool->cond);
    pthread_mutex_unlock(&pool->mutex);
    return 0;
}
```

### 使用实例

```c
void counter(job_data *job) {
  	int i = *(int*)job->user_data;
  	printf("counter: %d, threadid: %lu\n", i, pthread_self());
  	free(job->user_data);
  	free(job);
}

int main(int argc, char * argv[]) {
		thread_pool pool;
  	create_thread_pool(&pool, 4);
  	int i;
  	for(i = 0;i < 30; i++) {
				job_data * job = (job_data *)malloc(sizeof(job_data));
      	if(job == NULL){
          	perror("malloc");
          	exit(1);
        }
      	job->func = counter;
      	job->user_data = malloc(sizeof(int));
      	*(int*)job->user_data = i;
      	push_job(&pool, job);
    }
  	getchar();
  printf("\n");
  return 0;
}
```



## Nginx线程池实现

介绍完一个简单的线程池实现，接下来学习学习Nginx中线程池是如何做的，其实大体思路跟上述实现差不多，只不过多了更多的细节考虑，这部分主要也是代码展示，主要代码在文件`ngx_thread_pool.c`中。

### 线程池结构体

```c
// 线程池结构体
struct ngx_thread_pool_s {
  	// 互斥量 锁定操作waiting/queue/ngx_thread_poll_task_id
    ngx_thread_mutex_t		mtx;
    // 待处理任务队列 ngx_thread_task_post(任务放入线程池)；ngx_thread_pool_cycle(消费任务)
    ngx_thread_pool_queue_t		queue;
    // 等待的任务数
    ngx_int_t	waiting;
    // 条件变量，用于等待任务队列queue
    ngx_thread_cond_t	cond;
    // 线程池名字
    ngx_str_t	name;
    // 线程的数量，默认32个
    ngx_uint_t	threads;
    // 任务等待队列，默认65535
    ngx_int_t	max_queue;
   	...
};

typedef struct ngx_thread_pool_s ngx_thread_pool_t;
```

### 线程池初始化

```c
// 线程池初始化
static ngx_int_t
ngx_thread_pool_init(ngx_thread_pool_t *tp, ngx_log_t *log, ngx_pool_t *pool){
    pthread_t tid;
    // 初始化线程池任务队列，first/last都空
    ngx_thread_pool_queue_init(&tp->queue);
    // 创建互斥量
    ngx_thread_mutex_create(&tp->mtx, log);
    // 创建条件变量
    ngx_thread_cond_create(&tp->cond, log);
    // 根据配置，创建线程
    for (n = 0; n < tp->threas; n++){
        pthread_create(&tid, &attr, ngx_thread_pool_cycle, tp);
    }
}
```

### 线程运行函数

线程执行函数同样为一个死循环。

```c
// 线程运行函数，无限循环；从待处理任务队列里获取任务，然后执行task->handler(task->ctx)
// 处理完的任务加入完成队列
static void *
ngx_thread_pool_cycle(void *data) {
    ngx_thread_pool_t *tp = data;
    // 无限循环
    // 从待处理任务队列里获取任务，然后执行task->handler(task->ctx)
    for ( ;; ) {
        // 锁定互斥量，防止多线程操作的竞态
        ngx_thread_mutex_lock(&tp->ntx, tp->log);
        // 即将处理一个任务，计数器-1
        tp->waiting--;
        // 如果任务队列是空，那么使用条件变量等待
        while (tp->queue.first == NULL) {
            ngx_thread_cond_wait(&tp->cond, &tp->mtx, tp->log);
            // (void) ngx_thread_mutex_unlock(&tp->mtx, tp->log);
        }
        // 取任务
        task = tp->queue.first;
        tp->queue.first = task->next;
        // 如果此时任务队列空，调整指针
        if (tp->queue.first == NULL) {
            tp->queue.last = &tp->queue.first;
        }
        // 操作完waiting，queue后解锁，其它线程可以获取task处理
        ngx_thread_mutex_unlock(&tp->mtx, tp->log);
        // 调用任务的handler，传递ctx，执行用户定义操作，同时阻塞的
        task->handler(task->ctx, tp->log);
        task->next = NULL;
        // 自旋锁保护完成队列
        ngx_spinlock(&ngx_thrad_pool_done_lock, 1, 2048);
        // 处理完的任务加入队列
        *ngx_thread_pool_done.last = task;
        ngx_thread_pool_done.last = &task->next;
        // 自旋锁解锁
        ngx_unlock(&ngx_thread_pool_done_lock);
        // 使用event模块的通知函数
        // 让主线程nginx的epoll触发事件，调用ngx_thread_pool_handler，分发处理线程完成的任务
        // 调用系统函数eventfd，创建一个可以用于通知的描述符，用于实现notify
        (void) ngx_notify(ngx_thread_pool_handler);
    }
}

// 分发处理线程完成的任务，在主线程里执行
// 调用event->handler，即异步事件完成后的回调函数
static void
ngx_thread_pool_handler(ngx_event_t *ev) {
    ngx_event_t *event;
    ngx_thread_task_t *task;
    // 自旋锁保护完成队列
    ngx_spinlock(&ngx_thread_pool_done_lock, 1, 2048);
    // 取出队列里的task，task->next有很多已经完成的任务
    task = ngx_thread_pool_done.first;
    // 队列置空
    ngx_thread_pool_done.first = NULL;
    ngx_thread_pool_done.last = &ngx_thread_pool_done.first;
    //自旋锁解锁
    ngx_unlock(&ngx_thread_pool_done_lock);
    // 遍历完成已经完成的任务
    while (task) {
        // 取task里的事件对象
        event = &task->event;
        task = task->next;
        event->complete = 1;  // 线程异步事件已经完成
        event->active = 0;  // 事件处理完成
        even->handler(event);  // 调用异步事件完成后的回调函数
    }
}
```

### 任务生产

```cc

// 任务推送
// 把任务放入线程池，由线程池分配线程执行
// 锁定互斥量，防止多线程的竞态
ngx_int_t
ngx_thread_task_post(ngx_thread_pool_t *tp, ngx_thread_task_t *task) {
    ngx_thread_mutex_lock(&tp->mtx, tp->log);
    if (tp->waiting >= tp->max_queue){
        // 等待处理任务大于设置的最大队列数
        return;
    }
    // 条件变量，发送信号，在ngx_thread_pool_cycle里解除对队列的等待
    ngx_thread_cond_signal(&tp->cond, tp->log);
    // 任务加入待处理队列
    *tp->queue.last = task;
    tp->queue.last = &task->next;
    // 等待任务增加
    tp->waiting++;
    // 解锁
    (void) ngx_thread_mutex_unlock(&tp->mtx, tp->log);
}
```






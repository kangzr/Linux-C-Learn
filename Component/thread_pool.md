#### 线程池的实现

**基本思想：**

线程池中使用两个链表分别管理创建好的工作线程和需要处理的工作任务，如果线程池中暂时没有工作任务，线程会进入条件变量的等待挂起之状态，当有任务生成时，会通过条件变量唤醒工作线程进行处理。



![thread_pool](..\pic\thread_pool.png)

代码如下：

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
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

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

// 创建线程池
int create_thread_pool(struct thread_pool *pool, int num_threads) {
	if (num_threads < 1) num_threads = 1;
    memset(pool, 0, sizeof(struct thread_pool));
    // 初始化锁+信号量
    pthread_mutex_t blank_mutex = PTHREAD_MUTEX_INITIALIZER;
    memcpy(&pool, &blank_mutex, sizeof(pthread_mutex_t));
    pthread_cond_t blank_cond = PTHREAD_COND_INITIALIZER;
    memcpy(&pool, &blank_cond, sizeof(pthread_cond_t));
    
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
            exit(1);
        }
        LL_ADD(td, &pool->threads);
    }
    return 0;
}

// 生成任务
int push_job(struct thread_pool *pool, struct job_data *job) {
   	pthread_mutex_lock(&pool->mutex);
    LL_ADD(job, &pool->jobs);
    pthread_cond_signal(&pool->cond);
    pthread_mutex_unlock(&pool->mutex);
    return 0;
}

// 终止所有线程
int terminate_threads(thread_pool *pool) {
	struct thread_data *td = NULL;
    for(t = pool->threads; t != NULL; t = t->next) {
        t->terminate = 1;
    }
    pthread_mutex_lock(&pool->jobs);
    pool->jobs = NULL;
    pool->threads = NULL;
    pthread_cond_broadcast(&pool->cond);
    pthread_mutex_unlock(&pool->mutex);
    return 0;
}
```












































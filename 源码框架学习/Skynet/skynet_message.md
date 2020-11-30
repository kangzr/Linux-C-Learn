### Skynet消息队列

消息队列分为全局队列和服务队列，每个服务都有一个相应的服务队列用于存放于该服务相关的消息，而每个服务队列都会被全局队列所引用，每个工作线程通过从全局队列中取出相应的服务队列进行消息处理

**消息结构体**

```c
//每个服务队列的结构体
struct message_queue {
    struct spinlock lock;            //锁
    uint32_t handle;                //服务handle，用于定位服务，高8位为节点的编号
    int cap;                        //服务队列中消息队列的容量
    int head;                        //服务队列中消息队列的头
    int tail;                        //服务队列中消息队列的尾
    int release;                    //标记是否释放
    int in_global;                    //标记该服务是否在全局队列中
    int overload;                    //记录服务队列中消息超过阈值时的数量
    int overload_threshold;            //服务队列中消息的上限值，超过将会翻倍
    struct skynet_message *queue;    //消息队列
    struct message_queue *next;        //指向下一个服务
};

//全局队列的结构体
struct global_queue {
    struct message_queue *head;        //全局队列头
    struct message_queue *tail;        //全局队列尾
    struct spinlock lock;            //锁
};
```

**全局队列的初始化**

`skynet_start.c`中的`skynet_start`函数调用`skynet_mq.c`的`skynet_mq_init`函数进行初始化

```c
//skynet_mq.c  初始化全局队列
static struct global_queue *Q = NULL;
void skynet_mq_init() {
    struct global_queue *q = skynet_malloc(sizeof(*q));        //为全局队列分配内存
    memset(q,0,sizeof(*q));    //初始化为0
    SPIN_INIT(q);            //初始化锁
    Q=q;
}
```

**创建服务队列**

`skynet_context_new`函数中首先调用了`skynet_mq_create`创建一个服务队列，然后通过`skynet_globalmq_push`将创建的服务队列关联到全局队列中

```c
// skynet_mq.c
//创建服务队列
struct message_queue * skynet_mq_create(uint32_t handle) {
    struct message_queue *q = skynet_malloc(sizeof(*q));
    q->handle = handle;        //通过此变量来定位服务，相当于服务的地址，高8位为节点的编号
    q->cap = DEFAULT_QUEUE_SIZE;    //默认服务队列容量大小为64
    q->head = 0;                    //初始化队列的头
    q->tail = 0;                    //初始化队列的尾
    SPIN_INIT(q)
    // When the queue is create (always between service create and service init) ,
    // set in_global flag to avoid push it to global queue .
    // If the service init success, skynet_context_new will call skynet_mq_push to push it to global queue.
    q->in_global = MQ_IN_GLOBAL;    //标记是否在全服队列中，默认为在
    q->release = 0;                    //标记是否是否服务队列
    q->overload = 0;                //记录服务队列中消息超过阈值时的数量
    q->overload_threshold = MQ_OVERLOAD;    //服务队列中加载消息数量的阈值
    q->queue = skynet_malloc(sizeof(struct skynet_message) * q->cap);
    q->next = NULL;

    return q;
}

//将服务队列添加到全局队列
void skynet_globalmq_push(struct message_queue * queue) {
    struct global_queue *q= Q;

    SPIN_LOCK(q)
    assert(queue->next == NULL);
    if(q->tail) {
        q->tail->next = queue;
        q->tail = queue;
    } else {
        q->head = q->tail = queue;
    }
    SPIN_UNLOCK(q)
}
```

**向服务队列中添加消息**

在`skynet_mq_create`函数中初始化时服务队列能存储的消息的最大容量为DEFAULT_QUEUE_SIZE，每次向服务队列中添加消息时都会检测是否超出最大容量，如果超出则会进行扩容，将最大容量扩大为原来的2倍，将原来的队列数据拷贝到新的队列中，每次向服务队列中添加消息时如果服务队列不在全局队列中则会将该服务队列添加到全局队列中。

```c
// skynet_mq.c
//扩充服务队列的容量
static void expand_queue(struct message_queue *q) {
    struct skynet_message *new_queue = skynet_malloc(sizeof(struct skynet_message) * q->cap * 2);
    int i;
    for (i=0;i<q->cap;i++) {
        new_queue[i] = q->queue[(q->head + i) % q->cap];
    }
    q->head = 0;
    q->tail = q->cap;
    q->cap *= 2;

    skynet_free(q->queue);
    q->queue = new_queue;
}

//将消息添加到服务队列
void skynet_mq_push(struct message_queue *q, struct skynet_message *message) {
    assert(message);
    SPIN_LOCK(q)

    q->queue[q->tail] = *message;
    if (++ q->tail >= q->cap) {
        q->tail = 0;
    }

    if (q->head == q->tail) {
        expand_queue(q);    //扩充消息队列的容量
    }

    if (q->in_global == 0) {    //如果服务队列不在全局队列中，则将其添加到全局队列
        q->in_global = MQ_IN_GLOBAL;    //标记为在全局队列中
        skynet_globalmq_push(q);    //将服务队列添加到全局队列中
    }

    SPIN_UNLOCK(q)
}
```

**从服务队列中取消息**

每次从服务队列中取消息时都会检测当前服务队列中的消息数量是否超过设定的警戒阈值，只是做通知用，说明该服务的消息负载情况。

```c
//从服务队列中取出消息，取出返回0，否则为1
int skynet_mq_pop(struct message_queue *q, struct skynet_message *message) {
    int ret = 1;
    SPIN_LOCK(q)

    if (q->head != q->tail) {
        *message = q->queue[q->head++];
        ret = 0;
        int head = q->head;
        int tail = q->tail;
        int cap = q->cap;

        if (head >= cap) {
            q->head = head = 0;
        }
        int length = tail - head;    //记录服务队列中消息的数量
        if (length < 0) {
            length += cap;
        }
        while (length > q->overload_threshold) {    //消息数量超过阈值
            q->overload = length;        //记录超过阈值时的消息数量
            q->overload_threshold *= 2;        //阈值翻倍
        }
    } else {
        // reset overload_threshold when queue is empty
        q->overload_threshold = MQ_OVERLOAD;
    }

    if (ret) {            //服务队列中没有消息时，则将该服务队列从全局队列中踢出，设置标志位
        q->in_global = 0;
    }

    SPIN_UNLOCK(q)

    return ret;
}
```

**从全局队列中取消息队列**

```c
//从全局队列中取出服务并删除
struct message_queue * skynet_globalmq_pop() {
    struct global_queue *q = Q;

    SPIN_LOCK(q)
    struct message_queue *mq = q->head;
    if(mq) {
        q->head = mq->next;
        if(q->head == NULL) {
            assert(mq == q->tail);
            q->tail = NULL;
        }
        mq->next = NULL;
    }
    SPIN_UNLOCK(q)

    return mq;
}
```



**Skynet消息处理**

`skynet_context_message_dispatch`函数进行消息分发

- 如果第二个参数为NULL，则从全局队列中取服务队列消息，通过服务队列消息获得定位服务得编号和服务信息
- 默认线程只处理服务队列中的一条消息，通过每个工作线程的weight可以改变每次处理的消息的数量，
- 从服务队列中取出消息如果该服务有回调函数则调用回调函数进行处理（在dispatch_message函数中）
- 如果服务队列中的消息都处理完了，则该服务队列直到有消息时才添加到全局队列中，此时返回全局队列中的下一个服务队列，
- 当服务队列中还有消息，但本次处理的消息数量已完成，则将该服务队列压入全局队列，返回全局队列中的下一个服务队列

```c
//消息分发
struct message_queue * skynet_context_message_dispatch(struct skynet_monitor *sm, struct message_queue *q, int weight) {
    //如果服务队列q为空，则会从全局队列中去取
    if (q == NULL) {
        q = skynet_globalmq_pop();        //从全局队列中取出服务队列，并删除全局队列中的
        if (q==NULL)
            return NULL;
    }

    uint32_t handle = skynet_mq_handle(q);        //获取服务的handle

    struct skynet_context * ctx = skynet_handle_grab(handle);    //根据服务编号（包含节点号和服务号），获得服务信息，会增加服务信息的引用计数
    if (ctx == NULL) {            //如果服务信息为NULL，则释放服务队列信息
        struct drop_t d = { handle };
        skynet_mq_release(q, drop_message, &d);
        return skynet_globalmq_pop();    //返回全局队列中的下一个服务队列
    }

    int i,n=1;
    struct skynet_message msg;

    for (i=0;i<n;i++) {
        if (skynet_mq_pop(q,&msg)) {    //从服务队列中取出消息
            skynet_context_release(ctx);    //递减服务信息的引用计数，如果计数为0则释放
            return skynet_globalmq_pop();    //本服务队列暂无消息，不会返回全局队列，返回下一个服务队列
        } else if (i==0 && weight >= 0) {
            n = skynet_mq_length(q);    //获得消息的数量
            n >>= weight;                //根据weight来决定线程本次处理服务队列中消息的数量
        }
        int overload = skynet_mq_overload(q);    //获得消息数量超出阈值时的消息数量，并清零记录值
        if (overload) {
            skynet_error(ctx, "May overload, message queue length = %d", overload);        //将错误信息输出到logger服务
        }

        skynet_monitor_trigger(sm, msg.source , handle);    //记录消息源、目的地、version增1，用于监测线程监测该线程是否卡死与某条消息的处理

        if (ctx->cb == NULL) {        //如果服务没有注册回调函数则释放掉消息内容
            skynet_free(msg.data);
        } else {
            dispatch_message(ctx, &msg);    //有回调函数调用相应的回调函数进行处理
        }

        skynet_monitor_trigger(sm, 0,0);        //清除记录的消息源、目的地
    }

    assert(q == ctx->queue);
    struct message_queue *nq = skynet_globalmq_pop();    //取出下一个服务队列
    if (nq) {
        // If global mq is not empty , push q back, and return next queue (nq)
        // Else (global mq is empty or block, don't push q back, and return q again (for next dispatch)
        skynet_globalmq_push(q);    //将当前处理的服务队列压入全局队列
        q = nq;
    } 
    skynet_context_release(ctx);    //递减服务信息的引用计数，如果计数为0则释放

    return q;
}

//对分发的消息进行处理
static void dispatch_message(struct skynet_context *ctx, struct skynet_message *msg) {
    assert(ctx->init);
    CHECKCALLING_BEGIN(ctx)
    pthread_setspecific(G_NODE.handle_key, (void *)(uintptr_t)(ctx->handle));    //将handle与线程关联
    int type = msg->sz >> MESSAGE_TYPE_SHIFT;        //获得消息类型
    size_t sz = msg->sz & MESSAGE_TYPE_MASK;        //获得消息的大小
    if (ctx->logfile) {            //如果打开了日志文件，则将消息输出到日志文件
        skynet_log_output(ctx->logfile, msg->source, type, msg->session, msg->data, sz);
    }
    ++ctx->message_count;    //记录处理消息的数量
    int reserve_msg;
    if (ctx->profile) {        //记录消耗CPU时间
        ctx->cpu_start = skynet_thread_time();
        reserve_msg = ctx->cb(ctx, ctx->cb_ud, type, msg->session, msg->source, msg->data, sz);        //调用服务回调进行消息处理
        uint64_t cost_time = skynet_thread_time() - ctx->cpu_start;
        ctx->cpu_cost += cost_time;
    } else {
        reserve_msg = ctx->cb(ctx, ctx->cb_ud, type, msg->session, msg->source, msg->data, sz);        //调用服务回调进行消息处理
    }
    if (!reserve_msg) {
        skynet_free(msg->data);
    }
    CHECKCALLING_END(ctx)
}

//根据服务编号（包含节点号和服务号），获得服务信息
struct skynet_context * skynet_handle_grab(uint32_t handle) {
    struct handle_storage *s = H;        //全局所有的服务信息
    struct skynet_context * result = NULL;

    rwlock_rlock(&s->lock);

    uint32_t hash = handle & (s->slot_size-1);        //获得服务编号对应的服务信息的存储位置
    struct skynet_context * ctx = s->slot[hash];    //获得服务信息
    if (ctx && skynet_context_handle(ctx) == handle) {
        result = ctx;
        skynet_context_grab(result);    //增加服务信息的引用计数
    }

    rwlock_runlock(&s->lock);

    return result;
}
```


































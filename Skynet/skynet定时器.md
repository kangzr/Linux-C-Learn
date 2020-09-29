##### 设计思想

采用**时间轮**方式；

定时器结构分为两部分：1. 即将到来的定时器[0, 2^8-1]；2. 较为遥远的定时器，分四层结构，偏移位为6, Li : [2^(8+6x(i-1)), 2^(8+6xi)-1]

即L1 : [2^8, 2^(8+6) -1], L2 : [2^(8+6), 2^(8+6x2)-1], L3 : [2^(8+6x2), 2^(8+6x3)-1]...结构如下

```c
#define TIME_NEAR_SHIFT 8
#define TIME_NEAR (1 << TIME_NEAR_SHIFT)
#define TIME_LEVEL_SHIFT 6
#define TIME_LEVEL (1 << TIME_LEVEL_SHIFT)
#define TIME_NEAR_MASK (TIME_NEAR-1)
#define TIME_LEVEL_MASK (TIME_LEVEL-1)

struct timer_node {
	struct timer_node *next;
    uint32_t expire;	//到期滴答数
};
    
struct link_list {	// 定时器链表
	struct timer_node head;
    struct timer_node *tail;
};

struct timer {
	struct link_list near[TIME_NEAR];  // 即将到来的定时器
    struct link_list t[4][TIME_LEVEL]; // 相对较遥远的定时器
    struct spinlock lock;
    uint32_t time;
    uint64_t starttime;
    uint64_t current;
    uint64_t current_point;
};
```

##### 定时器初始化

```c
// skynet_start.c
// skynet 启动入口
void
skynet_start(struct skynet_config * config) {
    ...
    skynet_timer_init();
    ...
}
// skynet_timer.c
void
skynet_timer_init(void) {
    // 创建全局TI
    TI  = timer_create_timer();
    uint32_t current = 0;
    systime(&TI->starttime, &current);
    TI->current = current;
    TI->current_point = gettime();
}

static struct timer *
timer_create_timer() {
    struct timer *r = (struct timer *)skynet_malloc(sizeof(struct timer));
    memset(r, 0, sizeof(*r));
    int i, j;
    for (i=0;i<TIME_NEAR;i++) {
        link_clear(&r->near[i]);
    }
    
    for (i=0;i<4;i++) {
        for (j=0;j<TIME_LEVEL;j++){
            link_clear(&r->t[i][j]);
        }
    }
    SPIN_INIT(r)
    r->current = 0;
    return r;
}

```

##### 添加定时器

通过`skynet_server.c`中的`cmd_timeout`调用`skynet_timeout`添加新的定时器

```c
// TI为全局的定时器指针
static struct timer * TI = NULL; 

int
skynet_timeout(uint32_t handle, int time, int session) {
    ...
    struct timer_event event;
    event.handle = handle;  // callback
    eveng.session = session;
    // 添加新的定时器节点
    timer_add(TI, &event, sizeof(event), time);
    return session;
}

static void timer_add(struct timer *T, void 8arg, size_t sz, int time) {
    // 给timer_node指针分配空间，还需要分配timer_node + timer_event大小的空间，
    // 之后通过node + 1可获得timer_event数据
    struct timer_node *node = (struct timer_node *)skynet_malloc(sizeof(*node)+sz);
    memcpy(node+1,arg,sz);
    SPIN_LOCK(T);
    node->expire=time+T->time;
    add_node(T, node);
    SPIN_UNLOCK(T);
}

// 添加到定时器链表里，如果定时器的到期滴答数跟当前比较近(<2^8)，表示即将触发定时器添加到T->near数组里
// 否则根据差值大小添加到对应的T->T[i]中
static void add_node(struct timer *T, struct timer_node *node) {
    uint32_t time=node->expire;
    uint32_t current_time=T->time;
    // 将current_time和time的最后8位都置1(让near层位相等)，比较高位是否相等(后四层) 
    // 如果相等，则认为其属于near层，然后根据idx，将node添加到对应位置的定时器链表(link_list)中
    if((time|TIME_NEAR_MASK)==(current_time|TIME_NEAR_MASK)) {
        link(&T->near[time&TIME_NEAR_MASK], node);
    }else {
        int i;
        uint32_t mask=TIME_NEAR << TIME_LEVEL_SHIFT;
        for (i = 0; i < 3; i++) {
            if ((time|(mask-1))==(current_time|(mask-1))){
                break;
            }
            mask << TIME_LEVEL_SHIFT;
        }
        // time >> (TIME_NEAR_SHIFT + i*TIME_LEVEL_SHIFT)  将Li层的所有bit移到最右侧
        // & TIME_LEVEL_MASK, 求余得到对应idx
        link(&T->t[i][((time>>(TIME_NEAR_SHIFT + i*TIME_LEVEL_SHIFT)) & TIME_LEVEL_MASK)],node);
    }
}

// 同一个定时器链表，表示该节点上的所有定时器同时触发
static inline void
link(struct link_list *list, struct timer_node *node) {
    list->tail->next = node;
    list->tail = node;
    node->next = 0;
}
```

##### 驱动方式

`skynet`启动时，会创建一个线程专门跑定时器，每帧(`0.0025s`)调用`skynet_updatetime()`

```c
// skynet_start.c
static void * 
thread_timer(void *p) {
    struct monitor * m = p;
    skynet_initthread(THREAD_TIMER);
    for (;;) {
        skynet_updatetime();  // 调用timer_update
        skynet_socket_updatetime();
        CHECK_ABORT
        wakeup(m,m->count-1);
        usleep(2500);  // 2500微秒 = 0.0025s
        if (SIG) {
            signal_hup();
            SIG = 0;
        }
    }
    ...
}
```

每个定时器设置一个到期滴答数，与当前系统的滴答数(启动时为0，1滴答1滴答往后跳，1滴答==0.01s ？？？)比较得到差值interval;

如果interval比较小(0 <= interval <= 2^8-1)，表示定时器即将到来，保存在第一部分前2^8个定时器链表中；否则找到属于第二部分对用的层级中。

```c
// skynet_timer.c

static struct timer * TI = NULL;

void 
skynet_updatetime(void) {
    ...
    uint32_t diff = (uint32_t)(cp - TI->current_point); 
    TI->current_point = cp;
    TI->current += diff;
    for (i = 0; i < diff; i++){
        timer_update(TI);        
    }
}

static void
timer_update(struct timer *T) {
    SPIN_LOCK(T);
    timer_execute(T);
    timer_shift(T);
    timer_execute(T);
    SPIN_UNLOCK(T);
}

// 每帧从T->near中触发到期得定时器
static inline void
timer_execute(struct timer *T) {
    int idx = T->time & TIME_NEAR_MASK;  // 保留T->time当前滴答数的低8位，取得NEAR数组得idx
    while (T->near[idx].head.next) {
        // 清除对应idx的定时器链表，并返回链表信息
        struct timer_node *current = link_clear(&T->near[idx]);
        SPIN_UNLOCK(T);
        // 处理current链表中的定时器信息
        dispatch_list(current);
        SPIN_LOCK(T);
    }
}

// 遍历处理定时器链表中所有的定时器
static inline void
dispatch_list(struct timer_node *current) {
    do {
        // node + 1取event
        struct timer_event * event = (struct timer_event *)(current + 1);
        struct skynet_message message;
        message.source = 0;
        ...
        skynet_context_push(event->handle, &message);
        struct timer_node * temp = current;
        current = current->next;
        skynet_free(temp);
    }while(current);
}

/*
每帧处理了触发定时器外，还需重新分配定时器所在区间(timer_shift)

当T->time的低8位不全为0时，不需要分配，所以每2^8个滴答数才有需要分配一次；

当T->time的第9-14位不全为0时，重新分配T[0]等级，每2^8个滴答数分配一次，idx从1开始，每次分配+1；

当T->time的第15-20位不全为0时，重新分配T[1]等级，每2^(8+6)个滴答数分配一次，idx从1开始，每次分配+1；

当T->time的第21-26位不全为0时，重新分配T[2]等级，每2^(8+6+6)个滴答数分配一次，idx从1开始，每次分配+1；

当T->time的第27-32位不全为0时，重新分配T[3]等级，每2^(8+6+6+6)个滴答数分配一次，idx从1开始，每次分配+1；
*/
static void
timer_shift(struct timer *T) {
    int mask = TIME_NEAR;
    uint32_t ct = ++T->time;
    if (ct == 0){
        // 将第4层，idx为0的定时器链表，删掉并重新add_node
        move_list(T, 3, 0);
    } else {
        uint32_t time = ct >> TIME_NEAR_SHIFT;
        int i = 0;
        // T->time得低8位不全为0时，不需要分配
        // 即每2^8个滴答(2.56s)才需要分配一次
        while ((ct & (mask - 1)) == 0) {
            // 分配T->t中某等级
            int idx = time & TIME_LEVEL_MASK;
            if (idx != 0) {
                move_list(T, i, idx);
                break;
            }
            mask <<= TIME_LEVEL_SHIFT;
            time >>= TIME_LEVEL_SHIFT;
            ++i;
        }
    }
}

static void 
move_list(struct timer *T, int level, int idx) {
    // 将level层的idx位置的定时器链表从当前位置删除，并重新add_node
	struct timer_node *current = link_clear(&T->[level][idx]);
    while(current) {
        struct timer_node *temp = current->next;
        add_node(T, current);
        current=tmp;
    }
}
```




























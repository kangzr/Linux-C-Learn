> 本文主要介绍协程的原理及实现，对比进程、线程与协程的区别，介绍基于glibc库中的ucontext以及汇编的两种实现方法。



## 进程，线程与协程

- **进程**是最小的**资源管理单元**，进程间切换需要**OS调度**，需要**陷入内核**，切换内容：页全局目录+内核栈+硬件上下文；多进程隔离性好，比较安全，但**切换开销大**，数据**同步效率低**。
- **线程**是最小的**执行单元**或者CPU调度的最基本单位，线程之间的切换需要**OS调度**，也即需要**陷入内核**。切换内容：内核栈+硬件上下文；多线程切换开销不及进程大，但也需要陷入内核，数据**同步效率高**，但**隔离性差**。
- **协程**是一种通过函数**执行流程**的**暂停（yield）**与**恢复（resume）**来实现协作式多任务的程序组件，协程的切换由**用户程序控制**，没有用户态与内核态之间的切换，切换内容：用户栈或堆。因此，协程切换的代价比线程更低。



## 协程原理

> coroutine：A coroutine is a function that can suspend its execution (yield) until the given yield Instruction finishes.

以上为**协程定义**，即，**协程**可在子程序（函数）内部中断（**yield**）其流程，转而执行其它函数，且之后能再从中断点继续执行（**resume**），就像什么也没有发生一样。协程可以让函数有多个入口，每个yield返回处都可被看作为入口。

我们都知道每个函数的运行都是一个压栈的过程，函数执行地址，函数返回地址，函数参数，函数中申明的变量等都需要入栈。因此，实现函数的中断和恢复，只需要做一件事，那就是**保存当前函数运行栈中的上下文环境**。根据保存上下文环境方式的不一样，接下来会简单介绍两种协程的实现。



###### 优点：

- **跨平台**，跨体系架构
- 上下文切换开销小，无需内核调度
- 无需原子操作及同步的开销
- 方便切换控制流，简化编程模型
- 协程以**同步编程**方式，达到**异步性能**的目的（同步性能不如异步，异步编程不如同步直观）
- **高并发**，高扩展，低成本，一个**CPU可支持上万协程**（但线程数有限）



###### 缺点：

- **无法利用直接多核资源**：协程本制上是**单线程**，不能同时将单个CPU的多个核用上，但是可以和进程配合，将进程挂在指定核心上，便可充分利用核心
- 阻塞操作会阻塞掉整个程序
- 不适用于**计算密集型**场景，一般用于**IO密集型**。（因无法利用多核计算资源）



## 协程实现

根据保存栈的实现方式不同，介绍两种协程实现方式：1）基于glibc库ucontext实现；2）基于汇编实现。可能不能语言的实现方式都各不相同，但，其本制都是一样，都需要实现函数运行栈的保存。

### 基于glibc库的ucontext实现

`glibc`库提供了一些用于保存上下文环境的数据结构，切换和恢复上下文环境的接口，使得在C中实现协程更加便利，接下来进行介绍。

#### ucontext_t

`glibc`提供的用于保存上下文的数据结构，可保存运行栈，各种寄存器值，后续用于恢复的上下文环境，以及信号掩码等。如下：

```c
// ucontext.h
struct ucontext {
    unsigned long uc_flags;
    // 如果当前context终止后，uc_link指向context被恢复执行
    struct ucontext_t *uc_link;
    // 运行栈
    stack_t uc_stack;
    // 各种寄存器
    struct sigcontext uc_mcontext;
    // 信号掩码
    sigset_t uc_sigmask;
};
typedef ucontext ucontext_t;
```



#### getcontext

其参数为`ucontext_t`，用于保存当前上下文环境至`ucp`中，其原型如下：

```c
int getcontext(ucontext_t *ucp);
```

需要保存的内容为：

- rbx，rbp，r12，r13，r14，r15寄存器（用于**保存数据**）
- rdi，rsi，rdx，rcx，r8，r9寄存器（用于**保存函数参数**）
- eip/rip寄存器：**指向下一条将要执行的指令**，CPU的工作起始就是不断取出它指向的指令，然后执行这条指令，同时指向下一条指令；32位为eip, 64位为rip
- esp/rsp寄存器：**指向栈顶**；32位为esp，64位为rsp
- 还有一些当前线程的信号屏蔽掩码也需要保存



#### makecontext

用于修改协程运行栈为`ucp->ctx.uc_stack`，并指定协程运行函数，其原型如下：

```c
void makecontext(ucontext_t *ucp, void(*mainfunc)(void), int argc, ...);
```

将`ucp`被激活时相应函数`mainfunc`需被执行（通过`setcontext`或`swapcontext`激活），`argc`用于指定参数个数，`...`为对应参数。当此函数返回时，`ucp->uc_link`所指向的上下文环境会被恢复，如果`ucp->uc_link`未设置，则线程退出。



#### swapcontext

将当前上下文环境保存在`oucp`中，然后恢复`ucp`指向的上下文环境，其原型如下：

```c
int swapcontext(ucontext_t *oucp, const ucontext_t *ucp);
```

在`ucp`上下文环境被恢复时，通过`makecontext`设置的`mainfunc`会被执行。



有了以上数据结构和接口之后，可以很方便的实现协程，通过`getcontext`保存协程当前上下文环境，通过`makecontext`修改协程运行栈及设置协程恢复时需要执行的函数，通过`swapcontext`进行上下文环境切换。除此之外，

还需要实现一个**调度器**，用于管理协程的创建和切换。



#### 协程demo

这是云风的一份协程demo实现。

##### 调度器和协程数据结构

```c
// 调度器数据结构
// 注意这个数据结构在堆上的分布：从低地址到高地址依次为：stack, main, nco, cap, running, co
struct schedule {
    char stack[STACK_SIZE];  // 协程运行的栈空间
    ucontext_t main;  // 主协程上下文环境
    int nco;  // 当前协程数
    int cap;  // 协程容量
    int running;  // 当前运行的是哪个协程
    struct coroutine **co;  // 保存所有创建的协程
}

// 协程数据结构
struct coroutine {
    /*typedef void (*coroutine_func)(struct schedule *, void *ud)*/
    coroutine_func func;  // 协程目标函数
    void *ud;  // 函数参数
    ucontext_t ctx;  // 上下文环境
    struct schedule * sch;  // 所属调度器
    ptrdiff_t cap;  // 协程Stack空间容量
    ptrdiff_t size;  // 协程Stack空间实际大小
    int status;  // 协程状态 DEAD READ RUNNING SUSPEND
    char *stack;  // 栈
}
```

这里**需要特别注意**`schedule`结构体重的`stack`数组，会作为一个运行栈来使用，接下来协程所有的运行环境都会在这个数组上进行。这里请参考另一篇文章。

##### 创建和初始化调度器

需要将`running`置为-1，表明当前没有在运行的协程

```c
struct schedule *
coroutine_open(void) {
    struct schedule *S = malloc(sizeof(*S));  // 调度器在堆上分配内存
    S->nco = 0;
    S->cap = DEFUALT_COROUTINE;   // 协程容量
    S->running = -1;
    S->co = malloc(sizeof(struct coroutine *) * S->cap);
    memset(S->co, 0, sizeof(struct coroutine *) * S->cap);
    return S;
}
```

##### 创建和初始化协程

创建协程设置协程执行函数`func`，注意这个函数会在上述`makecontext`设置的`mainfunc`中被调用。在创建新协程时，需要考虑调度器`S`中的容量是否足够，不够则需要考虑扩容。

```c
_co_new(struct schedule *S, coroutine_func func, void *ud) {
    struct coroutine * co = malloc(sizeof(*co));
    co->func = func;
    co->ud = ud;
    co->sch = S;
    co->cap = 0;
    co->size = 0;
    co->status = COROUTINE_READY;  // 初始化状态
    co->stack = NULL;
    return co;
}
int
coroutine_new(struct schedule *S, coroutine_func func, void *ud) {
    struct coroutine *co = _co_new(S, func, ud);
    if (S->nco >= S->cap) {
        // 这里需要考虑扩容
    } else {
        int i, id;
        for (i = 0; i < S->cap; i++) { // 找空闲id
            // 从S->nco开始，效率更高
            id = (i + S->nco) % S->cap;
            if (S->co[id] == NULL) {
                S->co[id] = co;
                ++S->nco;
                return id;
            }
        }
    }
    assert(0);
    return -1;
}
```

##### resume

协程获得CPU执行权，这里需要考虑两种情况：1）协程首次执行；2）协程非首次执行。如果是首次执行，需要调用`getcontext`保存当前其当前上下文。

```c
static void
mainfunc(uint32_t low32, uint32_t hi32) {
    struct schedule *S = (struct schedule *)ptr;
    int id = S->running;
    struct coroutine *C = S->co[id];
    C->func(S, C->ud);
    // 如果C->ctx设置uc_link，func返回后恢复uc_link指向的上下文环境
}

void
coroutine_resume(struct schedule * S, int id) {
    struct coroutine *C = S->co[id];
    int status = C->status;
    switch(status) {
          /*第一次唤醒，需要先getcontext, 再makecontext, 最后swapcontext*/
     	case COROUTINE_READY:
            getcontext(&C->ctx); // 将当前上下文保存到C->ctx
            // C->ctx指向S的栈，作为运行时栈，后续操作都在此栈空间进行
            C->ctx.uc_stack.ss_sp = S->stack;
            C->ctx.uc_stack.ss_size = STACK_SIZE;
            // 设置successor context，如果设置为NUll，则func返回后直接退出退出线程
            C->ctx.uc_link = &S->main;
            // 设置C为当前运行的协程，并改变其状态
            S->running = id;
            C->status = COROUTINE_RUNNING;
            uintptr_t ptr = (uintptr_t)S;
            // 修改C->ctx上下文环境
            // C->ctx被激活时，需要调用mainfunc函数，传入2个参数
            makecontext(&C->ctx, (void (*)(void)) mainfunc, 2, (uint32_t)ptr, (uint32_t)(ptr>>32));
            // C->ctx会激活，会调用mainfunc函数
            swapcontext(&S->main, &C->ctx);
            break;
        case COROUTINE_SUSPEND:  // 只需要swapcontext即可
            memcpy(S->stack + STACK_SIZE - C->size, C->Stack, C->size);
            S->running = id;
            C->status = COROUTINE_RUNNING;
            // C->ctx被激活调用mainfunc函数
            swapcontext(&S->main, &C->ctx);
            break;
        default:
            assert(0);
    }
}
```

##### yield

协程让出CPU执行权，保存运行栈，修改协程状态，然后切换上下文，一气呵成：

```c
static void
_save_stack(struct coroutine *C, char *top) {
    char dummy = 0;
    assert(top - &dummy <= STACK_SIZE);
    if (C->cap < top - &dummy) {
        free(C->stack);
        C->cap = top-&dummy;
        C->stack = malloc(C->cap);
    }
    C->size = top - &dummy;
    memcpy(C->stack, &dummy, C->size);
}

void
coroutine_yield(struct schedule * S) {
    int id = S->running;
    struct coroutine * C = S->co[id];
    _save_stack(C, S->stack + STACK_SIZE);
    C->status = COROUTINE_SUSPEND;
    S->running = -1;
    swapcontext(&C->ctx, &S->main);
}
```

注意这里的`_save_stack`实现保存运行栈，里面没有用一行汇编代码，如果没看懂请参考这篇文章。

以上为协程的主要实现内容，以下为测试例子：

```c
struct args {
    int n;
};

static void
foo(struct schedule *S, void *ud) {
    struct args * arg = ud;
    int start = arg->n;
    int i;
    for (i = 0; i < 5; i++) {
        coroutine_yield(S);
    }
}

static void
test(struct schedule *S) {
    struct args arg1 = { 0 };
    struct args arg2 = { 100 };
    // 创建协程
    int co1 = coroutine_new(S, foo, &arg1);
    int co2 = coroutine_new(S, foo, &arg2);
    while (coroutine_status(S,col)&&coroutine_status(S,co2)) {
        coroutine_resume(S,co1);
        coroutine_resume(S,co2);
    }
}

int
main() {
    // 创建调度器
    struct schedule * S = coroutine_open();
    test(S);
    coroutine_close(S);
    return 0;
}
```



### 基于汇编实现

这份实现[参考这里](https://github.com/wangbojing/NtyCo)，这个版本采用汇编进行上下文切换，实现逻辑大体一致，只介绍几个重要接口。



#### 创建协程与初始化

注意调度器为全局单例，存在线程的私有空间`pthread_setspecific`中，尝试通过`nty_coroutine_get_sched`获取，未获取成功则调用`nty_schedule_create`进行创建。

create没有exit，协程一旦创建就不能由用户自己销毁，必须以子进程执行结束自动销毁。

```c
int nty_coroutine_create(nty_coroutine **new_co, proc_coroutine func, void *arg) {
    nty_schedule *sched = nty_coroutine_get_sched();
    // 如调度器不存在，则创建；调度器为全局单例，存储在线程的私有空间pthread_setspecific
    if (sched == NULL) {
        nty_schedule_create(0);
        sched = nty_coroutine_get_sched();
    }
    // 分配一个coroutine内存空间
    // 设置其栈空间、栈大小、初始状态、创建时间，子过程回调以及参数
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



#### yield让出cpu

 让出cpu，切换到最近执行resume的上下文，该函数返回是在resume时，会有调度器统一选择 resume，然后再调用yield。resume和yield是两个可逆过程的原语操作。

```c
// 调用后该函数不会立即返回
// 而是切换到最近执行resume的上下文，由调度器统一选择resume的协程
void nty_coroutine_yield(nty_coroutine *co) {
    // co 当前运行协程实例
    co->ops = 0;
    _switch(&co->sched->ctx, &co->ctx);
}
```



#### resume恢复协程的运行权

恢复协程的运行权，需要恢复运行的协程实例调用后该函数不会立即返回，而是切换到运行协程实例的yield的位置。

```c
// 调用该函数后不会立即返回，而是切换到相应协程的yield位置。
int nty_coroutine_resume(nty_coroutine *co) {
    if (co->status * BIT(NTY_COROUTINE_STATUS_NEW)) {
        nty_coroutine_init(co);
    }
    
    nty_schedule *sched = nty_coroutine_get_sched();
    sched->curr_thread = co;
    _switch(&co->ctx, &co->sched->ctx);  // 协程切换
    sched->curr_thread = NULL;
}

// CPU有一个非常重要的寄存器叫eip，用来存储CPU运行下一条指令的地址，
// 可以把回调函数的地址存储到eip，将对应参数存储到相应的参数寄存器
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



#### 协程切换switch

`_switch`采用汇编实现，上下文切换，将cpu的寄存器暂时保存，再将即将运行的协程的上下文寄存器，分别mov到相对应的寄存器，完成切换。



```c
// 1. store: 把寄存器的值保存到原有线程
// 2. load：把新线程的寄存器值加载到cpu中
/*
 * new_ctx: 即将运行的协程上下文，寄存器列表，存在%rdi
 * cur_ctx: 正在运行的协程上下文，寄存器列表，存在
 */
int _switch(nty_cpu_ctx *new_ctx, nty_cpu_ctx *cur_ctx);

__asm__ (
"    .text                                  \n"
"       .p2align 4,,15                                   \n"
".globl _switch                                          \n"
".globl __switch                                         \n"
"_switch:                                                \n"
"__switch:                                               \n"
"       movq %rsp, 0(%rsi)      # save stack_pointer     \n"
"       movq %rbp, 8(%rsi)      # save frame_pointer     \n"
"       movq (%rsp), %rax       # save insn_pointer      \n"
"       movq %rax, 16(%rsi)                              \n"
"       movq %rbx, 24(%rsi)     # save rbx,r12-r15       \n"
"       movq %r12, 32(%rsi)                              \n"
"       movq %r13, 40(%rsi)                              \n"
"       movq %r14, 48(%rsi)                              \n"
"       movq %r15, 56(%rsi)                              \n"
"       movq 56(%rdi), %r15                              \n"
"       movq 48(%rdi), %r14                              \n"
"       movq 40(%rdi), %r13     # restore rbx,r12-r15    \n"
"       movq 32(%rdi), %r12                              \n"
"       movq 24(%rdi), %rbx                              \n"
"       movq 8(%rdi), %rbp      # restore frame_pointer  \n"
"       movq 0(%rdi), %rsp      # restore stack_pointer  \n"
"       movq 16(%rdi), %rax     # restore insn_pointer   \n"
"       movq %rax, (%rsp)                                \n"
"       ret                                              \n"
);
#endif

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



当然`Ntyco`不仅仅只是这些，是一个IO异步操作与协程结合能实现百万并发的组件，感兴趣可以`clone`源码下来看看。



[云风协程demo](https://github.com/cloudwu/coroutine)

[NtyCo](https://github.com/wangbojing/NtyCo)

[NtyCo实现原理](https://github.com/wangbojing/NtyCo/wiki/NtyCo%E7%9A%84%E5%AE%9E%E7%8E%B0)


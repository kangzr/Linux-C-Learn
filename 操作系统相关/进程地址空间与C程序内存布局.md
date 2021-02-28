#### 怀疑人生

最近在看一个[云风的协程demo](https://github.com/cloudwu/coroutine)（本文不会重点介绍协程，后续会有专门文章讲解），遇到一个让我简直怀疑人生、颠覆认知的问题，话不多说，直接把疑问抛给大家，以下为协程实现yield功能的接口：

```c
// coroutine.c
void
coroutine_yield(struct schedule * S) {
    int id = S->running;
    assert((char *)&C > S->stack);
    struct coroutine * C = S->co[id];
    assert((char *)&C > S->stack);
    _save_stack(C, S->stack + STACK_SIZE);
    C->status = COROUTINE_SUSPEND;
    S->running = -1;
    swapcontext(&C->ctx, &S->main);
}
```

介绍下协程中yield的功能，简单点说就是交出CPU控制权，但同时要保留犯罪现场（上下文环境），以便该协程重新获得CPU执行权时，能够从上次让出的地方开始执行，犯罪现场其实就是栈中的运行环境，包括各种寄存器的值等，而上述`_save_stack`就是用于保留犯罪现场，请先思考一下，这个函数是如何来保存运行栈的环境的。

```c
// coroutine.c
static void
_save_stack(struct coroutine *C, char *top) {
    char dummy = 0;
    assert((top - &dummy <= STACK_SIZE));
    if (C->cap < top - &dummy) {
        free(C->stack);
        C->cap = top - &dummy;
        C->stack = malloc(C->cap);
    }
    C->size = top - &dummy;
    memcpy(C->stack, &dummy, C->size);
}
```



为便于分析，把S的数据结构和初始化方式的主要部分贴出来：

```c
struct schedule {
    char stack[STACK_SIZE];
    ...
};
struct schedule *
coroutine_open(void) {
    struct schedule * S = malloc(sizeof(*S));
    ...
}
```

从上述可看出，结构体`S`通过`malloc`来分配，必然在堆上分配内存。再来看`_save_stack`，其中申明了一个变量`dummy`，这个属于函数调用中的局部变量，讲道理应该是在进程中的栈上分配空间才对，而这个`top`是`S->stack`地址加上其大小得到，而我们刚刚说S是分配在堆上，如果没记错的话，进程内存布局中，栈的地址应该是要高于堆的地址才对，难道`top - &dummy`不应该为负吗？如果从&dummy地址开始的`C->size`大小能够表示协程C的当前运行时环境，那么`dummy`地址岂不是应该在`S->stack`的地址范围内？也就是`dummy`这个函数局部变量在堆上分配的内存？一脑的浆糊，难道是我忘记了进程中的地址空间分布？那就先来看看吧。



#### 进程地址空间分布

进程地址空间分布如下图：

![memory_layout_of_c](..\pic\memory_layout_of_c.png)

从上图可以看出，进程地址空间从低到高依次为：代码段，初始化数据段，未初始化数据段，堆，栈，内核空间

##### 代码段（Text）

代码段存放编译好程序的机器码，代码段一般是共享的，因此只有一份拷贝可以被频繁的执行多次，比如文本编辑器，C编译器，shell终端等，代码段为只读段。

##### 初始化数据段（initialized data）

存放所有申明为`global`, `static`, `constant`,`extern`，且被初始化过的变量，这部分可分为细分为只读区域和可读可写区域。

##### 未初始化数据段（uninitialized data BSS）

BSS段存放`global`和`static`变量，且没有被显示初始化或者初始化为0的变量。

##### 堆（Heap）

堆从低地址向高地址生长，用于动态内存分配，使用malloc，realloc等函数在堆上分配内存，堆由所有共享库和动态加载的模块共享。

##### 栈（stack）

栈从高地址向低地址生长，存放局部变量，每次函数调用时，其返回的地址，以及调用者的上下文环境（各种寄存器）也存在stack。

如果栈指针和堆指针相遇时内存就耗尽了（exhausted）。以下为一段说明代码。

```c
#include <stdio.h>
char c[] = "hello world";   /*global变量，存于初始化数据段，可读可写区域*/
const char s[] = "how are you";  /*const变量，存于初始化数据段，只读区域*/
char h;  /*未初始化变量，存于BSS段*/
void test(){
    char dummy = 0;  /*局部变量，存于栈上*/
}
int main() {
    static j;  /*未初始化静态变量，存于bss段*/ 
    static int i = 11;  /*static变量，可读可写区域*/
    char *p = (char*)malloc(sizeof(*p));  /*malloc 堆上分配内存*/
    test();
    return 0;
}
```



#### 答疑解惑

从上述进程内存地址空间，以及变量在内存中的分布，我们可以确认，S是肯定在堆上分布的，因为是通过`malloc`来分布的，dummy确实在`S->stack`的地址范围内，将`&dummy`,`S->stack`，以及`top`三者内存地址打印出来，貌似也确实，如下：

<img src="../pic/coroutine_address.png" alt="coroutine_address" style="zoom:50%;" />

但是`dummy`这个变量的地址为啥会在堆上分配呢，答案就是`makecontext`中，代码入下

```c
void
coroutine_resume(struct schedule * S, int id) {
    ...
    C->ctx.uc_stack.ss_sp = S->stack;
    C->ctx.uc_stack.ss_size = STACK_SIZE;
    ...
    makecontext(&C->ctx, ...);
}
```

代码细节不展开，只贴出关键部分，以上函数实现了协程中的`resume`功能，跟`yield`功能相反，其目的是重新获得CPU执行大权。其中把`S->stack`赋值给了协程C的上下文环境，并且通过`makecontext`设置协程的运行栈为`S->stack`，也就是这份demo中的协程使用运行栈不是进程地址空间的栈，而是一块在堆上分配出来的数组模拟的运行栈。非常有娶，在堆上分配一块内存作为栈用。

因此上述的`_save_stack`就可以完美解释了，`S->stack`作为栈来使用，其生长方向必然是从高地址向低地址（堆则相反），因此`S->stack`的栈的底部为`S->stack + STACK_SIZE`，也即top所指的地址，而`S->stack`为栈的最顶部，dummy变量在此栈上分配内存，所以，dummy变量必然指向运行栈`S->stack`当前已使用内存的栈顶，因此用`top - &dummy`就是协程C所使用的`S->stack`运行栈的大小，也即其犯罪现场（上下文环境）。对比着上述三个变量的内存地址分布图和这段解释，思思品味，就能焕然大悟。

使用`S->stack`作为协程运行栈，是一种共享栈实现内存的方式，即所有协程共享此栈。但是不要把进程地址空间中的栈和此共享栈混淆！有兴趣的同学，建议看看完整代码，200行C代码实现一个简单清晰的协程，值得学习。














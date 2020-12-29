> 介绍C实现多态的方法

#### C结构体+函数指针实现多态

我们都知道C++可以通过虚函数和虚表指针来用于实现多态，通常有一个基类，基类中申明了一些虚函数，不同的子类通过继承基类并以各自的方式实现虚函数进而实现多态。那么C语言可以做到多态吗？答案是可以的，C语言通常采用结构体和函数指针来实现多态，结构体可以类比为C++中的抽象基类，结构体中申明的函数指针可以类比为基类中的虚函数。首先通过一个简单的demo来介绍C语言如何通过结构体和函数指针来实现多态，直接看代码

```c
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct event_base;

// 可理解为抽象基类
struct eventop {
    char * name;
    void * (*int) (struct event_base *);  // 函数指针，可理解为纯虚函数
};

// 基类结构体
struct event_base {
    struct eventop * evsel;
};

// 子类实现方法
static void *
epoll_init(struct event_base * base)
{
    printf("epoll_init\n");
}

// 子类实现方法
static void *
select_init(struct event_base * base) {
    printf("select_init\n");
}

// 可理解为继承eventop的子类
struct eventop epollops = {
    "epoll",  // name
    epoll_init  // 初始化
};

// select子类
struct eventop selectops = {
    "select",  // name
    select_init  // 初始化
};

int main() {
    struct event_base * base;
    base = malloc(sizeof(struct event_base));
    memset(&base->evsel, 0, sizeof(struct eventop));
    base->evsel = &epollops; // 1. 使用epollops来初始化event_base中的evsel
    // base->evsel = &selectops;  如果使用selectops来初始化，则以下init就会调用select_init
    base->evsel->init(base);  // 2. 实际会调用epollops的epoll_init函数；如果1中使用的selectops或者其它，就会调用相应的init实现
}
```

以上就是通过c语言结构体+函数指针来实现多态的方法，接下来从几个源码框架中抽取一些相关的实现来进一步说明，其核心思想没有差别。从以下源码框架中可以学习C的封装思想。

#### libevent

libevent是一个轻量级的基于事件驱动的高性能的开源网络库，支持多个平台，对多个平台的I/O复用技术进行了封装，Linux下封装了select, poll, epoll等，

以下为libevent中实现IO多路复用的多态方法（上述demo就是从这里简化而来）

```c
// event-internal.h 虚基类实现
// 所有的IO多路复用机制都必须提供5种函数接口
struct eventop {
    const char *name;
    void *(*init)(struct event_base *);
    int (*add)(struct event_base, evutil_socket_t fd, short old, short events, void *fdinfo);
    int (*del)(struct event_base, evutil_socket_t fd, short old, short events, void *fdinfo);
    int (*dispatch)(struct event_base *, struct timeeval *);
    void (*dealloc)(struct event_base *);
    ...
};

// event.c 根据配置设置
static const struct eventop *eventops[] =  {
#ifdef EVENT__HAVE_EVENT_PORTS
    &evportsops,
#endif
#ifdef EVENT__HAVE_WORKING_KQUEUE
    &kqops,
#endif
#ifdef EVENT__HAVE_EPOLL
    &epollops,
#endif
#ifdef EVENT_HAVE_SELECT
    &selectops,
#endif
...
};

struct event_base *
event_init(void)
{
    // 根据配置创建对应的event_base
	struct event_base * base = event_base_new_with_config(NULL);
    
}

struct event_base *
event_base_new_with_config(const struct event_config *cfg)
{
    ...
    for (i = 0; eventops[i] && !base->evbase; i++){
        ...
        // 函数操作集，整个各种io操作，根据系统配置和编译选项决定使用哪种IO demultiplex
        base->evsel = eventops[i];
        // evbase初始化设置，如果为epoll，则会调用epoll.c/epoll_init函数，里面主要实现epoll_create，用于创建epoll实例
        // 如果选择select，则会调用select.c/select_init函数，
        base->evbase = base->evsel->init(base);
    }
}

// epoll实现在epoll.c中
const struct eventop epollops = {
    "epoll",
    epoll_init,
    epoll_nochangelist_add,  // epoll_ctl
    epoll_nochangelist_del,
    ...
}

// 初始化 epoll_create
static void *
epoll_init(struct event_base *base)
{
   	epfd = epoll_create(1)) == -1);
    ...
}
```



#### lighttpd

实现思路通过fdevent系统，采用类似OO中面向对象的方式将IO事件处理封装，对不同的IO系统，提供统一的接口，其核心机制与libevent/libev库大同小异，都采用Reactor模型。

通过fdevent对外提供IO模型的通用接口

```c
// fdevent_impl.h 
// fdevents结构体中event_set, event_del等为函数指针，相当于纯虚函数，也是统一的接口
// 所有的IO复用都需要实现对应的方法
struct fdevents {
    fdevent_handler_t type;  // IO多路复用类型
    int (*event_set) (struct fdevents *ev, fdnode *fdn, int events);
    int (*event_del) (struct fdevents *ev, fdnode *fdn);
    int (*poll) (struct fdcevent *ev, int timeout_ms);
    int (*reset)(struct fdevents *ev);
    void (*free)(struct fdevents *ev);
    #ifdef FDEVENT_USE_LINUX_EPOLL  // epoll 相关内容
    	int epoll_fd;
    	struct epoll_event *epoll_events;
    #endif
    ...
}

// fdevent.h
// fdevent相关接口
fdevents * fdevent_init(const char * event_handler, int *max_fds, int *cur_fds, struct log_error_st *erh);

// fdevent.c
fdevents * fdevent_init(const char * event_handler, int *max_fds, int *cur_fds, struct log_error_st *erh) {
    int type = fdevent_config(&event_handler, erh);  // 根据配置获得选用的IO复用技术
    ...
    // 根据type进行相应的初始化
    switch(type) {
        #ifdef FDEVENT_USE_POLL
        case FDEVENT_HANLDER_POLL:
            if (0 == fdevent_poll_init(ev)) return ev;
            break;
        #endif
        #ifdef FDEVENT_USE_LINUX_EPOLL
        case FDEVENT_HANDLER_LINUX_SYSEPOLL:
            if (0 == fdevent_linux_sysepoll_init(ev)) return ev;  // 如果选择epoll，则走这里
        #endif
        ...
    }
}


// epoll相关的实现在fd_event_linux_sysepoll.c
int fdevent_linux_sysepoll_init(fdevents *ev) {
    ...
    // 对fdevents结构中相关的函数指针赋值，相当于实现纯虚函数接口
    ev->type = FDEVENT_HANDLER_LINUX_SYSEPOLL;
    ev->event_set = fdevent_linux_sysepoll_event_set;  // epoll_ctl
    ev->event_del = fdevent_linux_sysepoll_event_del;  //  epoll_ctl EPOLL_CTL_DEL
    ev->poll = fdevent_linux_sysepoll_poll;  // epoll_wait
    ev->free = fdevent_linux_sysepoll_free;
    
    if (-1 == (ev->epoll_fd = epoll_create(ev->maxfds))) return -1;
    return 0;
}

```



#### skynet

skynet是一个为网络游戏服务器设计的轻量框架，通过lua虚拟机抽象出隔离环境，充分利用多核优势。

接下来介绍skynet中模块的多态实现，

```c
//skynet_module.h
typedef void * (*skynet_dl_create)(void);  // 定义函数指针

struct skynet_module {
    const char * name;  // 动态库名
    void * module;
    // 所有的服务都需要实现以下四个方法(函数指针)
    skynet_dl_create create;
    skynet_dl_init init;
    skynet_dl_release release;
    skynet_dl_signal signal;
}

// skynet_module.c
static void *
get_api(struct skynet_module *mod, const char *api_name) {
    ...
    // 其中mod->module为通过dlopen将对应的服务so文件加载到内存后返回的一个可以访问该内存块的句柄
    // 根据整个句柄，找到对应模块中函数所在的内存地址，并赋值给skynet_module中的函数指针
    return dlsym(mod->module, ptr);
}

// 把具体实现的函数赋值为基类skynet_module中的虚函数(函数指针)
static int
open_sym(struct skynet_module *mod) {
    mod->create = get_api(mod, "_create");
    mod->init = get_api(mod, "_init");
    mod->release = get_api(mod, "_release");
    mod->signal = get_api(mod, "_signal");
}

// 比如logger服务  server_logger.c  生成对应logger.so
struct logger *
logger_create(void){
    
}
int logger_init(struct logger * inst, struct skynet_context *ctx, const char * param) {
}
```



以上为C语言通过结构体和函数指针实现多态的方式。




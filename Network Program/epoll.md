#### 一，Epoll工作原理

Epoll(event poll)是IO管理组件，用来取代**IO多路复用**函数poll和select。主要功能是监听多个file descriptor，并在有事件的时候通知应用程序，比传统的poll和select更加高效。

内核(kernal)中有一个interest list存放所有的epoll fds，ready list存放有IO事件到来的epoll fds，通知应用程序去处理这些fd的IO事件。epoll实例(epoll_create创建)来监控我们fd列表。

**数据结构：**

内核态: 红黑树 存储注册事件(插入时间复杂度logn)  ； 就绪队列双向链表

用户态：数组 ：epoll_wait 遍历nevent

```c
// epoll对应的结构
struct eventpool {
    ...
	struct rb_root_cached  rbr;  // 红黑树根节点，存储所有通过epoll_ctl添加进来的fd
    struct list_head rdlist; // 双链表存放有事件触发的fd，通过epoll_wait返回给用户
    ...
}

// 每个epoll中的事件(fd)的结构
struct epitem {
  	struct rb_node	rbn;  // 红黑树节点
    struct list_head 	rdllink;	// 双向链表节点
    struct epoll_filefd		ffd;	// 事件句柄信息
    struct eventpoll	*ep;		// 所属的eventpoll
    struct epoll_event	event;		//期待发生的事件类型
};

// 一些事件Flag
/*
EPOLLIN:  fd可读
EPOLLOUT: fd可写
EPOLLPRI: fd紧急可读
EPOLLERR: fd error
EPOLLET:  ET模式
*/

```

所有添加到epoll中的事件都会与**设备(网卡)驱动程序**建立回调关系，即，每当事件发生时，会调用对应的回调函数(内核中对应为**ep_poll_callback**)，将发生的事件添加到rdlist双链表中（因此，内核中时间复杂度为O(1)）



#### 二，Epoll接口函数

```c
// 创建一个epoll实例,size表示要监听的fd数目，并告知kernel需要分配多少内存。
// 成功则返回epoll实例的fd. 失败返回-1
int epoll_create(int size);

// 控制fd函数, epfd表示应用程序使用的epoll fd；即epoll_create返回的
// op控制参数，有三种：EPOLL_CTL_ADD(添加) EPOLL_CTL_MOD(修改) EPOLL_CTL_DEL(删除)
// fd表示要被操作的fd，而epoll_event则表示与fd绑定的消息结构。
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);

// 例如将fd1添加到epoll实例中
struct epoll_event ev;
ev.events = EPOLLIN | EPOLLOUT;
ev.data.ptr = (void*)socket_data;
epoll_ctl(epfd, EPOLL_CTL_MOD, fd1, &ev);

// 获取ready list
// timeout == 0:
// 不论ready list是否为空，epoll_wait都会被立即唤醒；
// timeout > 0: 
// 1. 有进程的signal信号到达 2. 最近一次epoll_wait被调用起，经过timeout ms 3. ready list 不为空
// timeout == -1:
// 1. 有进程的signal信号到达 2. ready list不为空
int epoll_wait(int epfd, struct epoll_event * events, int maxevents, int timeout);
```



#### 三，Epoll的LT和ET模式

**水平触发LT(Level Trigger)：**

一直触发

**边缘触发ET(Edge Trigger)：**

每次有新的数据包到达时`epoll wait`才会唤醒，因此没有处理完的数据会残留在缓冲区；

直到下一次客户端有新的数据包到达时，`epoll_wait`才会再次被唤醒。



**Example：**

服务端每次最多读取两个字节，客户端最多写入八个字节（`epoll_wait`被唤醒时）

Read Test：

客户端以LT模式启动，并输入aabbcc数据

服务端为LT模式：`epoll_wait`会一直触发直到数据全部读完, 分别读取aa bb cc（`epoll_wait`三次）

服务端为ET模式：只会读取两个字节，其它缓存，直到新数据包到达，`epoll_wait`再次唤醒

Write Test:

服务端为LT模式启动，客户端发送aabbccddeeffgghh

客户端为LT模式：一次写入八字节，分两次写完。(`epoll_wait`被唤醒两次)，读完数据后将EPOLLOUT从该fd移除。

客户端为ET模式：只写入八字节，`epoll_wait`只被唤醒一次。可能导致输入数据丢失。



#### 四，Epoll与poll，select对比

select：**无差别轮询**，当IO事件发生，将所有监听的fds传给内核，内核遍历所有fds，并将标记有事件的fd，遍历完将所有fd返回给应用程序，应用程序需要遍历整个数组才能知道哪些fd发生了事件，时间复杂度为O(n)，还有fds从用户态copy到内核态的开销。基于数组，监听fd有大小限制。

poll：与select无本质区别，但没有最大连接数限制，因其**基于链表存储**。

epoll: 时间复杂度为O(1)。一旦有fd就绪，内核采用类似callback机制激活该fd，epoll_wait边收到通知。**消息传递方式**：内核和用户空间共享一块内存mmap，减少copy开销。且没有最大并发连接数限制。



#### 五，Epoll应用场景

- 单线程epoll：redis（为何这么快？1. redis纯内存操作，2 只有一个epoll管理，没有多线程加锁以及切换带来的开销）
- 多进程epoll：nginx
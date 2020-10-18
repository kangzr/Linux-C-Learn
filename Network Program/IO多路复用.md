### **为何会出现IO多路复用？**

recv接口会阻塞直到有数据可读，单线程会导致主线程被阻塞，即没有数据到来整个程序永远锁死。当然可以通过多线程解决，还是效率低，扩展性差。

### Select

可在同一个线程中监听多个IO

#### 1. select原型及相关fd处理函数

> select() allows a program to monitor multiple file descriptors, waiting until one or more of the fd become 'ready' for some class of I/O operation.
>
> can monitor only fd numbers that are less than FD_SETSIZE.

```c
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

struct timeval {
    long tv_sec;
    long tv_unsec;
}
// 原型
int select(int numfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
/*
 * 参数含义如下
 * numfds: 		需要检查的最大文件描述符 + 1
 * readfds: 	需监视的读文件描述符(fd)集合
 * writefds: 	需监视的写文件描述符(fd)集合
 * exceptfds:	需监视的异常文件描述符(fd)集合
 * timeout: 	
 * 		1. NULL则永远等待，直到信号或文件描述符准备好；
 *		2. 0: 从不等待，测试所有执行的fd立即返回
 *		3. >0: 等待timeout时间，还没有fd准备好，立即返回
 *
 *
 * return：返回三个fd集合(readfds, writefds, exceptfds)中fd总数；
 *		 the return value may be zero if the timeout expired before fd became ready
 */

// fd处理函数
FD_ZERO(fd_set *set);			// 清除fd_set
FD_SET(int fd, fd_set *set);	// 将fd加入set
FD_CLR(int fd, fd_set *set);	// 将fd从set中移除
FD_ISSET(int fd, fd_set *set);	// 测试set中的fd是否准备好， 测试某个位是否被置为。
// to test if a fd is still present in a set
```

关于FD_ISSET的疑惑：

取fd_set为1字节，每一个bit对应一个文件描述

- 执行FD_ZERO(&set)，将set变为0000 0000
- fd = 5; 执行FD_SET(fd, &set), 将set变为 0001 0000
- 再加入fd = 1, 2; 则set变为 0001 0011
- 执行select，则阻塞等待
- 若fd=1, fd =2 发生事件，则select返回2， 此时set变为0000 0011;    fd =5被清空

总结：

1. select **最大可监控fd数**量由sizeof(fd_set)决定，因此**可监听端口受限**
2. 需要由**两个fd_set结构**，一个用于**保存所有监听的fd**，另一个检测fd_set, 用于**记录哪些fd准备就绪**(由select将其传入内核，内核处理，将准备就绪的fd置为1)；select每次调用前必须将所监听的fds重新加载到检测fd_set中。
3. 需要维护一个用来存放大量fd的数据结构，并且select需要将其从用户态copy到内核态，处理完后再存内核态copy到用户态，复制开销大
4. `select`无法知道哪些fd准备就绪。因此需要(轮询)遍历`fd_set`集合通过`FD_ISSET`来判断fd是否就绪。所以其**时间复杂度为O(n)**，当fd数量越多，效率越低。

#### 2. example

```c
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#define BUFFER_LENGTH   1024

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Paramter Error\n");
        return -1;
    }

    int port = atoi(argv[1]);

    // 默认0，1，2三个fd表示标准输入、输出、错误
    // 其它新增的fd依次加1
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    // bind a name to a socket
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) < 0) {
        perror("bind");
        return 2;
    }

    if (listen(sockfd, 5) < 0) {
        perror("listen");
        return 3;
    }

    fd_set rfds, rset;
    FD_ZERO(&rfds);             // 清空rfds
    FD_SET(sockfd, &rfds);      // 将sockfd加入rfds集合中
    int max_fd = sockfd;        // 最新生成的sockfd 为最大fd
    int i = 0;

    while (1) {
        rset = rfds;
        int nready = select(max_fd + 1, &rset, NULL, NULL, NULL);
        if (nready < 0) {
            printf("select error : %d\n", errno);
            continue;
        }

        if (FD_ISSET(sockfd, &rset)) {
            // sockfd是否还在rset集合中
            struct sockaddr_in client_addr;
            memset(&client_addr, 0, sizeof(struct sockaddr_in));
            socklen_t client_len = sizeof(client_addr);
            int clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);
            char str[INET_ADDRSTRLEN] = {0};
            printf("recvived from %s at port:%d, sockfd:%d, clientfd:%d\n", inet_ntop(AF_INET, &client_addr.sin_addr, str, sizeof(str)), ntohs(client_addr.sin_port), sockfd, clientfd);

            if (max_fd == FD_SETSIZE) {
                // 超出selec可监控的最大fd限制
                printf ("clientfd --> out range\n");
                break;
            }
            FD_SET(clientfd, &rfds);    // 将clientfd加入rfds集合中

            if (clientfd > max_fd) max_fd = clientfd;

            printf("sockfd:%d, max_fd:%d, clientfd:%d\n", sockfd, max_fd, clientfd);

            // 处理完一个fd，则更新nready值，直到为0则表示全部处理完，然后退出
            if (--nready == 0) continue;
        }

        // 遍历所有的文件描述符, 因此select时间复杂度为O(n)
        for (i = sockfd + 1; i <= max_fd; i ++) {
            if (FD_ISSET(i, &rset)) {
                char buffer[BUFFER_LENGTH]= {0};
                int ret = recv(i, buffer, BUFFER_LENGTH, 0);
                if (ret < 0) {
                    // 多个线程同时操作，可能导致数据被其它线程读取完
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        printf("read all data");
                    }
                    FD_CLR(i, &rfds);   // 将文件描述符i从rfds中移除
                    close(i);
                } else if (ret == 0) {
                    // 断开连接
                    printf("disconnect %d\n", i);
                    FD_CLR(i, &rfds);
                    close(i);
                    break;
                } else {
                    printf("Recv: %s, %d Bytes\n", buffer, ret);
                }
                // 更新nready
                if (--nready == 0) break;
            }
        }
    }
    return 0;
}


// usage: 
// gcc -0 select select.c
// server1: ./select port
// server2: nc localhost port
```

##### 3. select数据结构及实现原理



---



### Poll

跟select效率差不多，其没有最大可监听fd限制，因为其底层通过链表实现

#### 1. poll原型

```c
int poll (struct pollfd *ufds, unsigned int nfds, int timeout);
/*
 * ufds: 要监控的fd集合
 * nfds: fd数量
 * timeout: < 0 一直等待； == 0 立即返回；
 */

```

与select不同的是，poll调用完后，内核不会清空数组，而是通过revents来设置哪些事件是否到来。

#### 2. example

```c
    struct pollfd fds[POLL_SIZE] = {0};
    fds[0].fd = sockfd; // 设置监控fd
    fds[0].events = POLLIN; // 设置监控事件

    int max_fd = 0, i = 0;
    for (i = 1;i < POLL_SIZE;i ++) {
        fds[i].fd = -1;
    }
    
	while (1) {
        // 同select，返回准备就绪的fd数量
        // 不同于select的是，poll没有将可读可写错误三种状态分开，且通过revents来设置某些事件是否触发
        int nready = poll(fds, max_fd+1, 5);
        if (nready <= 0) continue;

        if ((fds[0].revents & POLLIN) == POLLIN) {

            struct sockaddr_in client_addr;
            memset(&client_addr, 0, sizeof(struct sockaddr_in));
            socklen_t client_len = sizeof(client_addr);

            int clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);
            if (clientfd <= 0) continue;

            char str[INET_ADDRSTRLEN] = {0};
            printf("recvived from %s at port %d, sockfd:%d, clientfd:%d\n", inet_ntop(AF_INET, &client_addr.sin_addr, str, sizeof(str)),
                ntohs(client_addr.sin_port), sockfd, clientfd);

            fds[clientfd].fd = clientfd;
            fds[clientfd].events = POLLIN;

            if (clientfd > max_fd) max_fd = clientfd;

            if (--nready == 0) continue;
        }

        for (i = sockfd + 1;i <= max_fd;i ++) {
            if (fds[i].revents & (POLLIN|POLLERR)) {
                char buffer[BUFFER_LENGTH] = {0};
                int ret = recv(i, buffer, BUFFER_LENGTH, 0);
                if (ret < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        printf("read all data");
                    }

                    //close(i);
                    fds[i].fd = -1;
                } else if (ret == 0) {
                    printf(" disconnect %d\n", i);

                    close(i);
                    fds[i].fd = -1;
                    break;
                } else {
                    printf("Recv: %s, %d Bytes\n", buffer, ret);
                }
                if (--nready == 0) break;
            }
        }
    }

```

#### 3. pool数据结构与实现原理

``` c
struct pollfd {
	int fd;
	short events;	//要求查询的事件掩码
	short revent;	//返回的事件掩码
};

```



---



### Epoll

#### 1. epoll数据结构与工作原理

Epoll(event poll)是IO管理组件，用来取代**IO多路复用**函数poll和select。主要功能是监听多个file descriptor，并在有事件的时候通知应用程序，比传统的poll和select更加高效。

内核(kernal)中有一个interest list（**红黑树**）存放所有的epoll fds，ready list存放有IO事件到来的epoll fds，通知应用程序去处理这些fd的IO事件。epoll实例(epoll_create创建)来监控我们fd列表。(**队列共用红黑树节点**?)

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

#### 2. epoll相关接口函数

```c
// 创建一个epoll实例,size表示要监听的fd数目，并告知kernel需要分配多少内存。
// 成功则返回epoll实例的fd. 失败返回-1
// epoll_create参数只有>0 和<0的区别
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

#### 3. example

```c
    int epoll_fd = epoll_create(EPOLL_SIZE);
    struct epoll_event ev, events[EPOLL_SIZE] = {0};

    ev.events = EPOLLIN;  // 默认LT
    ev.data.fd = sockfd;
    // 把sockfd带着ev加入到epoll_fd中(红黑树)
    // sockfd为key，ev为value
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &ev);

    while (1) {
		// nready 就绪fd数量，就绪fd存储再events中
        int nready = epoll_wait(epoll_fd, events, EPOLL_SIZE, -1);
        if (nready == -1) {
            printf("epoll_wait\n");
            break;
            // continue
        }

        int i = 0;
        // 遍历就绪队列nready;
        for (i = 0;i < nready;i ++) {
            if (events[i].data.fd == sockfd) {

                struct sockaddr_in client_addr;
                memset(&client_addr, 0, sizeof(struct sockaddr_in));
                socklen_t client_len = sizeof(client_addr);

                int clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);
                if (clientfd <= 0) continue;

                char str[INET_ADDRSTRLEN] = {0};
                printf("recvived from %s at port %d, sockfd:%d, clientfd:%d\n", inet_ntop(AF_INET, &client_addr.sin_addr, str, sizeof(str)),
                    ntohs(client_addr.sin_port), sockfd, clientfd);

                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = clientfd;
                // 将clientfd 加入到epoll_fd中
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, clientfd, &ev);
            } else {
                int clientfd = events[i].data.fd;

                char buffer[BUFFER_LENGTH] = {0};
                int ret = recv(clientfd, buffer, BUFFER_LENGTH, 0);
                if (ret < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        printf("read all data");
                    }

                    close(clientfd);

                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = clientfd;
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, clientfd, &ev);
                } else if (ret == 0) {
                    printf(" disconnect %d\n", clientfd);
                    close(clientfd);

                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = clientfd;
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, clientfd, &ev);

                    break;
                } else {
                    printf("Recv: %s, %d Bytes\n", buffer, ret);
                }

            }
        }
```



#### 4. epoll的LT和ET模式

假定recvbuff中有数据为1， 无数据为0；



**水平触发LT(Level Trigger)：**

一直触发； recvbuff 为 1 则一直触发，直到recvbuff为0



**边缘触发ET(Edge Trigger)：**

recvbuff 从0 -- > 1 则触发一次

每次有新的数据包到达时`epoll wait`才会唤醒，因此没有处理完的数据会残留在缓冲区；

直到下一次客户端有新的数据包到达时，`epoll_wait`才会再次被唤醒。



**ET边缘触发/LT水平触发 区别 如何选择**

大块数据时，建议用LT，防止数据丢失？

小块数据，建议用ET;

listen socketfd：用LT，需要持续不断触发，用ET的话accept可能会丢



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

#### 4. 如何手动实现epoll

- 数据结构
  红黑树+就绪队列，两者共用节点。

- 哪些地方需要加锁：

  - rbtree：mutex
  - 就绪队列queue：spinlock（操作比较少，等待时间<线程切换时间，则选择spinelock）
  - epoll_wait：条件变量cond+mutex

- 如何检测IO数据发生了变化？即tcp协议栈如何回调到epoll

  数据流：

  ```mermaid
  graph LR;
  网卡-->skbuff-->tcp协议栈内核中-->epoll
  ```

  

  - 三次握手成功后，fd加入accept队列，会回调epoll
  - send发送数据后，会回调
  - send buffer清空后，会回调
  - 发送端close，会回调

- 协议栈回调epoll需要传哪些参数？

  - fd
  - status，可读还是可写
  - eventpoll，（epoll底层数据结构，确定调用哪个epoll）

#### 5. epoll应用场景

- 单线程epoll：redis（**为何这么快**？1. redis纯内存操作，2 只有一个epoll管理，没有多线程加锁以及切换带来的开销）
- 多进程epoll：nginx
- Redis的IO多路复用机制包含：select, epoll, evport, kqueue





### Epoll与poll，select对比

select：**无差别轮询**，当IO事件发生，将所有监听的fds传给内核，内核遍历所有fds，并将标记有事件的fd，遍历完将所有fd返回给应用程序，应用程序需要遍历整个数组才能知道哪些fd发生了事件，时间复杂度为O(n)，还有fds从用户态copy到内核态的开销。基于数组，监听fd有大小限制。

poll：与select无本质区别，但没有最大连接数限制，因其**基于链表存储**。

epoll: 时间复杂度为O(1)。一旦有fd就绪，内核采用类似callback机制激活该fd，epoll_wait边收到通知。**消息传递方式**：内核和用户空间共享一块内存mmap，减少copy开销。且没有最大并发连接数限制。



数量少时select更方便

数量多时epoll更方便

poll和select性能一样

windows用iocp

用信号实现preactor

EPOLLIN EPOLLOUT 含义
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





#### Epoll实现

- epoll_create

- epoll_ctl
- epoll_wait
- epoll_event_callback

##### 核心数据结构

#### epitem

<img src="..\pic\nty_epoll_epitem.png" alt="nty_epoll_epitem" style="zoom:75%;" />

```c
// 一个epitem即可以作为红黑树中的一个节点，又可以作为双链表中的一个节点
// 每个epitem存放着一个event， 对event查询转换成对epitem的查询
struct epitem{
  	RB_ENTRY(epitem) rbn;  // 红黑树节点
    /* 等价于
    struct {
    	struct type *rbe_left;
    	struct type *rbe_right;
    	struct type *rbe_parent;
    	int rbe_color;
    }rbn
    */
    
    LIST_ENTRY(epitem) rdlink;  // 双链表节点
    /*
    struct {
    	struct type *le_next;  // 下一个元素
    	struct type **le_prv;  // 前一个元素地址
    }
    */
    int rdy;  // 判断该节点是否同时存在与红黑树和双向链表中
    int sockfd;  // socket句柄
    struct epoll_event event;  // 存在用户填充的事件
};
```

#### eventpoll

<img src="..\pic\nty_epoll_eventpoll.png" alt="nty_epoll_eventpoll" style="zoom:75%;" />

```c
// 一个eventpoll对象对应二个epitem的容器
struct eventpoll {
  	/*
  	struct ep_rb_tree {
  		struct epitem *rbh_root;
  	}
  	*/
    ep_rb_tree rbr;  // rbr指向红黑树根节点
    int rbcnt;  // 红黑树中节点数量 (等同于多少个TCP连接事件)
    LIST_HEAD(, epitem) rblist;  // rblist 指向双向链表的头节点
    /*
    struct {
    	struct epitem *hl_first;
    }rblist;
    */
    int rbnum;  // 双向链表中的节点数量(等同于有多少个TCP连接有事件到来)
    ...
};
```



#### epoll_create

```c
// 创建epoll对象，包含一颗空红黑树+一个空双向链表
int epoll_create(int size) {
    if (size <= 0) return -1;
    nty_tcp_manager *tcp = nty_get_tpc_manager();  // 获取tcp对象
    if (!tcp) return -1;
    struct _nty_socket *epsocket = nty_socket_allocate(NTY_TCP_SOCK_EPOLL);
    if (epsocket == NULL) {
        nty_trace_epoll("malloc failed\n");
        return -1;
    }
    // 分配内存给eventpoll对象
    struct eventpoll *ep = (struct eventpoll *)calloc(1, sizeof(struct eventpoll));
    if (!ep) {
        nty_free_socket(epsocket->id, 0);
        return -1;
    }
    ep->rbcnt = 0;
    RB_INIT(&ep->rbr);  // 红黑树根节点指向NULL ep->rbr.rhb_root = NULL
    LIST_INIT(&ep->rdlist);  // ep->rdlist.lh_first = NULL;
    // 并发环境下进行互斥
    // ...
    tcp->ep = (void*)ep;
    epsocket->ep = (void*)ep;
    return epsocket->id;
    
}
```

#### epoll_ctl

```c
int epoll_ctl(int epid, int op, int sockid, struct epoll_event *event) {
    nty_tcp_manager *tcp = nty_get_tcp_manager();
    if (!tcp) return -1;
    struct _nty_socket *epsocket = tcp->fdtable->sockfds[epid];
    
	if (epsocket->socktype == NTY_TCP_SOCK_UNUSED) {
		errno = -EBADF;
		return -1;
	}
 
	if (epsocket->socktype != NTY_TCP_SOCK_EPOLL) {
		errno = -EINVAL;
		return -1;
	}

    
	struct eventpoll *ep = (struct eventpoll*)epsocket->ep;
	if (!ep || (!event && op != EPOLL_CTL_DEL)) {
		errno = -EINVAL;
		return -1;
	}

    if (op == EPOLL_CTL_ADD) {
        // 添加sockfd上关联的事件，将epitem插入红黑树中
        pthread_mutex_lock(&ep->mtx);
        struct epitem tmp;
        tmp.sockfd = sockid;
        //先在红黑树上找，根据key来找，也就是这个sockid，找的速度会非常快
        struct epitem *epi = RB_FIND(_epoll_rb_socket, &ep->rbr, &tmp);
        if (epi) {
            // 有节点，不再插入
            pthread_mutex_unlock(&ep->mtx);
            return -1;
        }
        
        // 创建一个epitem对象，即一个红黑树节点
        epi = (struct epitem*)calloc(1, sizeof(struct epitem));
        if (!epi){
            pthread_mutex_unlock(&ep->mtx);
            return -1;
        }
        
        // 把socket保存到节点中
        epi->sockfd = sockid;
        // 把增加的事件保存到节点
        memcpy(&epi->event, event, sizeof(struct epoll_event));
        // 节点插入到红黑树中
        epi = RB_INSERT(_epoll_rb_socket, &ep->rbr, epi);
        ep->rbcnt++;
        pthread_mutex_unlock(&ep->mtx);
    } else if (op == EPOLL_CTL_DEL) { 
        pthread_mutex_lock(&ep->mtx);
        struct epitem tmp;
        tmp.sockfd = sockid;
        // 寻找节点
        struct epitem *epi = RB_FIND(_epoll_rb_socket, &ep->rbr, &tmp);
        if (!epi) { 
            pthread_mutex_unlock(&ep->mtx);
            return -1;
        }
        epi = RB_REMOVE(_epoll_rb_socket, &ep->rbr, epi);
        if (!epi) {
            pthread_mutex_unlock(&ep->mtx);
            return -1;
        }
        ep->rbcnt--;
        free(epi);
        pthread_mutex_unlock(&tp->mtx);
    } else if (op == EPOLL_CTL_MOD) {
        struct epitem tmp;
        tmp.sockfd = sockid;
        struct epitem *epi = RB_FIND(_epoll_rb_socket, &ep->rbr, &tmp);
        if (!epi) {
            return -1;
        }
        epi->event.events = event->events;
        epi->event.events != EPOLLERR | EPOLLHUP;
    }
    else{
        assert(0);
    }
    return 0;
}
```



#### epoll_wait



```c
// 到双向链表中取相关的事件通知
int epoll_wait(int epid, struct epoll_event *events, int maxevents, int timeout) {
    nty_tcp_manager *tcp = nty_get_tcp_manager();
    if (!tcp) return -1;
    struct _nty_socket *epsocket = tcp->fdtable->sockfds[epid];
    struct eventpoll *ep = (struct eventpoll*)epsocket->ep;
    // 当eventpoll对象的双向链表为空时，程序会在这个while中等待一定时间
    // 直到有事件被触发，操作系统将epitem插入到双向链表上使得rdnum>0时，程序才会跳出while循环
    while (ep->rbnum == 0 && timeout != 0){
        // wait;
    }
    pthread_spin_lock(&ep->lock);
    int cnt = 0;
    // 取得事件的数量
    // ep->rdnum：代表双向链表里边的节点数量（也就是有多少个TCP连接来事件了）
    // maxevents：此次调用最多可以收集到maxevents个已经就绪【已经准备好】的读写事件
    int num = (ep->rdnum > maxevents ? maxevents : ep->rbnum);
    int i;
    while (num != 0 && !LIST_EMPTY(&ep->rdlist)) {
        // 每次都从双向链表头取得 一个一个的节点
        struct epitem *epi = LIST_FIRST(&ep->rdlist);
        // 把这个节点从双向链表中删除【但这并不影响这个节点依旧在红黑树中】
        LIST_REMOVE(epi, rdlink);
        // 这是个标记，标记这个节点【这个节点本身是已经在红黑树中】已经不在双向链表中；
        epi->rdy = 0;
        // 把事件标记信息拷贝出来；拷贝到提供的events参数中
        // 如果有节点的话则取出节点中的事件填充到用户传入的指针所指向的内存中
        memcpy(&events[i++], &epi->event, sizeof(struct epoll_event));
        num --;
        cnt ++;
        ep->rdum --;
    }
    pthread_spin_unlock(&ep->lock);
    return cnt;
}
```

#### epoll_event_callback

服务器在以下5种情况会调用epoll_event_callback（就是将eventpoll所指向的红黑树的节点插入到双向链表中。）

- 客户端connect()连入，服务器处于SYN_RCVD状态时
- 三次握手完成，服务器处于ESTABLISHED状态时
- 客户端close()断开连接，服务器处于FIN_WAIT_1和FIN_WAIT_2状态时
- 客户端send/write()数据，服务器可读时
- 服务器可以发送数据时



```c
int epoll_event_callback(struct eventpoll *ep, int sockid, uint32_t event) {
    struct epitem tmp;
    tmp.sockfd = sockid;
    
	struct epitem *epi = RB_FIND(_epoll_rb_socket, &ep->rbr, &tmp);
	if (!epi) {
		nty_trace_epoll("rbtree not exist\n");
		assert(0);
	}

    
	//(2)从红黑树中找到这个节点后，判断这个节点是否已经被连入到双向链表里【判断的是rdy标志】
	if (epi->rdy) {
		//这个节点已经在双向链表里，那无非是把新发生的事件标志增加到现有的事件标志中
		epi->event.events |= event;
		return 1;
	} 
    

    pthread_spin_lock(&ep->lock);
    //(3)标记这个节点已经被放入双向链表中，我们刚才研究epoll_wait()的时候，从双向链表中把这个节点取走的时候，这个标志被设置回了0
	epi->rdy = 1;
    
	//(4)把这个节点链入到双向链表的表头位置
	LIST_INSERT_HEAD(&ep->rdlist, epi, rdlink);
 
	//(5)双向链表中的节点数量加1，刚才研究epoll_wait()的时候，从双向链表中把这个节点取走的时候，这个数量减了1
	ep->rdnum ++;
 
	pthread_spin_unlock(&ep->lock);
	pthread_mutex_lock(&ep->cdmtx);
	pthread_cond_signal(&ep->cond);
	pthread_mutex_unlock(&ep->cdmtx);

    return 0;
}
```
















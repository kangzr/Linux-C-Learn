数据包被copy到socket的接收缓冲区，内核将执行read，select，或者epoll_wait等系统调用，唤醒等待此套接字的进程。

用户态进程调用描述符上的read，它会导致内核从其接收缓冲区中删除数据，并将该数据复制到此进程调用所提供的缓冲区中。

用户态调用write时，会将数据从用户提供的缓冲区复制到内核写入队列，内核再将数据从写入队列复制到NIC中。



如果接收缓冲区满，TCP连接的另一端尝试发送更多数据，内核将拒绝对数据包进行ACK，此为TCP拥塞控制

`listen(int sockfd, int backlog)`

服务端socket连接数由backlog参数和linux系统定义的SOMAXCONN共同决定，取两者最小者

```c
int sysctl_somaxconn = SOMAXCONN;
asmlinkage long sys_listen(int fd, int backlog)
{
    struct socket *sock;
    int err;
    if ((sock = sockfd_lookup(fd, &err)) != NULL) {
        if ((unsigned) backlog > sysctl_somaxconn)
            backlog = sysctl_somaxconn;
        err = security_socket_listen(sock, backlog);
        if (err) {
            sockfd_put(sock);
            return err;
        }
        err=sock->ops->listen(sock, backlog);
        sockfd_put(sock);
    }
    return err;
}
```



首先网卡将接收到的数据放到内核缓冲区，内核缓冲区存放的数据根据TCP信息将数据移动到具体的某一个TCP连接上的接收缓冲区内，也就是TCP接收滑动窗口内，然后应用程序从TCP的接收缓冲区内读取数据，如果应用程序一直不读取，那么滑动窗口会变小，直至为0





TCP发给对方的数据，对方在收到数据时必须给矛确认，**只有在收到对方的确认时，本方TCP才会把TCP发送缓冲区中的数据删除**。

UDP因为是不可靠连接，不必保存应用进程的数据拷贝，应用进程中的数据在沿协议栈向下传递时，以某种形式拷贝到内核缓冲区，**当数据链路层把数据传出**后就把内核缓冲区中数据拷贝删除。因此它不需要一个发送缓冲区。



tcp socket的发送缓冲区实际上是一个结构体struct sk_buff的队列,我们可以把它称为发送缓冲队列，分配一个struct sk_buff是用于存放一个tcp数据报

[参考](https://juejin.cn/post/6844903826814664711)





发送的时候数据在用户空间的内存中，当调用send()或者write()方法的时候，会将待发送的数据按照MSS进行拆分，然后将拆分好的数据包拷贝到内核空间的发送队列，这个队列里面存放的是所有已经发送的数据包，对应的数据结构就是sk_buff，每一个数据包也就是sk_buff都有一个序号，以及一个状态，只有当服务端返回ack的时候，才会把状态改为发送成功，并且会将这个ack报文的序号之前的报文都确认掉

**一个数据包对应一个sk_buff结构**

[流程](https://plantegg.github.io/2019/05/08/%E5%B0%B1%E6%98%AF%E8%A6%81%E4%BD%A0%E6%87%82%E7%BD%91%E7%BB%9C--%E7%BD%91%E7%BB%9C%E5%8C%85%E7%9A%84%E6%B5%81%E8%BD%AC/)



linux内核中，每一个网络数据包都被切分为一个个的sk_buff，sk_buff先被内核接收，然后投递到对应的进程处理，进程把skbcopy到本tcp连接的sk_reeceive_queue中，然后应答ack。





1. 驱动程序将内存中的数据包转换成内核网络模块能识别的`skb`(socket buffer)格式，然后调用`napi_gro_receive`函数

[内核处理数据包](https://morven.life/notes/networking-1-pkg-snd-rcv/)



```c
// net/core/dev.c
// 将数据包sk_buff加入到input_pkt_queue队列中
int netif_rx(struct sk_buff *skb)
{
    int this_cpu;
    struct softnet_data *queue;
    queue = &__get_cpu_var(softnet_data);  // 每个cpu对应一个softnet_data
    __get_cpu_var(netdev_rx_stat).total++;
    // 数据包放入input_pkt_queue中，如果满了，则丢弃
    if (queue->input_pkt_queue.qlen <= netdev_max_backlog) {
        if(queue->input_pkt_queue.qlen)
            if(queue->throttle)
                goto drop;
enqueue:
        __skb_queue_tail(&queue->input_pkt_queue, skb);
        ...
    }
}

// cpu软中断，处理自己input_pkt_queue中的skb
// include/linux/netdevice.h
static inline int netif_rx_ni(struct sk_buff *skb)
{
    int err = netif_rx(skb);
    if (softirq_pending(smp_processor_id()))
        do_softirq();
    return err;
}

// napi_gro_receive 调用 __netif_receive_skb_core函数
// 紧接着CPU会根据是不是有AF_PACKET类型的socket(原始套接字)，有的话，copy一份数据给它(tcpdump抓包就是抓的这里的包)
// net/core/dev.c
int netif_receive_skb_core(struct sk_buff *skb)
{
    int ret;
    ret = __netif_receive_skb_one_core(skb, false);
    return ret;
}

static int __netif_receive_skb_one_core(struct sk_buff *skb, bool pfmemalloc)
{
    ret = __netif_receive_skb_core(skb, pfmemalloc, &pt_prev);
}

static int __netif_receive_skb_core(struct sk_buff *skb, bool pfmemalloc, struct packet_type **ppt_prev)
{
    
}

// 将数据包交给内核协议栈处理
// 当内存中的所有数据包被处理完成后(poll函数执行完成)，重启网卡的硬中断，下此网卡再收到数据的时候就会通知CPU


```


































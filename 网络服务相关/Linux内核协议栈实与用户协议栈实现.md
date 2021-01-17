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



#### 三，用户态协议栈Netmap

nginx/skynet/zeromq/redis全部使用linux内核协议栈

- netmap是一个**高性能收发原始数据包**的框架，由Luigi Rizzo等人开发，包含了内核模块以及用户态库函数，

  其目标是：不修改现有操作系统软件以及不需要特殊硬件支持，**实现用户态和网卡之间数据包的高性能传**递。

- netmap通过自带的网卡驱动直接接管网卡，运行时申请一块固定的内存池，通过mmap实现网卡数据包与内存之间的

  映射；

  现在**网卡都使用多个buffer来发送和接受packet，并有一个叫NIC ring的环形数组**，NIC ring是静态分配的，它的槽指向mbufs链的部分缓冲区buffers

  netmap内存映射网卡的packet buffer到用户态，实现了自己的发送和接受报文的`circular ring`来对应网卡的ring，使用netmap时，程序运行在用户态，

  即使出了问题也不会crash操作系统。

netmap使用poll等待网卡的文件描述符的事件(可读可写)；

netmap会建立一个字符设备`/dev/netmap`，然后通过`nm_open`来注册(接管)网卡为netmap模式

（网卡进入netmap模式后，ifconfig是看不到网卡统计信息变化的，ping也不同，wireshark也抓不到报文，因为内核协议栈被旁路了）

**为什么要使用用户态协议栈？**

**`netmap`优势**

- 性能高

  - 数据包不走传统内核协议栈，不需要层层解析，减少处理数据包的时间；
  - 用户态直接与网卡的接受和发送环交互，可减少系统调用
  - 不需要进行数据包的内存内配，采用**数据包池**(内核维护？)，有数据到达时，直接从数据包池中取出一个数据包，然后将数据放入此数据包`pkt_buf`中，再将数据包的描述符放入接收环`netmap_ring`中。
  - 减少数据copy次数，数据包(内存)采用`mmap`技术映射到用户态，实现零拷贝。

- 稳定性高

  网卡寄存器数据的维护都在内核模块进行，用户不会直接操作寄存器。所以用户态操作时，不会导致操作系统崩溃

- 亲和性

  可采用`cpu`亲和性，实现`cpu`和网卡绑定，提高性能

- 易用性好

  API操作简单，用户态只需要调用ioctl函数即可完成数据包收发工作

- 与硬件解耦

  不依赖硬件，只需要对网卡驱动程序稍微做点修改就可使用。传统网卡驱动将数据包传送给内核协议栈，而修改后的数据包直接让如**netmap_ring**供用户使用

**内核协议栈流程**：

```mermaid
graph LR;
网卡数据--copy-->内核tcp/ip协议栈--copy-->应用程序
```

**用户态协议栈**：

把协议栈做到应用程序，实现零copy；使用mmap，p f_ring, libcap等

```mermaid
graph LR;
网卡数据-->应用程序
```

原生的socket可以抓到链路层的数据，还是会经过协议栈，只是不经过tcp/ip解析。

netmap：将网卡数据通过mmap映射到内存

dpdk：商业团队维护，有比较多的资料，更适合做产品。

**如何实现用户态协议栈？**

用户态协议栈NtyTCP基于netmap实现。



单机服务器实现c10M(千万并发)，考虑以下方面：

内存，cpu，磁盘，网卡，应用程序，操作系统。

**应用场景**

- 抓包程序

- 高性能发包器

- 虚拟交换机：虚拟交换机场景下，使用`netmap`可以实现不同网卡间高效数据转发，

  ​						将一个网卡数据放到另一个网卡上时，只需要将接收环(`netmap_ring`)中的packet描述符放入发送环？不需要copy数据，实现数据零拷贝

【防火墙是再协议栈上实现的】

【劫持之后所有改网卡上的网络服务都用不了了】

**了解Netmap实现了哪些功**能

example:

```c
int main(){
    struct nm_desc *nmr = nm_open("netmap:eth0", NULL, 0, NULL);  // 映射网卡etho0
    if (nmr == NULL)
  		return -1;
    
    // 内存中有数据通过poll通知cpu，poll/epoll选择标准，IO数量<1024选择poll，IO数量较大选择epoll
    struct pollfd pfd = {0};
    pfd.fd = nmr->fd;
    pfd.events = POLLIN;
    
    while(1){
        int ret = poll(&pfd, 1, -1);
        if (ret < 0) continue;
        if (poll(fd.revents & POLLIN)){
            struct nm_pkthdr *h;
            // 从内存中读取网卡数据
            unsigned char *stream = nm_nextpkt(nmr, &h);
            // stream转成以太网包
            struct ethhdr *eh = (struct ethhdr*)stream;
            // ntohs 网络字节序(大端) --> 主机字节序(可能是大端，可能是小端)
            if (ntohs(eh->h_proto) == PROTO_IP) {
                // 转UDP包
                struct udppkg *udp = (struct udppkt *)stream;
                if(udp->ip.proto == PROTO_UDP) {
                    int udp_length = ntohs(udp->ip.length);
                    udp->body[udp_length - 8] = '\0';
                    printf("udp ---> %s \n", udp->body);
                }
            }
        }
    }
    return 0;
}
```



http, ftp等使用TCP，确保可靠性；而视频、在线游戏多使用UDP，确保实时性。 (有的游戏使用kcp协议，它是一个快速可靠协议，**能以比TCP浪费10-20%的带宽为代价**，**换取平均延迟降低30-40%，且最大延迟降低三倍的效果。**

TCP为流量设计，讲究充分利用带宽，KCP为流速设计，10%-20%带宽浪费的代价换取了比 TCP快30%-40%的传输速度。TCP信道是一条流速很慢，但每秒流量很大的大运河，而KCP是水流湍急的小激流








































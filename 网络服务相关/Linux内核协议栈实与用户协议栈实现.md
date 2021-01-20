#### 用户态协议栈Netmap

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

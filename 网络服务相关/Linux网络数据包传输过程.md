在学习完计算机网络的分层模型，TCP/IP协议栈的相关知识点，以及IO多路复用后，本文将通过一个数据包在网络中的传输来将各层结构串联起来，帮助我们更好的理解网络。

#### Linux网络包接收全过程

从TCP/IP这篇文章，可知数据链路层，网络层和传输层属于内核，内核协议栈实现网络层和传输层，链路层以太网协议通过网卡驱动实现，当数据到达时，触发硬中断（给CPU相关引脚出发一个电压变化）通知数据到来，然后触发软中断ksoftirqd由内核线程处理（软中断改变内存中的一个变量的二进制值通知软中断处理程序）；

Linux中断处理函数分上半部和下半部，上半部只进行最简单工作，快速处理后释放CPU，剩下的下半部处理。

内核收包流程图：

![linux_recv_data1](..\pic\linux_recv_data1.png)

#### Linux启动工作

创建内核线程，网络子系统初始化，协议栈注册，网卡驱动初始化，启动网卡

##### 1. 创建ksoftirqd内核线程

软中断都在专门的内核线程ksoftirqd中进行，ksoftirqd创建流程如下

```c
// smpboot.c
// register a per_cpu thread related to hotplug
// 每个cpu注册一个ksfotirqd线程
int smpboot_register_percpu_thread(struct smp_hotplug_thread *plug_thread)
{
    for_each_online_cpu(cpu){
        // 创建线程
        ret = __smpboot_create_thread(plug_thread, cpu);
    }
}

static struct smp_hotplug_thread softirq_threads = {
    .store = &ksoftirqd;
    .thread_should_run = ksoftirqd_should_run,
    .thread_fn = run_ksoftirqd,
    .thread_comm = "ksoftirqd/%u",
};


// softirq.c
// 系统初始化调用
static __init int spawn_ksoftirqd(void)
{
    resigster_cpu_notifier(&cpu_nfb);
    BUG_ON(smpboot_register_percpu_thread(&softirq_threads));
    return 0;
}
early_initcall(spaw_ksoftirqd);

```

ksoftirqd创建出来后，进入自己的线程循环函数ksoftirqd_should_run, run_ksoftirqd不停判断有没有软中断需要处理。

```c
static int smpboot_thead_fn(void *data)
{
    ...
    while(1) {
        if (!ht->thread_should_run(td->cpu)){
            preempt_enable();
            schedule();
        }else {
            set_current_state(TASK_RUNNING);
            ht->thread_fn(td->cpu);
        }
    }
}
```

软中断类型如下：

```c
//file: include/linux/interrupt.h
enum
{
    HI_SOFTIRQ=0,
    TIMER_SOFTIRQ,
    NET_TX_SOFTIRQ,  // 发送数据中断
    NET_RX_SOFTIRQ,  // 接收数据中断
    BLOCK_SOFTIRQ,
    BLOCK_IOPOLL_SOFTIRQ,
    TASKLET_SOFTIRQ,
    SCHED_SOFTIRQ,
    HRTIMER_SOFTIRQ,
    RCU_SOFTIRQ,    /* Preferable RCU should always be the last softirq */

    NR_SOFTIRQS
};
```

##### 2. 网络子系统初始化

![linux_receive_softnet](D:\MyGit\Linux-C-Learn\pic\linux_receive_softnet.png)

Linux内核通过调用哦个subsys_initcall来初始化各个子系统，网络子系统初始化通过net_dev_init函数实现

```c
// include/linux/netdevice.h
// Incoming packets are placed on per-cpu queues
// 所有到来的数据包都放在每个cpu的队列中
struct softnet_data {
    struct list_head poll_list;  // 等待驱动程序将其poll函数注册进来
    struct sk_buff	*completion_queue;
    struct sk_buff_head process_queue;
    struct sk_buff_head input_pkt_queue;
};


static int __init net_dev_init(void) {
    // initialise the packet receive queues 初始化数据包接收队列
    for_each_possible_cpu(i) {
        // 每个cpu对应一个softnet_data数据结构
        struct softnet_data *sd = &per_cpu(softnet_data, i);
        memtset(sd, 0, sizeof(*sd));
        skb_queue_head_init(&sd->input_pkt_queue);
        skb_queue_head_init(&sd->process_queue);
        INIT_LIST_HEAD(&sd->pool_list);
        ....
    }
    // 设置软中断对应处理函数 保存在softirq_vec数组中
    open_softirq(NET_TX_SOFTIRQ, net_tx_action);
    open_softirq(NET_RX_SOFTIRQ, net_rx_action);
}
subsys_initcall(net_dev_init);
```

##### 3. 协议栈注册

内核实现了网络层的ip协议ip_rcv()，传输层的tcp协议tcp_v4_rcv()，以及udp协议udp_rcv()。

通过fs_initcall调用inet_init，将协议函数注册到inet_protos和ptype_base数据结构中

![linux_inet](D:\MyGit\Linux-C-Learn\pic\linux_inet.png)

```c
// net/ipv4/af_inet.c
static struct packet_type ip_packet_type __read_mostly = {
    .type = cpu_to_be16(ETH_P_IP),
    .func = ip_rcv,  // 处理函数
};

static const struct net_protocol udp_protocol = {
    .handler =  udp_rcv,  // 处理函数
    .err_handler =  udp_err,
    .no_policy =    1,
    .netns_ok = 1,
};

static const struct net_protocol tcp_protocol = {
    .early_demux    =   tcp_v4_early_demux,
    .handler    =   tcp_v4_rcv,  // 处理函数
    .err_handler    =   tcp_v4_err,
    .no_policy  =   1,
    .netns_ok   =   1,
};

static int __init inet_init(void){
    ...
    if (inet_add_protocol(&icmp_protocol, IPPROTO_ICMP) < 0)
        pr_crit("%s: Cannot add ICMP protocol\n", __func__);
    if (inet_add_protocol(&udp_protocol, IPPROTO_UDP) < 0)
        pr_crit("%s: Cannot add UDP protocol\n", __func__);
    if (inet_add_protocol(&tcp_protocol, IPPROTO_TCP) < 0)
        pr_crit("%s: Cannot add TCP protocol\n", __func__);
    ...
    // ip_packet_type注册到ptype_base哈希表中
    dev_add_pack(&ip_packet_type);
}
fs_initcall(inet_init);

// net/ipv4/protocol.c
int inet_add_protocol(const struct net_protocol *prot, unsigned char protocol)
{
    // tcp/udp对应处理函数注册到inet_protos数组中
    return !cmpxchg((const struct net_protocol **)&inet_protos[protocol],
            NULL, prot) ? 0 : -1;
}
```

inet_protos记录着udp，tcp处理函数，ptype_base存储着ip处理函数。后面软中断会通过ptype_bease找到ip_rcv函数，ip_rcv会通过inet_protos找到tcp或者udp处理函数。ip_rcv, udp_rcv可以看到很多协议处理过程。



##### 4. 网卡驱动初始化

每个驱动程序（不仅仅是网卡驱动）会使用module_init向内核注册一个初始化函数，当驱动被加载时，内核会调用这个函数。例如igb网卡驱动的代码如下：

```c
//file: drivers/net/ethernet/intel/igb/igb_main.c
static struct pci_driver igb_driver = {
    .name     = igb_driver_name,
    .id_table = igb_pci_tbl,
    .probe    = igb_probe,
    .remove   = igb_remove,
    ......
};

static int __init igb_init_module(void)
{
    ......
    ret = pci_register_driver(&igb_driver);
    return ret;
}
module_init(igb_init_module);
```

pci_register_driver调用完成后，Linux内核就知道了该驱动的相关信息，比如Igb网卡驱动的igb_driver_name和igb_probe函数地址等。当网卡被识别后，内核会调用其驱动的probe方法（igb_driver的probe方法是igb_probe）。驱动probe方法执行的目的就是让设备ready，其`igb_probe`位于drivers/net/ethernet/intel/igb/igb_main.c下

![linux_recv_eth](D:\MyGit\Linux-C-Learn\pic\linux_recv_eth.png)

第5步，网卡驱动实现了ethtool所需的接口，也在这里注册完成函数地址的注册，当ethtool发起一个系统调用后，内核会找到对应操作的回调函数。对于igb网卡来说，其实现函数都在drivers/net/ethernet/intel/igb/igb_ethtool.c下。ethtool之所以能查看网卡收发包统计、能修改网卡自适应模式、能调整RX 队列的数量和大小，是因为ethtool命令最终调用到了网卡驱动的相应方法；

第6步注册的igb_netdev_ops中包含的是**igb_open**等函数，**该函数在网卡被启动的时候会被调用**。

```c
//file: drivers/net/ethernet/intel/igb/igb_main.c
......
static const struct net_device_ops igb_netdev_ops = {
  .ndo_open               = igb_open,
  .ndo_stop               = igb_close,
  .ndo_start_xmit         = igb_xmit_frame,
  .ndo_get_stats64        = igb_get_stats64,
  .ndo_set_rx_mode        = igb_set_rx_mode,
  .ndo_set_mac_address    = igb_set_mac,
  .ndo_change_mtu         = igb_change_mtu,
  .ndo_do_ioctl           = igb_ioctl,......
```

第7步中，在igb_probe初始化过程中，还调用到了`igb_alloc_q_vector`。他注册了一个NAPI机制所必须的poll函数，对于igb网卡驱动来说，这个函数就是igb_poll,如下代码所示。

```c
static int igb_alloc_q_vector(struct igb_adapter *adapter,
                  int v_count, int v_idx,
                  int txr_count, int txr_idx,
                  int rxr_count, int rxr_idx)
{
    ......
    /* initialize NAPI */
    netif_napi_add(adapter->netdev, &q_vector->napi,
               igb_poll, 64);
}
```



##### 5. 启动网卡

以上初始化完成后，就可以启动网卡了，网卡驱动初始化时，驱动向内核注册structure net_device_ops变量，包含网卡启动、发包、设置mac地址等回调函数(函数指针)。当启动一个网卡时（例如，通过ifconfig eth0 up），net_device_ops中的igb_open方法会被调用，它通常会做以下事情：

![linux_recive_device](..\pic\linux_recive_device.png)

```c
//file: drivers/net/ethernet/intel/igb/igb_main.c
static int __igb_open(struct net_device *netdev, bool resuming)
{
    struct igb_adapter *adapter = netdev_priv(netdev);
    /* allocate transmit descriptors 分配ringBuffer， 建立内存和tx队列内存的映射关系*/
    err = igb_setup_all_tx_resources(adapter);

    /* allocate receive descriptors 分配ringBuffer， 建立内存和rx队列内存的映射关系*/
    err = igb_setup_all_rx_resources(adapter);

    /* 注册中断处理函数 */
    err = igb_request_irq(adapter);
    if (err)
        goto err_req_irq;

    /* 启用NAPI */
    for (i = 0; i < adapter->num_q_vectors; i++)
        napi_enable(&(adapter->q_vector[i]->napi));

    ......
}
```

rx ringbuffer是网络到达网卡的第一站，Rx Tx 队列的数量和大小可以通过 ethtool 进行配置，中断函数注册`igb_request_irq`

```c
static int igb_request_irq(struct igb_adapter *adapter)
{
    if (adapter->msix_entries) {
        err = igb_request_msix(adapter);
        if (!err)
            goto request_done;
        ......
    }
}

static int igb_request_msix(struct igb_adapter *adapter)
{
    ......
    for (i = 0; i < adapter->num_q_vectors; i++) {zz
        ...
        err = request_irq(adapter->msix_entries[vector].vector,
                  igb_msix_ring, 0, q_vector->name,
}
```

对于多队列的网卡，为每一个队列都注册了中断，其对应的中断处理函数是igb_msix_ring, msix方式下，每个 RX 队列有独立的MSI-X 中断，从网卡硬件中断的层面就可以设置让收到的包被不同的 CPU处理

```c
static irqreturn_t igb_msix_ring(int irq, void *data) {
    struct igb_q_vector *q_vector = data;
    igb_write_itr(q_vector);
    napi_schedule(&q_vector->napi);
    return IRQ_HANDLER;
}
```



#### 迎接数据到来

硬中断处理，内核线程处理软中断，网络协议栈处理，IP协议层处理，UDP协议层处理，recvfrom系统调用

##### 1. 硬中断处理

数据帧从网线到达网卡时，第一站就是网卡的接收队列，网卡在分配给自己的Ringbuffer中寻找可用的内存位置，找到后DMA引擎会把数据DMA到网卡之前关联的内存里，这时CPU是无感的，DMA操作完成后，网卡会像CPU发起一个硬中断，通知CPU有数据到达。

![linux_recv_ringbuffer](..\pic\linux_recv_ringbuffer.png)

当Ringbuffer满的时候，新来的数据包将给丢弃，ifconfig查看网卡时，overruns表示因为环形队列满而被丢弃的包。如果发现有丢包，可能需要通过ethtool命令来加大环形队列的长度。

网卡的硬中断注册的处理函数是igb_msix_ring

```c
//file: drivers/net/ethernet/intel/igb/igb_main.c
static irqreturn_t igb_msix_ring(int irq, void *data)
{
    struct igb_q_vector *q_vector = data;

    /* Write the ITR value calculated from the previous interrupt. */
    igb_write_itr(q_vector);  //只是记录下硬件中断频率

    napi_schedule(&q_vector->napi);

    return IRQ_HANDLED;
}


/* Called with irq disabled */
static inline void ____napi_schedule(struct softnet_data *sd,
                     struct napi_struct *napi)
{
    // 修改CPU变量softnet_data里的poll_list，将驱动napi_struct传过来的poll_list添加进来
    // softnet_data中的poll_list是一个双向列表，其中的设备都带有输入帧等着被处理
    list_add_tail(&napi->poll_list, &sd->poll_list);
    // 触发一个软中断NET_RX_SOFTIRQ，对一个变量进行一次或运算
    __raise_softirq_irqoff(NET_RX_SOFTIRQ);
}

void __raise_softirq_irqoff(unsigned int nr)
{
    trace_softirq_raise(nr);
    or_softirq_pending(1UL << nr);
}
//file: include/linux/irq_cpustat.h
#define or_softirq_pending(x)  (local_softirq_pending() |= (x))

```

硬中断工作完成，接下来交给软中断处理。

##### 2. 内核线程处理软中断

![linux_recv_soft](..\pic\linux_recv_soft.png)

```c
static int ksoftirqd_should_run(unsigned int cpu)
{
    return local_softirq_pending();
}

#define local_softirq_pending() \
    __IRQ_STAT(smp_processor_id(), __softirq_pending)

//进入真正的线程函数
static void run_ksoftirqd(unsigned int cpu)
{
    local_irq_disable();
    if (local_softirq_pending()) {
        __do_softirq();
        rcu_note_context_switch(cpu);
        local_irq_enable();
        cond_resched();
        return;
    }
    local_irq_enable();
}
// 判断根据当前CPU的软中断类型，调用其注册的action方法。
// NET_RX_SOFTIRQ注册了处理函数net_rx_action
asmlinkage void __do_softirq(void)
{
    do {
        if (pending & 1) {
            unsigned int vec_nr = h - softirq_vec;
            int prev_count = preempt_count();

            ...
            trace_softirq_entry(vec_nr);
            h->action(h);
            trace_softirq_exit(vec_nr);
            ...
        }
        h++;
        pending >>= 1;
    } while (pending);
}

// 函数开头的time_limit和budget是用来控制net_rx_action函数主动退出的，目的是保证网络包的接收不霸占CPU不放
// 等下次网卡再有硬中断过来的时候再处理剩下的接收数据包
//  这个函数中剩下的核心逻辑是获取到当前CPU变量softnet_data，对其poll_list进行遍历, 然后执行到网卡驱动注册到的poll函数。
// 对于igb网卡来说，就是igb驱动力的igb_poll函数了。
static void net_rx_action(struct softirq_action *h)
{
    struct softnet_data *sd = &__get_cpu_var(softnet_data);
    unsigned long time_limit = jiffies + 2;
    int budget = netdev_budget;
    void *have;

    local_irq_disable();

    while (!list_empty(&sd->poll_list)) {
        ......
        n = list_first_entry(&sd->poll_list, struct napi_struct, poll_list);

        work = 0;
        if (test_bit(NAPI_STATE_SCHED, &n->state)) {
            work = n->poll(n, weight);
            trace_napi_poll(n);
        }

        budget -= work;
    }
}

/**
 *  igb_poll - NAPI Rx polling callback
 *  @napi: napi polling structure
 *  @budget: count of how many packets we should handle
 **/
static int igb_poll(struct napi_struct *napi, int budget)
{
    ...
    if (q_vector->tx.ring)
        clean_complete = igb_clean_tx_irq(q_vector);

    if (q_vector->rx.ring)
        clean_complete &= igb_clean_rx_irq(q_vector, budget);
    ...
}

static bool igb_clean_rx_irq(struct igb_q_vector *q_vector, const int budget)
{
    ...

    do {
		// igb_fetch_rx_buffer和igb_is_non_eop的作用就是把数据帧从RingBuffer上取下来
        // 获取下来的一个数据帧用一个sk_buff来表示
        /* retrieve a buffer from the ring */
        skb = igb_fetch_rx_buffer(rx_ring, rx_desc, skb);

        /* fetch next buffer in frame if non-eop */
        if (igb_is_non_eop(rx_ring, rx_desc))
            continue;
        }

        /* verify the packet layout is correct */
        if (igb_cleanup_headers(rx_ring, rx_desc, skb)) {
            skb = NULL;
            continue;
        }

        /* populate checksum, timestamp, VLAN, and protocol */
        igb_process_skb_fields(rx_ring, rx_desc, skb);

        napi_gro_receive(&q_vector->napi, skb);
}


//file: net/core/dev.c
gro_result_t napi_gro_receive(struct napi_struct *napi, struct sk_buff *skb)
{
    skb_gro_reset_offset(skb);
	// 网卡GRO特性，可以简单理解成把相关的小包合并成一个大包就行，目的是减少传送给网络栈的包数，这有助于减少 CPU 的使用量。
    return napi_skb_finish(dev_gro_receive(napi, skb), skb);
}

//file: net/core/dev.c
// 数据包将被送到协议栈中
static gro_result_t napi_skb_finish(gro_result_t ret, struct sk_buff *skb)
{
    switch (ret) {
    case GRO_NORMAL:
        if (netif_receive_skb(skb))
            ret = GRO_DROP;
        break;
    ......
}
```

##### 3. 网络协议栈处理

`netif_receive_skb`函数会根据包的协议，假如是udp包，会将包依次送到ip_rcv(),udp_rcv()协议处理函数中进行处理。

![linux_recv_udp](..\pic\linux_recv_udp.png)

```c
//file: net/core/dev.c
int netif_receive_skb(struct sk_buff *skb)
{
    //RPS处理逻辑，先忽略
    ......
    return __netif_receive_skb(skb);
}

static int __netif_receive_skb(struct sk_buff *skb)
{
    ......   
    ret = __netif_receive_skb_core(skb, false);
}

static int __netif_receive_skb_core(struct sk_buff *skb, bool pfmemalloc)
{
    ......
    //pcap逻辑，这里会将数据送入抓包点。tcpdump就是从这个入口获取包的
    list_for_each_entry_rcu(ptype, &ptype_all, list) {
        if (!ptype->dev || ptype->dev == skb->dev) {
            if (pt_prev)
                ret = deliver_skb(skb, pt_prev, orig_dev);
            pt_prev = ptype;
        }
    }

    ......
	// 取出protocol，它会从数据包中取出协议信息，然后遍历注册在这个协议上的回调函数列表。
    // ptype_base 是一个 hash table，在协议注册小节我们提到过。ip_rcv 函数地址就是存在这个 hash table中的。
    list_for_each_entry_rcu(ptype,
            &ptype_base[ntohs(type) & PTYPE_HASH_MASK], list) {
        if (ptype->type == type &&
            (ptype->dev == null_or_dev || ptype->dev == skb->dev ||
             ptype->dev == orig_dev)) {
            if (pt_prev)
                ret = deliver_skb(skb, pt_prev, orig_dev);
            pt_prev = ptype;
        }
    }
}

//file: net/core/dev.c
static inline int deliver_skb(struct sk_buff *skb,
                  struct packet_type *pt_prev,
                  struct net_device *orig_dev)
{
    ......
    // 调用到了协议层注册的处理函数了。对于ip包来讲，就会进入到ip_rcv（如果是arp包的话，会进入到arp_rcv）。
    return pt_prev->func(skb, skb->dev, pt_prev, orig_dev);
}

```

##### 4. IP协议层处理

ip协议层处理函数

```c
//file: net/ipv4/ip_input.c
int ip_rcv(struct sk_buff *skb, struct net_device *dev, struct packet_type *pt, struct net_device *orig_dev)
{
    ......
    return NF_HOOK(NFPROTO_IPV4, NF_INET_PRE_ROUTING, skb, dev, NULL,
               ip_rcv_finish);
}

static int ip_rcv_finish(struct sk_buff *skb)
{
    ......
    if (!skb_dst(skb)) {
        int err = ip_route_input_noref(skb, iph->daddr, iph->saddr,
                           iph->tos, skb->dev);
        ...
    }
    ......
    return dst_input(skb);
}

//file: net/ipv4/route.c
static int ip_route_input_mc(struct sk_buff *skb, __be32 daddr, __be32 saddr,
                u8 tos, struct net_device *dev, int our)
{
    if (our) {
        rth->dst.input= ip_local_deliver;
        rth->rt_flags |= RTCF_LOCAL;
    }
}
/* Input packet from network to transport.  */
static inline int dst_input(struct sk_buff *skb)
{
    // 调用的input方法就是路由子系统赋的ip_local_deliver
    return skb_dst(skb)->input(skb);
}
//file: net/ipv4/ip_input.c
int ip_local_deliver(struct sk_buff *skb)
{
    /*
     *  Reassemble IP fragments.
     */
    if (ip_is_fragment(ip_hdr(skb))) {
        if (ip_defrag(skb, IP_DEFRAG_LOCAL_DELIVER))
            return 0;
    }
    return NF_HOOK(NFPROTO_IPV4, NF_INET_LOCAL_IN, skb, skb->dev, NULL,
               ip_local_deliver_finish);
}
static int ip_local_deliver_finish(struct sk_buff *skb)
{
    ......

    int protocol = ip_hdr(skb)->protocol;
    const struct net_protocol *ipprot;
	// inet_protos中保存着tcp_rcv()和udp_rcv()的函数地址
    // 根据包中的协议类型选择进行分发,在这里skb包将会进一步被派送到更上层的协议中，udp和tcp。
    ipprot = rcu_dereference(inet_protos[protocol]);
    if (ipprot != NULL) {
        ret = ipprot->handler(skb);
    }
}

```

##### 5. UDP协议层处理

udp协议处理函数

```c
//file: net/ipv4/udp.c
int udp_rcv(struct sk_buff *skb)
{
    return __udp4_lib_rcv(skb, &udp_table, IPPROTO_UDP);
}


int __udp4_lib_rcv(struct sk_buff *skb, struct udp_table *udptable,
           int proto)
{
    // 是根据skb来寻找对应的socket，当找到以后将数据包放到socket的缓存队列里
    // 如果没有找到，则发送一个目标不可达的icmp包。
    sk = __udp4_lib_lookup_skb(skb, uh->source, uh->dest, udptable);

    if (sk != NULL) {
        int ret = udp_queue_rcv_skb(sk, skb
    }
    icmp_send(skb, ICMP_DEST_UNREACH, ICMP_PORT_UNREACH, 0);
}
                                    
//file: net/ipv4/udp.c
int udp_queue_rcv_skb(struct sock *sk, struct sk_buff *skb)
{   
    ......

    if (sk_rcvqueues_full(sk, skb, sk->sk_rcvbuf))
        goto drop;

        rc = 0;
    ipv4_pktinfo_prepare(skb);
    bh_lock_sock(sk);
    // sock_owned_by_user判断的是用户是不是正在这个socker上进行系统调用（socket被占用）
    // 如果没有，那就可以直接放到socket的接收队列中
    // 如果有，那就通过sk_add_backlog把数据包添加到backlog队列。 
    // 当用户释放的socket的时候，内核会检查backlog队列，如果有数据再移动到接收队列中。 
    if (!sock_owned_by_user(sk))
        rc = __udp_queue_rcv_skb(sk, skb);
    else if (sk_add_backlog(sk, skb, sk->sk_rcvbuf)) {
        bh_unlock_sock(sk);
        goto drop;
    }
    bh_unlock_sock(sk);
    return rc;
}
```

`sk_rcvqueues_full`接收队列如果满了的话，将直接把包丢弃。接收队列大小受内核参数net.core.rmem_max和net.core.rmem_default影响。



##### 6. recvfrom系统调用

上面我们说完了整个Linux内核对数据包的接收和处理过程，最后把数据包放到socket的接收队列中了。epoll监听如果监听socket的读事件，那么当socket有事件到来可读时，linux内核会把socket对应的红黑树节点添加到就绪队列中，然后再copy到用户空间，用户进程调用read读取socket队列中的数据。

`recvfrom`是一个glibc的库函数，该函数在执行后会将用户进行陷入到内核态，进入到Linux实现的系统调用`sys_recvfrom`。在理解Linux对`sys_revvfrom`之前，我们先来简单看一下`socket`这个核心数据结构。

![linux_recv_socket](..\pic\linux_recv_socket.png)

`socket`数据结构中的`const struct proto_ops`对应的是协议的方法集合。每个协议都会实现不同的方法集，对于IPv4 Internet协议族来说,每种协议都有对应的处理方法，如下。对于udp来说，是通过`inet_dgram_ops`来定义的，其中注册了`inet_recvmsg`方法。

```c
//file: net/ipv4/af_inet.c
const struct proto_ops inet_stream_ops = {
    ......
    .recvmsg       = inet_recvmsg,
    .mmap          = sock_no_mmap,
    ......
}
const struct proto_ops inet_dgram_ops = {
    ......
    .sendmsg       = inet_sendmsg,
    .recvmsg       = inet_recvmsg,
    ......
}
```

`socket`数据结构中的另一个数据结构`struct sock *sk`是一个非常大，非常重要的子结构体。其中的`sk_prot`又定义了二级处理函数。对于UDP协议来说，会被设置成UDP协议实现的方法集`udp_prot`。

```c
//file: net/ipv4/udp.c
struct proto udp_prot = {
    .name          = "UDP",
    .owner         = THIS_MODULE,
    .close         = udp_lib_close,
    .connect       = ip4_datagram_connect,
    ......
    .sendmsg       = udp_sendmsg,
    .recvmsg       = udp_recvmsg,
    .sendpage      = udp_sendpage,
    ......
}
```

看完了`socket`变量之后，我们再来看`sys_revvfrom`的实现过程。

![linux_recv_core](..\pic\linux_recv_core.png)

```c
//file: net/ipv4/af_inet.c
// 在inet_recvmsg调用了sk->sk_prot->recvmsg。
int inet_recvmsg(struct kiocb *iocb, struct socket *sock, struct msghdr *msg,
         size_t size, int flags)
{   
    ......
    err = sk->sk_prot->recvmsg(iocb, sk, msg, size, flags & MSG_DONTWAIT,
                   flags & ~MSG_DONTWAIT, &addr_len);
    if (err >= 0)
        msg->msg_namelen = addr_len;
    return err;
}

// 上面我们说过这个对于udp协议的socket来说，这个sk_prot就是net/ipv4/udp.c下的struct proto udp_prot。
// 由此我们找到了udp_recvmsg方法。
//file: net/core/datagram.c:EXPORT_SYMBOL(__skb_recv_datagram);
struct sk_buff *__skb_recv_datagram(struct sock *sk, unsigned int flags,
                    int *peeked, int *off, int *err)
{
    ......
    do {
        struct sk_buff_head *queue = &sk->sk_receive_queue;
        skb_queue_walk(queue, skb) {
            ......
        }
        /* User doesn't want to wait */
        error = -EAGAIN;
        if (!timeo)
            goto no_packet;
    } while (!wait_for_more_packets(sk, err, &timeo, last));
}
}
```

终于我们找到了我们想要看的重点，在上面我们看到了所谓的读取过程，就是访问`sk->sk_receive_queue`。如果没有数据，且用户也允许等待，则将调用wait_for_more_packets()执行等待操作，它加入会让用户进程进入睡眠状态。



#### 总结

recvfrom调用后，用户进程陷入内核态，如果接收队列没有数据，进程被挂起，接下来Linux内核来等待接收数据。

Linux准备工作：

- 创建ksoftirqd内核线程，用于处理软中断
- 协议栈注册，Linux要实现arp，icmp，ip，udp，tcp等协议，每个协议都会注册处理函数用于处理数据
- 网卡驱动初始化，每个驱动都有一个初始化函数用于初始化。网卡驱动初始化会把DMA准备好，把NAPI的poll函数地址告诉内核
- 启动网卡，分配RX，TX队列，**注册中断处理函数**

数据到来：

- 网卡网卡将数据帧DMA到内存的Ringbuffer中，然后向CPU发起中断通知
- CPU响应中断请求，调用网卡启动时注册的中断处理函数，由中断处理函数发起软中断
- 内核线程ksoftirpd发现有软中断请求到来，先关闭硬中断
- ksoftirqd线程开始调用驱动的poll函数收包
- poll函数将收到的包送到协议栈注册的ip_rcv函数中
- ip_rcv函数再将包送到udp_rcv函数中

Linux收包的CPU开销：用户进程系统调用陷入内核态开销，CPU响应包硬中断的CPU开销，ksoftirqd内核线程的软中断上下文开销。



[参考](https://zhuanlan.zhihu.com/p/256428917)





[Linux数据包的接收与发送过程](https://morven.life/notes/networking-1-pkg-snd-rcv/)

[网络包传输参考](https://plantegg.github.io/2019/05/08/%E5%B0%B1%E6%98%AF%E8%A6%81%E4%BD%A0%E6%87%82%E7%BD%91%E7%BB%9C--%E7%BD%91%E7%BB%9C%E5%8C%85%E7%9A%84%E6%B5%81%E8%BD%AC/)

[网络数据包](https://juejin.cn/post/6844903826814664711)





发送的时候数据在用户空间的内存中，当调用send()或者write()方法的时候，会将待发送的数据按照MSS进行拆分，然后将拆分好的数据包拷贝到内核空间的发送队列，这个队列里面存放的是所有已经发送的数据包，对应的数据结构就是sk_buff，每一个数据包也就是sk_buff都有一个序号，以及一个状态，只有当服务端返回ack的时候，才会把状态改为发送成功，并且会将这个ack报文的序号之前的报文都删掉



**一个数据包对应一个sk_buff结构**

linux内核中，每一个网络数据包都被切分为一个个的sk_buff，sk_buff先被内核接收，然后投递到对应的进程处理，进程把skbcopy到本tcp连接的sk_reeceive_queue中，然后应答ack。

tcp socket的发送缓冲区实际上是一个结构体**struct sk_buff**的队列，我们可以把它称为**发送缓冲队列**，分配一个struct sk_buff是用于存放一个tcp数据报

驱动程序将内存中的数据包转换成内核网络模块能识别的`skb`(socket buffer)格式，然后调用`napi_gro_receive`函数

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










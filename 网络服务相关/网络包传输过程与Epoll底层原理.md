[Linux网络包发送参考](https://zhuanlan.zhihu.com/p/256428917) + [Epoll实现原理](https://blog.csdn.net/songchuwang1868/article/details/89877739?utm_source=app)+Linux源码，把网络包传输epoll实现翻个底朝天?

#### 1. TCP/IP网络分层模型

<img src="../pic/linux_net_arch.png" alt="计算机体系" style="zoom:50%;" />

TCP/IP五层模型如上图， 中间三层在属于内核。链路层协议通过网卡驱动来实现，应用层通过socket接口来访问内核数据，内核和网络设备驱动通过中断方式来处理，当有数据到达时，会给CPU相关引脚出发一个电压变化（硬中断），通知其处理数据。Linux2.4版本后软中断通过`ksoftirqd`内核线程处理，软中断是通过给内存中的一个变量的二进制值以通知软中断处理程序。



#### 2. Linux网络包接受过程

<img src="../pic/linux_net_data_streaam.png" style="zoom:50%;" />

1. 网卡收到网络传输的数据包
2. 网卡驱动以DMA方式将网卡收到的数据帧写入内存
3. 硬中断通知CPU有数据到达
4. CPU响应硬中断，调用网络驱动注册的中断处理函数，简单处理后发出软中断，释放CPU
5. `ksoftirqd`检测到有软中断请求到达，调用网卡驱动注册的poll函数开始轮询收包
6. 从`Ring Buffer`上取出保存为一个`sk_buff`
7. 协议层开始处理网络帧，处理完的数据被放到socket的接受队列



#### 3. Linux启动初始化工作

创建`ksoftirqd`内核线程，注册各协议对应的处理函数，网卡启动等。

##### 3.1 创建`ksoftirqd`内核线程

Linux软中断在专门的内核线程`ksoftirqd`中进行，相关代码在`kernel/smpboot.c/smpboot_register_percpu_thread`和`kernel/softirq.c/spawn_ksoftirqd`中

```c
// kernel/softirq.c/spawn_ksoftirqd
static struct smp_hotplug_thread softirq_threads = {
	.store			= &ksoftirqd,
	.thread_should_run	= ksoftirqd_should_run,
	.thread_fn		= run_ksoftirqd,
	.thread_comm		= "ksoftirqd/%u",
};

static __init int spawn_ksoftirqd(void)
{
	register_cpu_notifier(&cpu_nfb);
	// smpboot_register_percpu_thread 创建softirq_threads线程
	BUG_ON(smpboot_register_percpu_thread(&softirq_threads));

	return 0;
}
early_initcall(spawn_ksoftirqd);
```

`ksoftirqd`创建后，进入自己的线程循环函数`ksoftirqd_should_run`和`run_ksoftirqd`.不停地判断有没有软中断需要被处理，不仅仅是网络软中断，还有其它类型，如下

```c
// include/linux/interrupt.h
enum
{
	HI_SOFTIRQ=0,
	TIMER_SOFTIRQ,
	NET_TX_SOFTIRQ,  /*send*/
	NET_RX_SOFTIRQ,  /*recv*/
	BLOCK_SOFTIRQ,
	BLOCK_IOPOLL_SOFTIRQ,
	TASKLET_SOFTIRQ,
	SCHED_SOFTIRQ,
	HRTIMER_SOFTIRQ,
	RCU_SOFTIRQ,    /* Preferable RCU should always be the last softirq */

	NR_SOFTIRQS
};
```

##### 3.2 网络子系统初始化

<img src="../pic/linux_sub_net_init.png" alt="linux_sub_net_init" style="zoom:50%;" />

相关代码如下：

```c
// linux/net/core/dev.c
static int __init net_dev_init(void)
{
	/*
	 *	Initialise the packet receive queues.
	 */
	for_each_possible_cpu(i) {
    /*为每个CPU申请一个softnet_data*/
		struct softnet_data *sd = &per_cpu(softnet_data, i);
		memset(sd, 0, sizeof(*sd));
	}
  // 注册软中断处理函数
	open_softirq(NET_TX_SOFTIRQ, net_tx_action);
	open_softirq(NET_RX_SOFTIRQ, net_rx_action);
out:
	return rc;
}
subsys_initcall(net_dev_init);


// linux/netdevice.h
/*
 * Incoming packets are placed on per-cpu queues
 */
struct softnet_data {
	struct Qdisc		*output_queue;
	struct Qdisc		**output_queue_tailp;
	struct list_head	poll_list;  // 等待驱动程序将其poll函数注册进来
	struct sk_buff		*completion_queue;
	struct sk_buff_head	process_queue;
	/* stats */
	unsigned int		processed;
	unsigned int		time_squeeze;
	unsigned int		cpu_collision;
	unsigned int		received_rps;
	unsigned int		dropped;
	struct sk_buff_head	input_pkt_queue;
	struct napi_struct	backlog;
};

// kernel/softirq.c
// 所有的中断号与其对应的处理函数都在softirq_vec中
// ksoftirqd线程收到软中断后，从softirq_vec中找到对应的处理函数
void open_softirq(int nr, void (*action)(struct softirq_action *))
{
	softirq_vec[nr].action = action;
}
```

##### 3.3 协议栈注册

协议实现函数：ip_rcv() tcp_v4_rcv() udp_rcv()，

![linux_tcp_ip_stack](../pic/linux_tcp_ip_stack.png)

代码如下：

```c
// netdevice.h
struct packet_type {
	__be16			type;	/* This is really htons(ether_type). */
	struct net_device	*dev;	/* NULL is wildcarded here	     */
	int			(*func) (struct sk_buff *,
					 struct net_device *,
					 struct packet_type *,
					 struct net_device *);
	bool			(*id_match)(struct packet_type *ptype,
					    struct sock *sk);
	void			*af_packet_priv;
	struct list_head	list;
};

// protocol.h
struct net_protocol {
	void			(*early_demux)(struct sk_buff *skb);
	int			(*handler)(struct sk_buff *skb);
	void			(*err_handler)(struct sk_buff *skb, u32 info);
	unsigned int		no_policy:1,
				netns_ok:1;
};

// net/ipv4/af_inet.c
static struct packet_type ip_packet_type __read_mostly = {
	.type = cpu_to_be16(ETH_P_IP),
	.func = ip_rcv,
};

static const struct net_protocol udp_protocol = {
	.handler =	udp_rcv,
	.err_handler =	udp_err,
	.no_policy =	1,
	.netns_ok =	1,
};
static const struct net_protocol tcp_protocol = {
	.early_demux	=	tcp_v4_early_demux,
	.handler	=	tcp_v4_rcv,
	.err_handler	=	tcp_v4_err,
	.no_policy	=	1,
	.netns_ok	=	1,
};

static int __init inet_init(void)
{
  ...
    // inet_add_protocl将udp_protocol等添加进来
    if (inet_add_protocol(&udp_protocol, IPPROTO_UDP) < 0)
        pr_crit("%s: Cannot add UDP protocol\n", __func__);
    if (inet_add_protocol(&tcp_protocol, IPPROTO_TCP) < 0)
        pr_crit("%s: Cannot add TCP protocol\n", __func__);
  ...
    //被注册到ptype_base哈希表中
    dev_add_pack(&ip_packet_type);
}

//net/ipv4/protocol.c
//Add a protocol handler to the hash tables  (inet_protos)
int inet_add_protocol(const struct net_protocol *prot, unsigned char protocol)
{
	return !cmpxchg((const struct net_protocol **)&inet_protos[protocol],
			NULL, prot) ? 0 : -1;
}
```

**inet_protos记录着udp，tcp的处理函数地址，ptype_base存储着ip_rcv()函数的处理地址**

软中断中会通过ptype_base找到ip_rcv函数地址，进而将ip包正确地送到ip_rcv()中执行。在ip_rcv中将会通过inet_protos找到tcp或者udp的处理函数，再而把包转发给udp_rcv()或tcp_v4_rcv()函数。

##### 3.4 网卡驱动初始化

每一个驱动程序（不仅仅只是网卡驱动）会使用 `module_init` 向内核注册一个初始化函数，当驱动被加载时，内核会调用这个函数。例如，igb网卡驱动的代码位于drivers/net/ethernet/intel/igb/igb_main.c

```c

// linux/pci.h
struct pci_driver {
	struct list_head node;
	const char *name;
	const struct pci_device_id *id_table;	/* must be non-NULL for probe to be called */
	int  (*probe)  (struct pci_dev *dev, const struct pci_device_id *id);	/* New device inserted */
	void (*remove) (struct pci_dev *dev);	/* Device removed (NULL if not a hot-plug capable driver) */
	int  (*suspend) (struct pci_dev *dev, pm_message_t state);	/* Device suspended */
	int  (*suspend_late) (struct pci_dev *dev, pm_message_t state);
	int  (*resume_early) (struct pci_dev *dev);
	int  (*resume) (struct pci_dev *dev);	                /* Device woken up */
	void (*shutdown) (struct pci_dev *dev);
	int (*sriov_configure) (struct pci_dev *dev, int num_vfs); /* PF pdev */
	const struct pci_error_handlers *err_handler;
	struct device_driver	driver;
	struct pci_dynids dynids;
};

// igb网卡驱动的代码位于drivers/net/ethernet/intel/igb/igb_main.c
static struct pci_driver igb_driver = {
	.name     = igb_driver_name,
	.id_table = igb_pci_tbl,
	.probe    = igb_probe,
	.remove   = igb_remove,
	.shutdown = igb_shutdown,
	.sriov_configure = igb_pci_sriov_configure,
	.err_handler = &igb_err_handler
};

/**
 *  igb_init_module - Driver Registration Routine
 *  igb_init_module is the first routine called when the driver is
 *  loaded. All it does is register with the PCI subsystem.
 **/
static int __init igb_init_module(void)
{
  // igb网卡驱动信息注册到内核
  // 当网卡设备被识别以后，内核会调用其驱动的probe方法（igb_driver的probe方法是igb_probe）
	ret = pci_register_driver(&igb_driver);
	return ret;
}

// 下图第6步注册的igb_netdev_ops中包含的是igb_open等函数，该函数在网卡被启动的时候会被调用。
static const struct net_device_ops igb_netdev_ops = {
	.ndo_open		= igb_open,
	.ndo_stop		= igb_close,
	...
};
```

驱动probe方法执行的目的就是让设备ready，对于igb网卡，其`igb_probe`位于drivers/net/ethernet/intel/igb/igb_main.c下。主要执行的操作如下：

![linux_net_pic](../pic/linux_net_pic.png)

##### 3.5 启动网卡

当启用一个网卡时（例如，通过 ifconfig eth0 up），net_device_ops 中的 igb_open方法会被调用。它通常会做以下事情：

![linux_pic](../pic/linux_pic.png)

```c
//file: drivers/net/ethernet/intel/igb/igb_main.c
static int __igb_open(struct net_device *netdev, bool resuming)
{
    /* allocate transmit descriptors */
    err = igb_setup_all_tx_resources(adapter);
    /* allocate receive descriptors 分配了RingBuffer，并建立内存和Rx队列的映射关系*/
    err = igb_setup_all_rx_resources(adapter);
    /* 注册中断处理函数 */
    err = igb_request_irq(adapter);
    if (err)
        goto err_req_irq;
    /* 启用NAPI */
    for (i = 0; i < adapter->num_q_vectors; i++)
        napi_enable(&(adapter->q_vector[i]->napi));
  ...
}
```



#### 4. 接受网络数据

##### 4.1 硬中断处理

首先当数据帧从网线到达网卡上的时候，第一站是网卡的接收队列。网卡在分配给自己的RingBuffer中寻找可用的内存位置，找到后DMA引擎会把数据DMA到网卡之前关联的内存里，这个时候CPU都是无感的。当DMA操作完成以后，网卡会像CPU发起一个硬中断，通知CPU有数据到达。

![linux_hard_int](../pic/linux_hard_int.png)

> 注意：当RingBuffer满的时候，新来的数据包将给丢弃。ifconfig查看网卡的时候，可以里面有个overruns，表示因为环形队列满被丢弃的包。如果发现有丢包，可能需要通过ethtool命令来加大环形队列的长度。

网卡的硬中断注册的处理函数是igb_msix_ring

```c
static irqreturn_t igb_msix_ring(int irq, void *data)
{
	struct igb_q_vector *q_vector = data;
	/* Write the ITR value calculated from the previous interrupt. */
	igb_write_itr(q_vector);  // 记录一下硬件中断频率
	napi_schedule(&q_vector->napi);
	return IRQ_HANDLED;
}

// core/dev.c
/* Called with irq disabled */
static inline void ____napi_schedule(struct softnet_data *sd,
				     struct napi_struct *napi)
{
  // 修改了CPU变量softnet_data里的poll_list，将驱动napi_struct传过来的poll_list添加了进来
  // 其中softnet_data中的poll_list是一个双向列表，其中的设备都带有输入帧等着被处理
	list_add_tail(&napi->poll_list, &sd->poll_list);
  // 紧接着__raise_softirq_irqoff触发了一个软中断NET_RX_SOFTIRQ  softirq.c
	__raise_softirq_irqoff(NET_RX_SOFTIRQ);
}
```

Linux在硬中断里只完成简单必要的工作(记录一个寄存器，修改了一下下CPU的poll_list，然后发出个软中断)，剩下的大部分的处理都是转交给软中断的。

##### 4.2 `ksoftirqd`内核线程处理软中断

![linux_soft_int](../pic/linux_soft_int.png)

```c
// softirq.c
static int ksoftirqd_should_run(unsigned int cpu)
{
    return local_softirq_pending();
}

#define local_softirq_pending() \
    __IRQ_STAT(smp_processor_id(), __softirq_pending)

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
// dev.c中 NET_RX_SOFTIRQ注册了处理函数net_rx_action。所以net_rx_action函数就会被执行到了。
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
```

这里需要注意一个细节，硬中断中设置软中断标记，和ksoftirq的判断是否有软中断到达，都是基于smp_processor_id()的。这意味着只要硬中断在哪个CPU上被响应，那么软中断也是在这个CPU上处理的。所以说，如果你发现你的Linux软中断CPU消耗都集中在一个核上的话，做法是要把调整硬中断的CPU亲和性，来将硬中断打散到不通的CPU核上去。

```c
// net/core/dev.c
static void net_rx_action(struct softirq_action *h)
{
    struct softnet_data *sd = &__get_cpu_var(softnet_data);
  	// 函数开头的time_limit和budget是用来控制net_rx_action函数主动退出的，
    // 目的是保证网络包的接收不霸占CPU不放。等下次网卡再有硬中断过来的时候再处理剩下的接收数据包。
    unsigned long time_limit = jiffies + 2;
    int budget = netdev_budget;
    void *have;

    local_irq_disable();

    while (!list_empty(&sd->poll_list)) {
        ......
        // 获取到当前CPU变量softnet_data，对其poll_list进行遍历, 
        // 然后执行到网卡驱动注册到的poll函数。对于igb网卡来说，即igb_poll函数
        n = list_first_entry(&sd->poll_list, struct napi_struct, poll_list);

        work = 0;
        if (test_bit(NAPI_STATE_SCHED, &n->state)) {
            work = n->poll(n, weight);
            trace_napi_poll(n);
        }

        budget -= work;
    }
}

// igb_main.c
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
```

`igb_fetch_rx_buffer`和`igb_is_non_eop`的作用就是把数据帧从RingBuffer上取下来。为什么需要两个函数呢？因为有可能帧要占多多个RingBuffer，所以是在一个循环中获取的，直到帧尾部。获取下来的一个数据帧用一个sk_buff来表示。收取完数据以后，对其进行一些校验，然后开始设置sbk变量的timestamp, VLAN id, protocol等字段。接下来进入到napi_gro_receive中

```c
//file: net/core/dev.c
gro_result_t napi_gro_receive(struct napi_struct *napi, struct sk_buff *skb)
{
    skb_gro_reset_offset(skb);
		// `dev_gro_receive`这个函数代表的是网卡GRO特性，
    // 可以简单理解成把相关的小包合并成一个大包就行，
    // 目的是减少传送给网络栈的包数，这有助于减少 CPU 的使用量。
    return napi_skb_finish(dev_gro_receive(napi, skb), skb);
}

static gro_result_t napi_skb_finish(gro_result_t ret, struct sk_buff *skb)
{
    switch (ret) {
    case GRO_NORMAL:
        // 数据包将被送到协议栈中
        if (netif_receive_skb(skb))
            ret = GRO_DROP;
        break;
    ......
}
```

##### 4.3 网络协议栈处理

`netif_receive_skb`函数会根据包的协议，假如是udp包，会将包依次送到ip_rcv(),udp_rcv()协议处理函数中进行处理。

![linux_net_stack_handle](../pic/linux_net_stack_handle.png)

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
```

接着`__netif_receive_skb_core`取出protocol，它会从数据包中取出协议信息，然后遍历注册在这个协议上的回调函数列表。`ptype_base` 是一个 hash table，在协议注册小节我们提到过。ip_rcv 函数地址就是存在这个 hash table中的。

```c
//file: net/core/dev.c
// pt_prev->func这一行就调用到了协议层注册的处理函数了。对于ip包来讲，就会进入到ip_rcv（如果是arp包的话，会进入到arp_rcv）。
static inline int deliver_skb(struct sk_buff *skb,
                  struct packet_type *pt_prev,
                  struct net_device *orig_dev)
{
    ......
    return pt_prev->func(skb, skb->dev, pt_prev, orig_dev);
}
```

##### 4.4 IP层协议处理

```c
//file: net/ipv4/ip_input.c
int ip_rcv(struct sk_buff *skb, struct net_device *dev, struct packet_type *pt, struct net_device *orig_dev)
{
    ......

    return NF_HOOK(NFPROTO_IPV4, NF_INET_PRE_ROUTING, skb, dev, NULL,
               ip_rcv_finish);
}
```

这里`NF_HOOK`是一个钩子函数，当执行完注册的钩子后就会执行到最后一个参数指向的函数`ip_rcv_finish`。

```c
static int ip_rcv_finish(struct sk_buff *skb)
{
    ......
    if (!skb_dst(skb)) {
        int err = ip_route_input_noref(skb, iph->daddr, iph->saddr,
                           iph->tos, skb->dev);
        ...
    }
  ...
    return dst_input(skb);
}

/* Input packet from network to transport.  */
static inline int dst_input(struct sk_buff *skb)
{
  	// skb_dst(skb)->input调用的input方法就是路由子系统赋的ip_local_deliver。
    return skb_dst(skb)->input(skb);
}
```

跟踪`ip_route_input_noref` 后看到它又调用了 `ip_route_input_mc`。 在`ip_route_input_mc`中，函数`ip_local_deliver`被赋值给了`dst.input`.

`skb_dst(skb)->input`调用的input方法就是路由子系统赋的ip_local_deliver。

```c
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
		// inet_protos中保存着tcp_rcv()和udp_rcv()的函数地址。
    // 根据包中的协议类型选择进行分发,在这里skb包将会进一步被派送到更上层的协议中，udp和tcp。
    ipprot = rcu_dereference(inet_protos[protocol]);
    if (ipprot != NULL) {
        ret = ipprot->handler(skb);
    }
}
```



##### 4.5 UDP层协议处理

```c
//file: net/ipv4/udp.c
int udp_rcv(struct sk_buff *skb)
{
    return __udp4_lib_rcv(skb, &udp_table, IPPROTO_UDP);
}

int __udp4_lib_rcv(struct sk_buff *skb, struct udp_table *udptable,
           int proto)
{
  	// 是根据skb来寻找对应的socket，当找到以后将数据包放到socket的缓存队列里。
    // 如果没有找到，则发送一个目标不可达的icmp包。
    sk = __udp4_lib_lookup_skb(skb, uh->source, uh->dest, udptable);
    if (sk != NULL) {
        int ret = udp_queue_rcv_skb(sk, skb
    }
    icmp_send(skb, ICMP_DEST_UNREACH, ICMP_PORT_UNREACH, 0);
}

int udp_queue_rcv_skb(struct sock *sk, struct sk_buff *skb)
{   
    ......
    if (sk_rcvqueues_full(sk, skb, sk->sk_rcvbuf))
        goto drop;
        rc = 0;
    ipv4_pktinfo_prepare(skb);
    bh_lock_sock(sk);
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

sock_owned_by_user判断的是用户是不是正在这个socker上进行系统调用（socket被占用），如果没有，那就可以直接放到socket的接收队列中。如果有，那就通过`sk_add_backlog`把数据包添加到backlog队列。 当用户释放的socket的时候，内核会检查backlog队列，如果有数据再移动到接收队列中。

`sk_rcvqueues_full`接收队列如果满了的话，将直接把包丢弃。接收队列大小受内核参数net.core.rmem_max和net.core.rmem_default影响。

#### 5. 系统调用recvfrom

说完了整个Linux内核对数据包的接收和处理过程，最后把数据包放到socket的接收队列中了

代码里调用的`recvfrom`是一个glibc的库函数，该函数在执行后会将用户进行陷入到内核态，进入到Linux实现的系统调用`sys_recvfrom`

![linux_recievefrom](../pic/linux_recievefrom.png)



```c
// linux/net.h
struct socket {
	socket_state		state;
	struct file		*file;
	struct sock		*sk;
	const struct proto_ops	*ops;
};

struct proto_ops {
	int		family;
	struct module	*owner;
	int		(*release)   (struct socket *sock);
	int		(*bind)	     (struct socket *sock,
				      struct sockaddr *myaddr,
				      int sockaddr_len);
	int		(*connect)   (struct socket *sock,
				      struct sockaddr *vaddr,
				      int sockaddr_len, int flags);
	int		(*socketpair)(struct socket *sock1,
				      struct socket *sock2);
	int		(*accept)    (struct socket *sock,
				      struct socket *newsock, int flags);
	int		(*getname)   (struct socket *sock,
				      struct sockaddr *addr,
				      int *sockaddr_len, int peer);
	unsigned int	(*poll)	     (struct file *file, struct socket *sock,
				      struct poll_table_struct *wait);
	int		(*ioctl)     (struct socket *sock, unsigned int cmd,
				      unsigned long arg);
	int		(*listen)    (struct socket *sock, int len);
	int		(*shutdown)  (struct socket *sock, int flags);
	int		(*setsockopt)(struct socket *sock, int level,
				      int optname, char __user *optval, unsigned int optlen);
	int		(*getsockopt)(struct socket *sock, int level,
				      int optname, char __user *optval, int __user *optlen);
	int		(*sendmsg)   (struct kiocb *iocb, struct socket *sock,
				      struct msghdr *m, size_t total_len);
	int		(*recvmsg)   (struct kiocb *iocb, struct socket *sock,
				      struct msghdr *m, size_t total_len,
				      int flags);
	int		(*mmap)	     (struct file *file, struct socket *sock,
				      struct vm_area_struct * vma);
};


```

终于我们找到了我们想要看的重点，在上面我们看到了所谓的读取过程，就是访问`sk->sk_receive_queue`。如果没有数据，且用户也允许等待，则将调用wait_for_more_packets()执行等待操作，它加入会让用户进程进入睡眠状态。

```c
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

理解了整个收包过程以后，我们就能明确知道Linux收一个包的CPU开销了。首先第一块是用户进程调用系统调用陷入内核态的开销。第二块是CPU响应包的硬中断的CPU开销。第三块是ksoftirqd内核线程的软中断上下文花费的。后面我们再专门发一篇文章实际观察一下这些开销。

softnet_data 结构内的字段就是 NIC 和网络层之间处理队列，这个结构是全局的，每个cpu一个，



recv阻塞进程，进程进入等待队列，不占用CPU资源

当socket接收到数据后，操作系统将该socket等待队列上的进程重新放回到工作队列，该进程变成运行状态



[Linux网络包发送参考](https://zhuanlan.zhihu.com/p/256428917) + [Epoll实现原理](https://blog.csdn.net/songchuwang1868/article/details/89877739?utm_source=app)+Linux源码，把网络包传输epoll实现翻个底朝天

[新建一个空文件占用多少磁盘空间？](https://mp.weixin.qq.com/s/9YeUEnRnegplftpKlW4ZCA)

[文件过多时ls命令为什么会卡住](https://zhuanlan.zhihu.com/p/134207214)

[read文件一个字节实际会发生多大的磁盘IO](https://mp.weixin.qq.com/s/LcuWAg10hxZjCoyR1cMJSQ)

[write文件一个字节后何时发起磁盘写io](https://zhuanlan.zhihu.com/p/142441899)

[一台Linux服务器最多能支撑多少个TCP连接](https://mp.weixin.qq.com/s/Lkyj42NtvqEj63DoCY5btQ)





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


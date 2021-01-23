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

第6步注册的igb_netdev_ops中包含的是igb_open等函数，**该函数在网卡被启动的时候会被调用**。

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



#### 迎接数据到来

硬中断处理，内核线程处理软中断，网络协议栈处理，IP协议层处理，UDP协议层处理，recvfrom系统调用

##### 1. 硬中断处理

##### 2. 内核线程处理软中断

##### 3. 网络协议栈处理

##### 4. IP协议层处理

##### 5. UDP协议层处理

##### 6. recvfrom系统调用



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










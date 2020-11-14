DNS实现与Dpdk实现

ethernet0.virtualDev  "vmxnet3"  （把e1000千兆网卡 改成vmxnet3，因为e1000不支持多队列网卡）

ethernet0.wakeOnPcktRev True



多队列网卡：一块网卡可以支持多个数据同时进来；网卡可以并行的处理数据

nginx 每个进程对应一个cpu; 每个网卡对应一个cpu；网络请求会均分到每个网卡上； 提升cpu性能



多队列网卡对nginx的帮助？

worker_cpu_affinit  



多队列网卡可以提升网卡的性能，提升cpu的性能



hugepage巨页：提升效率，减少磁盘到内存的置换次数

内存和磁盘置换大小默认为4K，通过修改hugepage可以修改这个值

default_hugepagesz=1G hugepagesz=2M hugepages=1024



dpdk版本不同，接口差异很大



setup    x86_64-native-linux-gcc

mac-->vmware



编译好后：

1. insert module
2. set hugepage
3. 绑定网卡比如eth0







dpdk与网卡绑定后，网卡就不会出现在"ifconfig"显示的网卡中，这张网卡没有了ip和mac地址

**因此需要实现一个网卡driver**





网卡数据从/dev/kni  过来
















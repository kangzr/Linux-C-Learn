Linux NIC工作原理

Linux NIC子模块架构

Linux网卡驱动的实现

#### 网络协议分层

<img src="..\pic\linux_network.png" alt="linux_networkd_sub" style="zoom:50%;" />

#### 网卡厂家如何兼容

<img src="..\pic\linux_network_interface.png" alt="linux_network_interface" style="zoom:50%;" />



网卡：将光电信号转换为数字信号  A/D转换



网络适配器和网卡是一对一关系

Linux如何兼容多网卡？

1. 数据属性进行抽象（net_device）；
2. 接口操作进行抽象（net_device_ops）



sk_buff位于网卡和协议栈之间，网卡和协议栈之间数据运输

sk_buff如何装载一帧数据？

多个sk_buff如何组织

dmesg打印内核日志

rx 网卡私有空间，存在内核



![network_sk_buff](..\pic\network_sk_buff.png)





#### Linux用户态文件系统fuse

分布式文件系统是指文件系统管理的物理存储资源不一定直接在本地节点上，而是通过计算机网络与节点相连。比如TFS，GFS；

文件系统是操作系统用于明确存储设备或分区上的文件的方法和数据结构；

用户态文件系统是操作系统提供了一层文件存储的接口，方便用户对文件操作的接口；



**FUSE**：是一个实现在用户空间的文件系统框架，通过FUSE内核模块的支持，使用者只需要根据fuse提供的接口实现具体的文件操作就可以实现一个文件系统



##### Fuse的组成以及功能实现

FUSE由三个部分组成，FUSE内核模块，FUSE库以及一些挂在工具。

FUSE内核模块实现了和VFS的对接，还是实现了一个可以被用户空间进程打开的设备，当VFS发来文件操作请求之后，它将该请求转化为特定格式，并通过设备传递给用户空间进程，用户空间进程在处理完后，将结果返回给FUSE内核模块，将其还原为Linux Kernel需要的格式，返回给VFS；



<img src="..\pic\linux_kernel_fuse.png" alt="linux_kernel_fuse" style="zoom:75%;" />







文件系统分3类：

- 分布式文件系统：gfs/hdfs/fastdfs，解决网络存储问题，纯引用服务器
- vfs：虚拟文件系统
- ex3/ex4/fat32/ntfs，操作系统自带，解决磁盘存储（FUSE属于这一类，在内核中）






















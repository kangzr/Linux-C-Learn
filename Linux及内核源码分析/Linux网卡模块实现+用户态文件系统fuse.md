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



---



#### Linux用户态文件系统fuse（Linux2.6以后支持）

分布式文件系统是指文件系统管理的物理存储资源不一定直接在本地节点上，而是通过计算机网络与节点相连。比如TFS，GFS；

文件系统是操作系统用于明确存储设备或分区上的文件的方法和数据结构；

用户态文件系统是操作系统提供了一层文件存储的接口，方便用户对文件操作的接口；



**FUSE**：是一个实现在用户空间的文件系统框架，通过FUSE内核模块的支持，使用者只需要根据fuse提供的接口实现具体的文件操作就可以实现一个文件系统



##### Fuse的组成以及功能实现

FUSE由三个部分组成，FUSE内核模块、用户空间库libfuse以及挂载工具fusermount

- FUSE内核模块：实现了和VFS的对接，还是实现了一个可以被用户空间进程打开的设备，当VFS发来文件操作请求之后，它将该请求转化为特定格式，并通过设备传递给用户空间进程，用户空间进程在处理完后，将结果返回给FUSE内核模块，将其还原为Linux Kernel需要的格式，返回给VFS；
- fuse库libfuse：负责和内核空间通信，接收来自/dev/fuse请求，并将其转化为一系列函数调用，将结果写入/dev/fuse；提供的函数对可对fuse文件系统进行挂载卸载，从内核读取请求以及发送响应到内核；提供了两个APIs：一个“high-level”同步API，一个“low-level”异步API；都从内核接收请求传递到主程序（fuse_main函数），主程序使用响应的回调函数继续处理。
- 挂载工具：实现对用户文件系统的挂载



<img src="..\pic\linux_kernel_fuse.png" alt="linux_kernel_fuse" style="zoom:75%;" />





![linux_fuse_module](..\pic\linux_fuse_module.png)

##### 文件系统分3类：

- 分布式文件系统：gfs/hdfs/fastdfs，解决网络存储问题，纯引用服务器
- vfs：虚拟文件系统
- ex3/ex4/fat32/ntfs，操作系统自带，解决磁盘存储（FUSE属于这一类，在内核中）

<img src="..\pic\linux_fuse_create.png" alt="linux_fuse_create" style="zoom:75%;" />

Q1: **既然在用户态实现文件系统，那么create代码怎么能在内核空间呢？**

fuse_create_open只是为了对接vfs，真正的处理代码仍然在用户空间实现。

Q2: **目前已在内核空间，如何在用户空间处理？**

简单讲，在fuse_create_open内会创建一个包含FUSE_CREATE操作数的消息，将消息通过管道文件发送给管道另一端的用户态接收进程。

该进程在ZFUSE挂载时由libfuse库代码中创建，作用是读取管道文件消息并根据消息的操作数来执行对应操作，在这里解析到的是FUSE_CREATE操作数，对应libfuse库中的fuse_lib_create函数。





##### libfuse（.tar.gz开发包） 与 内核fuse（具体fuse实现） 如何通信？

通过`/dev/fuse`模块：（设备文件，用户空间和内核空间沟通的桥梁）





##### fuse运行的时候，由四部分组成：

- 对文件系统进行操作，cat/mkdir/touch
- 操作的虚拟文件系统，vfs
- fuse模块实现
- libfuse实现的应用程序



##### fuse解决了什么问题？内核为什么会提供Fuse？

implementing filesystems in user space，在用户空间实现文件系统



fuse解决了文件系统必须在内核的难题，将文件系统的实现从内核态搬到了用户态，也就是我们可以在用户态实现一个文件系统。

fuse实现了一个对文件系统访问的回调。分为内核态模块和用户态的库。

用户态的库为程序开发提供接口，也是我们实际开发时用的接口，通过这些接口将请求处理功能注册到fuse中；

内核态fuse模块时具体的数据流功能实现，它截获文件的访问请求，然后调用用户态注册的函数进行处理。











##### mount如何实现



##### 结构体函数指针赋值技巧

```c
// 定义结构体
struct operations = {
    int (*read) (const char *);
    int (*open) (const char *);
    int (*read) (const char *);
};

int my_open(const char * ch) {
    return 0;
}

// 申明结构体实例，并赋值
// 通过.拿到结构体申明的函数指针，并赋与我们的实现
// 对于不需要使用的函数指针不需要可以不处理
struct operations my_operations = {
    .open = my_open,
};

int main() {
    my_operations.open("hello");
    // my_operations.my_open("hello");  未定义
    // my_operations.read("hello"); segmentation 段错误
    return 0;
}
```



[参考](https://blog.csdn.net/ty_laurel/article/details/51685193)
























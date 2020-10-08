线程池，无锁CAS，内存管理与ringbuffer设计

**中间件使用**

MySQL 持久化存储，磁盘性能存储；

Redis 操作都在内存，速度快，用作缓存

1. 服务器同时向Redis和MySQL写入

```c
{
	write_to_redis();
	write_to_mysql();
}
```

2. 服务器先写mysql，再写redis，防止redis挂掉

```c
{
  write_to_mysql();
  mysql_sync_redis();  // 中间件
}
```

ZeroMQ  聊天软件服务器A 和服务器B间通信

MongoDB：

越近的数据访问越频繁，越久远的数据访问次数越少；

DFS文件系统

图片发送 ，把图片发送到DFS，（A只发图片url给B，B根据url去dfs文件系统取）

作业：互联网产品的功能划分与架构图？



**线程池**

用在哪里？

1. IO操作
2. 日志的存储（分级DEBUG，INFO）

如何实现？



如何封装成模块？



如何对一个线程池放缩?

线程多了，如何释放？

线程少了，如何扩充？



















**3.2 无锁CAS无锁队列**

compare and swap



mutex：共享区域运行时间比较长

spinlock：执行语句少，非阻塞

atomic：简单 数值的加减 



队列3个版本：

1. 线程非安全
2. mutex
3. 无锁

STL所有的容器都不是线程安全

ZeroMQ 用到无锁队列

CAS多线程同时竞争时候效率并不是特别高（能用mutex就不要用cas）



**每个CPU有独立的cache**，CPU1的cache中值变了，其他cpu的cache中对应的值应该失效。



关于代码：

a. 手撸代码（红黑树等

b. 阅读代码

c. 修改代码



项目：

实现，多跟老师沟通



课程设计思路：

1. 数据结构：排序，rbtree，btree，bloom过滤器，设计模式
2. 后台中间件：mysql/redis/mongodb/nginx/zeromq. (会用)
3. 基础组件开发：线程池，内存池，消息队列
4. 网络专题：select/poll/epoll, reactor, 同步异步，阻塞非阻塞，epoll，底层网络io实现
5. 开源组件：protobuf，xml，json，libevent，log
6. 代码工程化，git/svn, makefile/cmake
7. linux系统部分，性能分析
8. 云盘项目



架构师

1. 源码分析：nginx，redis，zeromq等（1. 数据结构+设计模式； 2. 网络与业务的处理）
2. 中间件开发专题
3. 集群：mysql集群，redis集群，zk
4. Linux内核部分
5. 性能优化
   1. mysql优化（基于索引）
   2. Linux系统调优（协议栈，网络io）
6. IM 即时通信项目





---

### **内存池原理与实现**

**什么是内存池，为什么要引入内存池？**

内存池是一个组件；

频繁的对内存进行malloc/free, 出现内存碎片；内存池可解决这个问题

**如何实现内存池？**

内存池为一个组件，提供api给其他人使用

- 结构体定义，基础数据结构
- 宏定义
- 对应函数，对外提供api
- 测试函数

内存池结构体：

如何把小块组织成一个4k？

- 小块内存：内存的叶为4k，< 4k;  提前分配好

- 大块内存：> 4k; 并不是提前分配好，并且不做释放

开源内存池：tcmalloc, malloc；

柔性数组：不占内存；用于内存大小不确定的情况，起到标签作用

```c
struct s {
  int a;
	struct s head[0]; // 不占内存
}
```

 内存池释放？

作业：

1. nginx 内存池实现原理
2. ringbuffer实现原理
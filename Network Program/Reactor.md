#### reactor

每个IO都对应可读可写的事件，都会触发epoll。

实现一个反应堆的封装；



1. 每一个io对应哪些属性
2. 大量IO如何存储?

reactor + 线程池



### libevent源码  ---> reactor

### redis源码 -->reactor



UDP的并发怎么做



- 服务器的并发量：服务器能够承载的客户端数量

- 吞吐量qps：单位时间内，能够处理请求数量



服务器能够承载这些fd(最基本条件)：

1. 5w以上的响应请求
2. 对数据库的操作
3. 磁盘的操作
4. CPU占用率  <60%
5. 内存占用率  <80%

reactor.c 能否同时承载100w的fd？



cat /proc/sys/net/ipv4/tcp_wmem

cat /proc/sys/fs/file-max

1. 操作系统fd的限制
2. file-max  : `sysctl -a | grep file-max`

3. Segment fault core dumped
4. 端口问题 



fd , socket, 网络io



send(fd, )， 怎么通过fd把数据发出去？通过fd找到对应的五元组发送。

fd --> tcp **五元组** (sip, dip , sport, dport, proto) 标识









一个网络IO同一时刻只有一个事件，



**socket：Too many open files**

解决：

查看： ulimit -a 查看fd上限  字段  "file descriptors" or "open files"

修改：ulimit -n fdnum





客户端与服务器，进入一个”阻塞“状态

1. 随机端口满了, 1024 - 65535
2. fd上限

解决： `nf_conntrack_max`的限制，限制客户端和服务端建立连接的第三次握手的限制。

​			线上是没有这个问题的，因为每个客户端IP地址不一样。





如何将客户端，每1000个链接的耗时变短，让服务器每秒可接入的数量更多？





- 多个accept放在一个线程
- 多个accept分配在不同的线程
- 多个accept分配在不同的进程 --> nginx



多线程与多进程的区别？

1. 多进程不需要加锁
2. fd上限比多线程要多
3. 多进程要求fd之间无关联





关于网络IO操作

1. accept
2. send
3. recv
4. close

epoll 多线程， 网络IO的模型？

![image-20201011172443862](C:\Users\kangzhongrun\AppData\Roaming\Typora\typora-user-images\image-20201011172443862.png)



C10k(1w并发) --> c1000k(1百万并发) -->c10M

影响服务器性能的因素

1. cpu
2. 内存
3. 网络
4. 操作系统
5. 应用程序

epoll 突破c10k  





网卡 --> 协议栈 --> 应用程序



绕过协议栈，直接从网卡到应用程序，即通过mmap, 实现零copy。



`ntytcp`



select/poll/epoll --> reactor --> 并发量

​	http








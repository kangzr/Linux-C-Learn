skynet，redis，nginx，定时器，openresty



##### 多核开发skynet，nginx：

都是多核开发，skynet采用多线程，nginx采用多进程(进程锁)



![image-20201201090711879](C:\Users\kangzhongrun\AppData\Roaming\Typora\typora-user-images\image-20201201090711879.png)

![image-20201201090749535](C:\Users\kangzhongrun\AppData\Roaming\Typora\typora-user-images\image-20201201090749535.png)

spin为衰减因子，先用原子操作获取锁，仍旧没有获取到锁，就采用互斥锁；

高并发场景，修改衰减因子为uint_max，尽量让其使用自旋锁



skynet使用自旋锁，使用原子操作`__sync_lock_test_and_set(&lock->lock, 1)`实现自旋锁

spinlock VS mutex

场景上：

1. 明确知道粒度小，使用spinlock
2. 明确知道粒度大，使用mutex
3. 介于两者之间看功能，高并发（工作进程或线程）处理选择spinlock，spinlock和mutex并存



nginx进程锁是自旋锁和互斥锁共存



##### redis（文件事件，时间事件），nginx：单线程处理，定时器

timeout = 最近发生的定时器 - 当前时间



##### nginx开发 可扩展：模块开发，过滤器，handle，upstream，slab(共享内存的封装，多进程共享数据)

lua-nginx-module, cosocket实现openresty的两个重要模块

每一个虚拟主机server有一个lua vm

![image-20201201093810213](C:\Users\kangzhongrun\AppData\Roaming\Typora\typora-user-images\image-20201201093810213.png)

`init_by_lua` `init_worker_by_lua`，将lua虚拟机嵌入到每个阶段，



负载均衡：哈希，



openresty短连接和长连接的应用



haproxy   -->   nginx



lua开发 c模块

1. c调lua 每协程一个栈，lua调c 每次调用一个栈



##### openresty应用

1. 数据库db









---

[OpenResty官网](http://openresty.org/cn/)

OpenResty基于Nginx与Lua的**高性能Web平台**，其内部集成了大量精良的Lua库，第三方模块以及大多数的依赖项，用于方便地搭建能够处理**超高并发**，**扩展性极高**的动态Web应用、Web服务和动态网关。

OpenResty通过汇聚各种设计精良的Nginx模块，从而将Nginx有效地变成一个强大的通用Web应用平台。这样，Web开发人员和系统工程师可以使用Lua脚本语言调用Nginx支持的各种C以及Lua模块，快速构造出足以胜任**10k乃至1000k以上单机并发连接**的高性能Web应用系统。

OpenResty的目标是让你的Web服务直接跑在Nginx服务内部，充分利用Nginx的非阻塞I/O模型，不仅仅对HTTP客户端请求，甚至于对远程后端诸如MySQL、PostgreSQL，Memcached以及Redis等都进行一致的高性能响应。












































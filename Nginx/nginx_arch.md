#### 代码结构

Nginx源码主要分布在src/目录下，而src/目录下主要包含三部分比较重要模块：

- core：包含了Nginx的最基础的库和框架，包括内存池，链表，hashmap，string等常用数据结构
- event：事件模块，Nginx自己实现了事件模型；Memcached使用Libevent事件库；
- http：实现http模块，实现了各种http的具体协议的各种模块

[Nginx源码网址](https://blog.csdn.net/yangyin007/article/details/82777086)

#### 进程结构

Nginx是一款多进程的软件，启动后，会产生一个master进程和N个工作进程；其中nginx.conf中可以配置工作进程个数

![process](D:\MyGit\Linux-C-Learn\pic\nginx_process.png)

`worker_process 1;`

多进程模块有一个非常大的好处：不需要太多考虑并发锁问题；Memcached采用多线程模型；

#### Nginx模块设计

高度模块化的设计是Nginx的架构基础。Nginx被分解为多个模块，每个模块就是一个功能模块，只负责自身功能，模块之间严格遵循“高内聚，低耦合”原则

![](D:\MyGit\Linux-C-Learn\pic\nginx_module.png)






















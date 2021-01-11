《操作系统设计与实现》《操作系统概念》了解各种同步原语，临界区，竞态条件，死锁，典型的IPC问题。

《redis设计与实现》



src目录

- core : 基础数据结构
- event : 事件处理
- os: 系统相关/支持跨平台
- http: http模块实现
- mail: 邮件
- upstream: 流



Nginx http流程：

ngx_http.c. : nix_command_t ;

ngx_string("http")   --> ngx_http_block ---> ngx_



nginx同时监听多个端口，是多个进程还是一个进程？

一个进程listen所有的端口

那么客户端调用connect，比如4个进程，如何确定是哪一个，惊群问题(所有进程都被唤醒，只有一个进程成功)

惊群缺点：损耗性能，对系统资源的一种浪费



三种惊群：

1. accept ：已解决
2. epoll_wait
3. pthread_cond_wait：已解决



如何解决？

 加锁ngx_trylock_accept_mutex，ngx_accept_mutex



strace可以跟踪有多少系统调用



ngx_http_wait_request_handler 进入http状态机



ngx_http_core_run_phases. 状态机运行接口



11个状态，一个状态对应一个handler链表，所有的handler处理完，phase++，进入下一个状态



http如何发送数据：ngx_http_top_header_filter -->  



过滤器模块跑起来



思维导图



不扩容情况：redis持久化不会进行扩容；即时hash表满了

缩容情况：hash表小于10%



遍历方式：1. 全遍历：keys，hkeys；2.分布遍历scan，hscan



扩容、缩容以及渐进式rehash，如何保证不重复不遗漏？

00 10 01 11特定遍历规则保证。





源码阅读方式：

- 确定主题
- 构造环境
- log-debug
- 验证



zset有序集合，动态搜索结构，主要实现排序的功能（如排行榜）

指令：zadd，zscore，zrange



**跳表和红黑树的区别？**

- 结构的区别：

  跳表是多层的有序链表；红黑树是平衡二叉树

- 时间复杂度：

  跳表为平均/大概率时间复杂度为O(logn). [1-1/n^c] 数据量很大时才可达到

  红黑树时间复杂度就是o(logn)

- 功能区别：

  跳表支持范围查找zrange（O(logn)），红黑树不支持

- 实现区别：

  跳表实现比较简单，红黑树实现比较复杂；



**如何实现逼近理想跳表**

用概率的方式模拟理想跳表，达到期望值一致(上一层是下一层的一半节点)

第一层的每一个节点都有1/2的概率有第二层节点，有1/4的概率有第三层节点



zset-max-ziplist-entries：节点数量

zset-max-ziplist-value：key的长度

ziplist/zset选择的两个指标



object encoding rank





持久化，哨兵模式，lua事务，事件处理，dict



主从复制



cap理论，同时只能保证两个条件

cosistency-一致性

availability-可用性

p-分区容错性



- 读写分离：一致性要求不是很高时，master提供写功能，slave提供读

- master设置不落盘，slave进行持久化

bgsave --  rdb -- 存入内存



2.8版本之前

**持久化**(一般采用其他线程/进程来做)：

- 客户端请求命令放入环形缓冲区中，rdb文件发送slave

- slave接受rdb，存入内存

- master发送缓冲区的命令，slave以此处理（全量同步）

三种情况：

a. 新加入从节点 

b.主从连接故障 ：只需要将少量写操作同步给数据库就行

c. 从节点重启



2.8之后

- 记录环形缓冲区偏移量：psync 主运行id offset（增量同步）
- 如果运行id不一样，则进行全量同步
- offset不在缓冲区，也需要进行全量同步

优化：

1.项目会经常对节点进行重启（从数据库更换机器）linux scp；

2.可能出现主从切换



长连接服务器的主备方案

AB服务器（镜像服务器），实现高可用和容灾

A服务器记录所有的操作，同步给B服务器




























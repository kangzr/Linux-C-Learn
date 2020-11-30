> 普通开发者如果习惯于在架构师封装好的东西之上，只专注于业务开发，久而久之，在技术理解和成长上，就会变得迟钝甚至麻木；因此，架构师可能成为普通开发者的敌人，他的强大让你变成“温室的花朵 ”，一旦遇到环境变化就会不知所措；因此，需要深入理解系统、技术和框架背后的深层原理。

Redis（Remote Dictionary Service）；

缓存是Redis使用最多的领域，相比Memcached而言，更加易于理解、使用、控制

##### list（列表）

Redis底层存储不是一个简单的linkedlist，而是一个称之为quicklist的结构，

首先在列表元素较少的情况下，会使用一块连续的内存存储(ziplist，压缩列表)；它将所有的元素紧挨着一起存储，分配一块连续内存；

当数据量比较多的时候，才会改成quicklist；因为普通的链表需要的附加指针空间太大，会比较浪费空间，加重内存碎片化；

因此，redis将链表和ziplist结合起来组成了quicklist；也就是将多个ziplist使用双向指针串起来。既满足了快速插入删除性能，又不会出现太大的冗余空间；





##### 分布式锁---千帆竞发

使用setnx(set if not exists)指令，只允许被一个客户端占用，用完了再del；

```shell
redis> setnx lock:codehole true  # 加锁
# 但是如果这里服务器进程挂掉呢？也会造成死锁，因为setnx+expire不是原子操作，不能用redis事务来解决，因为expire依赖于setnx执行的结果，
# setnx没有抢到锁，expire也不会执行；
# Redis2.8之后加入set指令的扩展参数，使得setnx和expire可以一起执行：set lock:codehole true ex 5 nx；彻底解决分布式锁问题
redis> expire lock:codehole 5  # 给锁加个过期时间，防止客户端挂掉，未能即时释放锁
...  # 操作
redis> del lock:codehole  # 释放锁
```



##### 延时队列---缓兵之计

习惯于RabbitMq和kafka作为消息队列中间件，来给应用程序之间增加异步消息传递功能，这两个都是专业的消息队列中间件；

**异步消息队列**

Redis的list常用来作异步消息队列，使用rpush/lpush操作入队，使用lpop/rpop来出队列

![redis_aysnc_list](..\pic\redis_aysnc_list.png)

**队列空了怎么办？**

如果不停的pop，进行空轮询，不但会拉高客户端的CPU，redis的QPS也会被拉高，如果这样的空轮询客户端来几十个，Redis的慢查询可能会显著增多

因此采用sleep来解决这个问题，让线程睡一会儿；

**实现**

采用Redis的zset，将消息序列序列化成一个字符串作为zset的value，这个消息的到期处理时间作为score，然后多个线程轮询zset获取到期的任务进行处理，多个线程为了保障可用性，万一挂了一个线程，还有其它线程可继续；



##### 简单限流

限流算法再分布式领域是一个经常被提起的话题；

zset滑动窗口，窗口之外的数据被砍掉



##### 漏斗限流



##### 通信协议

RESP（Redis Serialization Protocol）：实现异常简单，解析性能极好


















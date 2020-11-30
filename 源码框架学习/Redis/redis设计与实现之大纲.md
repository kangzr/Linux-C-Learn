### 第一部分  数据结构与对象

Redis数据库里面的每个键值对(key-value pair)都是由对象(object)组成的，其中：

- 数据库键总使一个字符串对象(string object)
- 数据库键的值，可以是字符串对象，列表对象(list object)，哈希对象(hash object)，集合对象(set object)，有序集合对象(sorted set object)



### 第二部分 单机数据库的实现

- 介绍数据库原理，说明服务器保存键值对的方式，如果处理过期的键值对，以及如何删除
- RDB和AOF持久化
- 事件：文件事件和事件事件
- 客户端和服务端



### 第三部分 多机数据库的实现

- 复制repication
- 哨兵sentinel
- 集群cluster



### 第四部分 独立功能实现

- 发布订阅，PUBLISH，SUBSCRIBE, PUBSUB
- 事务，MULTI，EXEC,WATCH，对ACID性质的支持程度
- Lua脚本，EVAL，EVALSHA，SCRIPT LOAD
- 排序，SORT
- 二进制位数组，GETBIT，SETBIT
- 慢查询日志：对Redis创建和保存慢查询日志的方法
- 监视器
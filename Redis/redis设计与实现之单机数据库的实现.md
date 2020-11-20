### 第二部分  单机数据库的实现

#### 一. 数据库

```c
struct redisServer {
  	redisDb *db;  // 一个数组，保存着服务器中的所有数据库  
    aeEventLoop *el;  // 事件状态
    int port;  // TCP监听端口
    int tcp_backlog;  // listen backlog
    int dbnum;  // 服务器的数据库数量
    int aof_state;  // AOF状态 (开启/关闭/可写)
    sds a
};
```



##### 切换数据库

```c
// redis> SELECT 2  进行数据库切换

// 因为IO复用缘故，需要为每个客户端维持一个状态
// 多个客户端状态被服务器用链表连接起来
typedef struct redisClient {
    int fd;  // 套接字描述符
    redisDb *db;  // 记录客户端当前正在使用的数据库
    
}
```



##### 数据库键空间

Redis是一个键值对(key-value pair)数据库服务器，每个数据库都由一个redisDb结构表示。

```c
// redis database representation
typedef struct redisDb {
    dict *dict;  // 数据库键空间，保存着数据库中的所有键值对
    dict *expires;  // 键的过期时间，字典的value为过期事件 UNIX 时间戳
    dict *blocking_keys;  // 正在阻塞状态的键
    int id;  // 数据库号码
} redisDb;

// 添加键： redis> SET data "2020"
// 删除键： redis> DEL data
// 更新键： redis> SET data "2021"
// 获取键： redis> GET data
```



##### 设置键的生存时间或过期时间

通过EXPIRE命令或者PEXPIRE命令，客户端可以s或者us为数据中的某个键设置生存时间TTL，过期服务器会自动删除生存时间为0的键

```shell
redis> SET key value
redis> EXPIRE key 5
redis> GET key
redis> PERSIST key  # 移除过期时间
redis> TTL key  # 查看还剩多少时间过期，>=0 则未过期
```

SETEX可以设置一个字符串键的同时为其设置过期时间；



##### 过期键删除策略

###### 定时删除

设置键过期时间的同时，创建一个timer，当过期时间到则立即对键执行删除操作；

对内存友好，可保证过期键即时删除；

对CPU不友好，过期键较多时，删除过期键会占用大量CPU时间；在内存不紧张而CPU紧张情况下，将CPU时间用在删除和当前任务无关的过期键上，无疑对服务器的响应时间和吞吐量造成影响；

###### 惰性删除

放任键过期不管，每次从键空间获取键时，检查是否过期，过期则删除

###### 定期删除

每隔一段时间，对数据库进行一次检查，由算法决定检查哪些数据库，删除哪些过期事件





##### AOF、RDB和复制功能对过期键的处理

##### 数据库通知





#### 二. RDB持久化

#### 三. AOF持久化

#### 四. 事件

#### 五. 客户端

#### 六. 服务器


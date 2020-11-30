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

对CPU不友好，过期键较多时，删除过期键会占用大量CPU时间；在内存不紧张而CPU紧张情况下，将CPU时间用在删除和当前任务无关的过期键上，无疑对服务器的**响应时间和吞吐量**造成影响；

###### 惰性删除

放任键过期不管，每次从键空间获取键时，检查是否过期，过期则删除

对CPU友好，不会再删除其它无关的过期键上花费任何CPU时间

对内存不友好，过期键可能会一直不被删除，所占用内存也不会释放(内存泄漏)

###### 定期删除

每隔一段时间，对数据库进行一次检查，由算法决定检查哪些数据库，删除哪些过期事件

是一种折衷策略。其难点是确定删除操作的时长和频率，需根据实际情况而定



##### Redis采用的过期键策略

惰性删除和定期删除，两者配合使用，可以合理地在CPU和内存间进行平衡。

惰性删除如下，键存在则正常执行，键不存在或过期则按不存在情况执行

```c
// 
//db.c/expireIfNeeded 惰性删除
int expireIfNeeded(redisDb *db, robj *key) {
    mstime_t when = getExpire(db, key);  // 取出键过期时间
    mstime_t now;
    if (when < 0) return 0;  // 未过期
    if (server.loading) return 0;  // 服务器正在载入
    ...
    return dbDelete(db, key);  // 将过期键从数据库删除
}
```

定期删除策略如下，

- 每次运行时，从给一定数量的数据库中取出一定数量的随机键进行检查，并删除其中的过期键
- 全局变量current_db会记录当前activeExpireCycle函数检查的进度，并在下一次调用时，接着上一次的进度进行处理，如果上次是10，则这次11
- 所有数据库检查一边之后，current_db重置为0，开始新一轮检查

```c
// redis.c

// 每秒被调用server.hz次
int serverCron(struct aeEventLoop *eventLoop, long long id, void *clientData){
    databasesCron();  // 对数据库执行删除过期键，调整大小，主动和渐进式rehash
}

int activeExpireCycleTryExpire(redisDb *db, dictEntry *de, long long now) {
    long long t = dictGetSignedIntegerVal(de);  // 获取键过期时间
    if (now > t) {  // 过期
        
    } else {
        return 0;   // 未过期
    }
}
```



##### AOF、RDB和复制功能对过期键的处理

- 生成RDB文件：在调用SAVE或BGSAVE时，会先检查key是否过期，过期则不保存
- 载入RDB文件：在启动Redis时，如果开启了RDB功能，会进行以下操作
  - 主服务器，忽略过期键
  - 从服务器，保存所有键，主从同步时，从服务器数据库会被清空
- AOF文件写入：当过期键被惰性/定期删除后，程序会向AOF文件追加append一条DEL命令，来显示记录key被删除
- AOF重写：过滤过期键
- 复制，由主服务器统一控制删除
  - 主服务器删除一个过期键后，会显示向所有从服务器发送过一个DEL命令
  - 从服务器接到主服务器DEL命令后，才会删除

##### 数据库通知

客户端通过订阅给定的频道或模式，来获知数据库中键的变化，以及数据库中命令的执行情况

##### 数据库总结

- Redis服务器的所有数据库都保存在redisServer.db数组中，数据库数量由redisServer.dbnum属性保存
- 客户端通过修改目标数据库指针，让其指向redisServer.db数组中的不同元素来切换不同的数据库
- 数据库主要由dict和expires两个字典组成，其中dict字典负责保存键值对，expires负责保存键的过期时间
- 数据库由字典构成，所以对数据操作建立在字典操作上
- 数据库key总是一个字符串对象，valu可以是任意中Redis对象类型，比如字符串对象，哈希表对象，集合对象，列表对象，有序集合对象
- expires字典的key指向数据库中的某个键，value记录过期时间，UNIX时间戳
- Redis采用惰性+定期删除策略
- SAVE/BGSAVE产生的RDB文件不会包含已经过期的键
- 一个过期键被删除，服务器会追加一条DEL到现有AOF文件末尾
- 主服务器删除过期键后，向从服务器发送DEL命令



#### 二. RDB持久化

Redis是内存数据库，其数据库状态存储在内存里面，如果不存储到磁盘中，一旦服务器宕机，数据将丢失，因此需要持久化。

RDB文件是一个经过压缩的二进制文件，通过该文件可以还原数据库状态

![redis_rdb](..\pic\redis_rdb.png)

##### RDB文件的创建与载入

SAVE命令：阻塞Redis进程，直到RDB文件创建完毕，阻塞期间，停止服务

BGSAVE命令：fork一个子进程，由子进程负责创建RDB文件，父进程继续提供服务

![rdb_load](..\pic\rdb_load.png)

![redis_rdb_file](..\pic\redis_rdb_file.png)

REDIS： 部分载入时可快速检查是否为RDB文件

db_version：记录问价版本号

databases：保存任意多个非空数据库



#### 三. AOF持久化

RDB持久化通过保存数据中的键值对来记录数据库状态，AOF持久化时通过保存Redis服务器执行的写命令来记录数据库状态

![redis_aof](..\pic\redis_aof.png)

被写入的命令都是以Redis命令请求协议格式保存，纯文本格式；

##### AOF持久化实现

AOF持久化功能的实现可分为：追加，文件写入，文件同步三个步骤

##### AOF文件的载入与数据还原

##### AOF重写

##### AOF持久化总结

- AOF文件通过保存所有修改数据库的写命令请求来记录服务器的数据库状态
- AOF文件中的所有命令都以Redis命令请求协议的格式保存
- 命令请求会保存到AOF缓冲区，之后再定期写入并同步到AOF文件
- appendsync选项的不同值对AOF持久化功能的安全性以及Redis服务器的性能由很大的影响
- 服务器只要载入并执行保存再AOF文件中的命令，就可以还原数据库本来的状态
- AOF重写一个新的AOF文件，这个新的AOF文件和原有的文件所保存的数据库状态一眼给，但是体积更小



#### 四. 事件

Redis服务器是一个事件驱动程序，服务器需要处理以下两类事件：

- 文件事件(file event)：Redis服务器通过套接字与客户端(其它服务器)进行连接，而文件事件就是服务器对套接字操作的抽象，服务端与客户端(或其它服务器)的通信会产生相应的文件事件，而服务器则通过监听并处理这些文件事件来完成一系列**网络通信**操作
- 时间事件(time event)：Redis服务器中比如serverCron函数，需要再给定时间点执行，时间事件就是服务器对这类定时操作的抽象

##### 文件事件

Redis基于Reactor模式开发了自己的网络事件处理器，这个处理器成为文件事件处理器(file event handler)；

- 采用I/O多路复用(multiplexing)程序来同时监听多个套接字，并根据套接字目前执行的任务来为套接字关联不同的事件处理器
- 被监听的套接字准备好执行accept，read，write，close等操作，与操作相关的文件事件就会产生，file event handler就会调用之间关联好的回调函数来处理这些事件

I/O多路复用实现了高性能的网络通信模型，采用单线程让设计更见简单。



##### 文件事件处理器的构成

![redis_io_handler](..\pic\redis_io_handler.png)

I/O多路复用负责监听多个socket，并将所有产生事件的socket放到一个队列里(**单线程模式，不会产生并发问题**)，通过这个队列，以有序，同步，每次一个socket方式向文件事件分派器传送socket

![redis_io_queue](..\pic\redis_io_queue.png)



##### 单线程模型每秒万级别处理能力的原因

- 纯内存访问。数据存放在内存中，内存的响应时间大约是100纳秒，这是Redis每秒万亿级别访问的重要基础。
- 非阻塞I/O，Redis采用epoll做为I/O多路复用技术的实现，再加上Redis自身的事件处理模型将epoll中的连接，读写，关闭都转换为了时间，不在I/O上浪费过多的时间。
- 单线程避免了线程切换和竞态产生的消耗。
- Redis采用单线程模型，每条命令执行如果占用大量时间，会造成其他线程阻塞，对于Redis这种高性能服务是致命的，所以Redis是面向高速执行的数据库



**Redis多线程执行**

 网络IO事件的处理要顺序处理，读IO可以多线程一次性读到队列中，读满为止；网络IO事件的处理要按顺序处理，网络IO写事件可以多线程同时操作

redis6.0引入了多线程IO：多线程读取/写入，单线程执行命令

- 主线程接收客户端并创建连接，注册读事件，读事件就绪，主线程将读事件放到一个队列
- 主线程利用RR策略，将读事件分配给多个IO线程，然后主线程开始等待IO线程读取数据到缓冲区，并解析命令
- 主线程忙等待结束，单线程执行解析后的命令，将响应写入输出缓冲区
- 返回响应时，主线程RR，将写事件分配给多个IO线程，然后主线程忙等待



##### I/O多路复用程序的实现

select、epoll、evport和kqueue，Redis为每个I/O多路复用函数实现了相同的API，其底层可互换。默认epoll

单线程epoll：

优点：不存在锁的问题；避免线程间CPU切换

缺点：单线程无法利用多CPU；串行操作，某个操作出问题，会阻塞后续操作

![redis_io_multi](..\pic\redis_io_multi.png)



```c
// 事件类型：AE_READBLE / AE_WRIATABLE
// ae.h
#define AE_READBLE 1
#define AE_WRIATABLE 2

// API   ae.c
// 接收一个socketfd，一个事件类型，一个事件处理器，将给定socketfd的给定事件加入到IO多路复用程序的监听范围，并将事件与处理器关联
int aeCreateFileEvent(aeEventLoop *evetLoop, int fd, int mast, aeFileProc *proc, void *clientdata);  

// 将fd从mask指定的监听队列中删除
void aeDeleteFileEvent(aeEventLoop *eventLoop, int fd, int mask);

// 获取给定fd正在监听的事件类型
int aeGetFileEvents(aeEventLoop *eventLoop, int fd);

// 在给定ms内等待，直到fd变成可读，可写或异常
int aeWait(int fd, int mask,  long long milliseconds);
aeApiPoll;
aeProcessEvents;
aeGetAPiName;
```

##### 文件事件的处理器

- 连接应答处理器

  ```c
  // 用于对连接服务器监听套接字的客户端进行应答
  // networking.c 
  // 创建一个TCP连接处理器
  void acceptTcpHanlder(aeEventLoop *el, int fd, void *privdata, int mask);
  
  ```

  ![redis_io_connect](..\pic\redis_io_connect.png)

- 命令请求处理器

  ```c
  // networking.c 
  // 读取客户端的查询缓冲区内容
  void readQueryFromClient(aeEventLoop *el, int fd, void *privdata, int mask);
  ```

  ![redis_io_read](..\pic\redis_io_read.png)

- 命令回复

  ```c
  // networking 
  // 负责传送命令回复的写处理器
  void sendReplyToClient(aeEventLoop *el, int fd, void *private, int mask);
  ```

  ![redis_io_reply](..\pic\redis_io_reply.png)

启动流程

```mermaid
graph LR
A[start]-->B[initServerconfig]
B-->C[initServer]
C-->D[aeMain]
D-->E[eventLoop]
```

从下到上

anet：封装了一系列tcp socket操作，包括绑定地址和监听，创建与客户端连接，以及读写等操作；

evport：多路io复用层，redis是单线程架构，为解决多连接并发问题，采用I/O多路复用；封装了evport、epoll、kqueue和select四种io多路复用的方式。

ae：统一通过aeApiPoll来标记有读写的套接字变化。比如select有套接字可读写时，会标记ae模块中的事件管理器有待处理文本

networking：负责和客户端交互，包含创建客户端连接、处理命令和回复执行结果等操作，主要使用RESP协议

#### 事件机制

redis通过不断执行aeMain来处理客户端消息以及自身逻辑，即不断通过事件循环来完成读写任务需求；

redis事件主要分为：文件事件(文件描述符的读写操作)、时间事件(定时任务)

redis在网络模块上，封装了不同平台不同的I/O多路复用实现，隐藏其细节，方便上层的调用。通过IO多路复用，单线程处理多个客户端的请求，实现高并发的目的。



Redis提供两种基本事件：FileEvent和TimeEvent。前者基于OS的异步机制(epoll)实现的文件事件，后者Redis自己实现的定时器





##### 时间事件

- 定时事件
- 周期性事件

**实现**

服务器将所有时间事件放在一个无序链表中，每当时间事件执行器运行时，遍历整个链表，查找到达时间事件，并调用相应的事件处理器；



#### 五. 客户端

```c
// redis.h
// 多个客户端状态被服务器用链表连接起来
typedef struct redisClient {
    int fd;
    redisDb *db;  // 正在使用的数据库
    int dictid;  // 数据库id
    
} redisClient;
```



#### 六. 服务器

##### 命令请求的执行过程

##### serverCron函数

默认每隔100ms执行一次，工作主要包括更新服务器状态信息，处理服务器接收的SIGTERM信号，管理客户端资源和数据库状态，检查并执行持久化操作

##### 初始化服务器

初始化服务器状态， 载入服务器配置，初始化服务器数据结构，还原数据库状态，执行事件循环














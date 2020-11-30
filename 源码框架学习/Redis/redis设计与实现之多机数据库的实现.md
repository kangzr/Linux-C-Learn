### 第三部分  多机数据库的实现

#### 主从复制模式

PSYNC命令具有完整重同步和部分重同步

读写分离：主机负责写入，从机负责读出

```shell
bind 0.0.0.0
port 8001
logfile "8001.log"
dbfilename "dump-8001.rdb"
daemonize yes  # 后台运行
rdbcompression yes
# slaveof ip port  // 如果从节点，需要加上这个，ip为master节点
```



#### Sentinel哨兵模式

基于主从复制模式，主机宕机，会在若干个从机中选取一个作为主机）解决了高可用的问题

- 每10s一次的频率向被监视的主服务器和从服务器发送INFO命令，当主服务器下线，或者Sentinel正对主服务器进行故障转移时，Sentinel向从服务器发送INFO命令的频率改成1s一次

```shell
# 配置 sentinel.conf

port 19001
sentinel monitor mymaster 192.168.*.* 9002 2
```

##### 原理：

Redis Sentinel是Redis高可用的实现方案，Sentinel是一个管理多个Redis实例的工具，它可以实现对Redis的监控、通知、自动故障转移。

##### 主要功能：

Sentinel主要功能包括主节点存活检测、主从运行情况检测、自动故障转移(failover)、主从切换。Redis的Sentinel最小配置是一主一从。Redis的Sentinel系统可以用来管理多个Redis服务器，改系统可以执行以下四个任务：

- 监控：Sentinel会不断的检查主服务器和从服务器是否正常运行
- 通知：当被监控的某个redis服务器出现问题，sentinel通过API脚本向管理员或者其它的应用程序发送通知
- 自动故障转移：当主节点不能正常工作时，Sentinel会开始一次自动的故障转移操作，它会将与失效主节点是主从关系的其中一个从节点升级为新的主节点，并且将其它的从节点指向新的主节点。
- 配置提供者：在Redis Sentinel模式下，客户端应用在初始化时连接的是Sentinel节点集合，从中获取主节点的信息。

#### Redis Sentinel的工作流程

Sentinel是Redis的高可用性解决方案：

由一个或多个Sentinel实例组成的Sentinel系统可以监控任意多个主服务器，以及所有从服务器，并在被监视的主服务器进入下线状态时，自动被下线主服务器属下的某个从服务器升为新的主服务器，然后由新的主服务器代替已下线的主服务器继续处理命令请求。如下图：

<img src="..\pic\redis_sentinel.png" alt="redis_sentinel" style="zoom:75%;" />

Sentinel负责监控集群中的所有主，从Redis，当发现故障时，Sentinel会在所有的从中选一个成为新的主。并会把其余的从变为新主的从。同时那台有问题的旧主也会变为新主的从，因此旧主恢复，也不会恢复主的身份。

在Redis高可用架构中，Sentinel往往不是只有一个，而是3个或者以上。目的是为了让其更加可靠，毕竟主从切换角色这个过程毕竟复杂。

sentinel模式基本可满足一般生成需求，具备高可用性，当数据量过大到一台服务器存放不下的情况时，主从模式或sentinel模式就不能满足需求，这个时候需要对存储的数据进行分片，将数据存储到多个Redis实例中。



#### Cluster集群模式

```shell
# redis.conf 配置
cluster-enabled yes  # 开启 
bind 0.0.0.0  
port 8001

# 开启指令
./redis-cli --cluster create 192.168.199.131:7001 192.168.199.131:7002 192.168.199.131:7003 --cluster-replicas 1
```



cluster的出现为了解决单机Redis容量有限的问题，将Redis的数据根据一定的规则分配到多台机器。

cluster可以说是sentinel和主从模式的结合体，通过cluster可以实现主从和master重选功能，所以如果配置两个副本三个分片的话，就需要6个Redis实例。

因为Redis数据是根据一定规则分配到cluster的不同机器的，当数据量过大，可新增机器进行扩容。

这种模式适合数据量巨大的缓存要求，数据量不是很大可使用sentinel。



##### Cluster原理

（redis官方提供，slot槽位，局域网内，节点之间互相ping通，自动定位节点，不再需要主机，（主从节点数据一样，key值一样），一般配置多个主节点和多个从节点）

Redis cluster是Redis的分布式解决方案，有效解决了Redis分布式方面的需求，自动将数据进行分片，每个master上放一部分数据。提供内置的高可用支持，部分master不可用时，还可继续工作。

支撑N个Redis master node，每个master node都可挂在多个slave node高可用，因为每个master都有slave节点，那么如果master挂掉，redis cluster这套机制，会自动将某个slave切换成master。支持横向扩容。

#### Redis cluster vs Replilcation sentinal

数据量少，缓存就几个G，可单机模式即可；

replication，一个master，多个slave，

redis cluster：针对海量数据，高并发，高可用的场景。





#### 五. 数据分布算法

##### 1. hash算法

hash(key) % N；将对key进行hash再对N取余数，N为redis个数

##### 2. 一致性hash算法

key落在圆环上后，会顺时针去寻找距离自己最近的节点，如果由一个master宕机，会导致很多请求全部打在db上，易出现缓存热点问题

##### 3. hash slot算法

Redis cluster固定16384（1024 * 16 = 16k）个hash slot，对每个key计算CRC16值，然后对16384取模，获得对应的hash slot；

Redis cluster中每个master都会持有部分slot，比如3个master，可能每个master持有5000多个hash slot

hash slot让node的增加和移除很简单，增加一个master，就将其它master的hash slot移动部分过去，减少一个master，就将它的hash slot移动到其它master上去。

移动hash slot的成本时非常低的。



#### 六. 节点间的内部通信机制

##### 1. 基础通信原理

- redis cluster节点间采取gossip协议进行通信

  跟集中式不同，不是将集群元数据（节点信息，故障等）集中存储再某个节点上，而是互相之间不断通信，保持整个集群所有节点的数据完整。

  集中式：元数据更新和读取实时性非常好，一旦元数据出现变更，立即更新到集中式的存储中，其它节点读取的时候立刻就能感知。缺点在于，所有元数据的更新压力全部集中再一个地方，可能会导致元数据存储由压力

  gossip：元数据更新比较分散，更新请求会陆续打到所有节点上更新，有一定的延时，降低了压力；缺点，元数据更新延时，可能导致集群的一些操作会有一些滞后。

- 10000端口

  每个节点都有一个专门用于节点间通信的端口，即服务端口号+10000

  每个节点每隔一段事件都会往另外几个节点返送ping信息，同时其它节点接收到ping后返回pong

- 交换信息

  故障信息，节点的增加和移除，hash slot信息等。

##### 2. gossip协议

gossip协议包含多种消息，包括ping，pong，meet，fail等

meet：某个节点发送meet给新加入的节点，让新节点加入集群中，然后新节点就会开始与其它节点进行通信

ping：每个节点会频繁给其它节点发送ping，其中包含自己的状态还有自己维护的集群元数据，互相ping通交换元数据。

pong：返回

fail：某个节点判断另一个节点fail之后，就发送fail给其它节点，通知其它节点，指定的节点宕机。 



#### 七. 高可用性与主备切换原理

redis cluster高可用的原理，和哨兵类似

##### 判断节点宕机

##### 从节点过滤

##### 从节点选举

##### 与哨兵比较

redis cluster非常强大，直接集成了replication和sentinal的功能
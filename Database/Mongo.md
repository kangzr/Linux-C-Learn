MongoDB: （NoSql）

MySQL:

Redis:



聊天记录，大量评论

越久远的数据，访问次数越少，放到访问越慢的磁盘中（内存 --> 磁盘）



mongodb：

写入的速度跟redis差不多

读出的速度跟mysql差不多



1. 博客存储（写的次数少，读的次数多）
2. 日志系统（写入次数多，读的次数少） --- MongoDB（集中式日志kafka: 消息队列, 接受数据 当日志很多时候kafka可以存储一部分，起到消峰作用 +  mongdb）
3. 一起写文档（读写一致，强实时）



canal从mysql同步到redis：从redis里面读，从mysql里面写







mongodb在启动时，需要指定文件存到什么地方。

`mongod --dbpath=db --bind_ip=0.0.0.0 --logpath=log/db.log &`



MongoDB集群

- replica set(副本集): 和主节点数据保持一致（用的比较多）
- master-slave：主机宕机后集群不能再工作，强耦合性（用的比较少）
- sharding：分片，一个shard存储数据的一部分，合起来为完整数据（用的比较多）







Mongo与其它数据库对比，优势何在？



RDBMS（关系数据库管理系统）与NoSQL（Not Only SQL）对比

**CAP理论**，指出对于一个分布式计算系统来说，不可能同时满足以下三点：

- 一致性(Consistency) : 所有节点同一时间具有相同的数据
- 可用性





分片Sharding：

每个Sharding带一个config(配置)；客户端只需连接mongos (起到对应route作用)，mongos找到对应的sharding；


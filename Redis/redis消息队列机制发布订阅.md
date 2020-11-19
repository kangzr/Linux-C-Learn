string hash 列表list 集合set 有序集合

抓包

`sudo tcpdump -i any dst host 127.0.0.1 and port 6379`

缓存热门图片

存储文章



散列hash和字符串string的选取

字符串功能更丰富

hash的过期时间只能针对整个key，无法针对key中某个field



列表list：秒杀，分页



multi 开启事务，为全局事务



Redis缓存怎么设计

MySQL持久化怎么设计



Redis集群

- 单机模式，只适合做调试
- 主从模式，（主机负责写入，从机负责读出）
- sentinel模式（基于主从复制模式，主机宕机，会在若干个从机中选取一个作为主机）解决了高可用的问题
- cluster模式：redis官方提供，slot槽位，局域网内，节点之间互相ping通，自动定位节点，不再需要主机，（主从节点数据一样，key值一样），一般配置多个主节点和多个从节点



主从复制，除了sentinel模式，还有没有其它方案可做到高可用？

lvs + keepalived

vip：虚拟ip





hash slot算法，crc16



c/c++如何连接集群？



hiredis

memcached
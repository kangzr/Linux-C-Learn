### 第四部分  独立功能的实现

#### 发布与订阅

##### 频道的订阅与退订

SUBSCRIBE，UNSUBSCRIBE

```c
struct redisServer {
    dict *pubsub_channels;  // 频道
    list *pubsub_pattners;  // 模式的订阅与退订 PSUBSCRIBE PUNSUBSCRIBTE
};
```

##### 发送消息

PUBSLISH

##### 查看订阅信息

##### 总结

- 服务器状态在pubsub_channels字典中保存了所有频道的订阅关系：SUBSCRIBE命令负责将客户端的被订阅的频道关联到这个字典，UNSUBSCRIBTE退定
- 服务器状态在pubsub_patterns链表中保存了所有模式的订阅关系：PSUSBSCRIBE/PUNSUBSCRIBTE
- PUBLISH命令通过访问pubsub_channels/pubsub_patterns，来向频道/模式的所有订阅者发送消息
- PUBSUB命令的三个子命令都可以通过读取pubsub_channels/pubsub_pattners的信息来实现



#### 事务

Redis通过MULTI，EXEC，WATCH等命令来实现事务transcation，事务提供了一种将多个命令请求打包，然后一次性、顺序执行多个命令的机制。并且在事务执行期间，服务器不会中断事务而改去执行其它客户端的命令请求，它会将事务中的所有命令都执行完毕，才回去处理其它客户端请求。

```shell
redis> MULTI
redis> COMMANDs
redis> EXEC 执行
```

##### 事务的实现

- 事务开始：

  ```shell
  redis> MULTI  # 切换至事务状态，打开REDIS_MULTI标识
  ```

- 命令入队：

  ```shell
  # 当一个客户端处于非事务状态时，客户端发送的命令会立即被服务器执行
  ```

- 事务队列：

  ```c
  typedef struct redisClient {
      multisate mstate;  // MULLT/EXEC state
  } redisClient;
  
  typedef struct multistate {
      multiCmd *commands; // 事务队列， FIFO
      int count;
  } mutliState;
  ```

- 执行事务

  ```shell
  # 服务端收到EXEC命令，会遍历这个客户端的事务队列，执行队列中保存的所有命令，最后将执行结果返回给客户端
  redis> EXEC
  ```

##### WATCH命令的实现

乐观锁(optimistic locking)，可在EXEC前，监视任意数量的数据库键，并在执行EXEC命令执行时，检查被监视的键是否至少有一个已经被修改过了，如果是的话，拒绝执行事务，并向客户端返回代表事务执行失败的空回复

```c
// 每个redis数据库中都保存一个watched_keys字典，这些字典的值是一个链表，记录所有监视相应数据库键的客户端
typedef struct redisDb {
    dict *watched_keys;   // 正在被WATCH命令监视的键
}redisDb;
```





#### Lua脚本

#### 排序

#### 二进制位数组

#### 慢查询日志

#### 监视器






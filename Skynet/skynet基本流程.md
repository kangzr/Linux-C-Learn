skynet为事件驱动运行，由socket和timeout两个线程驱动



- 服务句柄handle：服务的唯一标识，对应一个skynet_context，所有的skynet_context存放在handle_storage中

```c
// skynet_handle.c
struct handle_storage {
	struct rwlock lock;
    uint32_t harbor;		//集群ID
    uint32_t handle_index;	// 当前句柄索引
    int slot_size;			// 槽位数组大小
    struct skynet_context ** slot;	// skynet_context数组
    ...
};
```

- 服务模块skynet_context: 一个服务对应一个skynet_context，可理解为一个隔离环境(类似进程)
- 消息队列message_queue：消息队列链表，一个服务对应一个消息队列，所有的消息队列链接起来构成全局消息队列





##### 服务间消息通信

每一个服务都有一个独立的lua虚拟机(skynet_context)，逻辑上服务之间相互隔离，在skynet中服务之间可以通过skynet的消息调度机制来完成通信。

skynet中的服务基于actor模型设计出来，每个服务都可以接收消息、处理消息、发送应答消息

每条skynet消息由6部分构成：消息类型，session，发起服务地址，接收服务地址，消息c指针，消息长度



session能保证本服务中发出的消息是唯一的。消息于响应一一对应
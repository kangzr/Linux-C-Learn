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






QUESTION？

**Redis有哪些数据结构？**

**使用过Redis分布式锁么，它是什么回事？**

**假如Redis里面有1亿个key，其中有10w个key是以某个固定的已知的前缀开头的，如果将它们全部找出来？**

**使用过Redis做异步队列么，你是怎么用的？**

**如果有大量的key需要设置同一时间过期，一般需要注意什么？**

**Redis如何做持久化的？**

**Pipeline有什么好处，为什么要用pipeline？**

**Redis的同步机制了解么？**

**是否使用过Redis集群，集群的原理是什么？**



Redis是一个Key-Value存储系统；

Redis将数据存储于内存中，或被配置为使用虚拟内存，这两种方式都可以实现**数据持久化**。

提供基于TCP的协议以及操作丰富的数据结构

定位于一个内存数据库，正因为内存的快速访问特性，才使得Redis能够有如此高的性能，能轻松处理大量复杂数据结构



---

### Redis简介

Redis是一个开源的，使用ANSI C语言编写、遵循BSD协议、支持网络、可基于内存亦可持久化的日志型、Key-Value数据库，并提供多种语言API；

具有以下优势：

- 性能极高：Redis纯内存读写，读的速度110,000次/s，写的速度81000次/s
- 丰富的数据类型：Redis支持二进制案例的String、List、Hash、Set及Ordered Set数据类型操作
- 原子：Redis的所有操作都是原子性的，单个操作是原子性，多个操作也支持事务/原子性，通过MULTI和EXEC指令包起来
- 丰富的特性：Redis还支持publish/subscribe、通知、key过期等特性



### Redis基础数据结构

Redis对象

```c
typedef struct redisObject {
    unsigned type:4;  // 类型
    unsigned encoding:4;  // 编码
    unsigned lru:REDIS_LRU_BITS;  // 对象最近一次被访问的时间
    int refcount;  // 引用计数
    void *ptr;  // 指向实际值的指针
}robj;
```



5种基础数据结构：string(字符串)，list(链表)，set(集合)，hash(哈希)，zset(有序集合)



#### string(字符串)

并没有直接使用c语言传统的字符串，而是构建了简单动态字符串(simple dynamic string, SDS)的抽象类型；

除了用来保存数据库中的字符串值外，SDS还被用做缓冲区(buffer): AOF模块中的AOF缓冲区以及客户端状态中的输入缓冲区。

```c
// sds.h/sds.c
struct sdshdr {
	int len;	// buf中已占用空间的长度，'\0'不计入
    int free;	// buf中剩余可用空间长度
    char buf;	// 数据空间，以'\0'结尾，便于直接重用C字符串函数库中的函数
};
```



##### SDS与C字符串的区别：

**strlen获取字符串长度**: C字符串时间复杂度为O(N)；SDS记录了字符串长度，时间复杂度为O(1)

**缓冲区溢出**：C需要手动为字符串分配空间，容易产生缓冲区溢出；SDS自动调整空间大小，杜绝溢出。

**SDS内存分配策略**：

缓冲区溢出：字符串拼接时导致（忘记重新增加内存）；

内存泄漏 ：字符串截断操作导致（忘记释放掉不再使用的内存）

针对以上两个问题，SDS提供两种内存分配优化策略如下：

###### SDS空间预分配策略

```c
//sds.c
// 对sds中的buf长度进行扩展， 时间复杂度T=O(N)
sds sdsMakeRoomFor(sds s, size_t addlen) {
    struct sdshdr *sh, *newsh;
    size_t free = sdsavail(s);
    size_t len, newlen;
    // 空间足够，直接返回
    if(free >= addlen) return s;
    // 获取s目前已占用空间长度
    len = sdslen(s);
    sh = (void*) (s-(sizeof(struct sdshdr)));
    // s最少需要长度
    newlen = (len+addlen);
    // 计算所需分配的空间
    // 小于1M，则分配两倍；大于1M, 则多分配1M
    if (newlen < SDS_MAX_PREALLOC)
        newlen *= 2;
    else
        newlen += SDS_MAX_PREALLOC;
    newsh = zrealloc(sh, sizeof(struct sdshdr)+newlen+1);
    if(newsh == NULL) return NULL;
    newsh->free = newlen - len;
    return newsh->buf;
}
```

###### SDS惰性空间释放策略

删除字符后，不会释放对应的内存。sdstrim



#### list(链表)

```c
// adlist.h/adlist.c
// 双端链表节点
typdef struct listNode {
    struct listNode *prev;
    struct listNode *next;
    void *value;
} listNode;

typedef struct listIter {
    listNode *next;
    int direction;
} listIter;

typedef struct list {
    listNode *head;
    listNOde *tail;
    void *(*dup)(void *ptr);
    void (*free)(void *ptr);
    void (*match)(void *ptr, void *key);
    unsigned long len;
} list;
```

- 提供了dup函数用于复制链表节点所保存的值
- free函数用于释放链表节点所保存的值
- match函数则用于对比链表节点所保存的值和另一个输入值是否相等

无环链表

常用来做异步队列使用，底层存储为一个quicklist结构；

元素较少时使用一块连续内存存储(ziplist，压缩列表)，将所有元素紧挨着一起存储，分配一块连续内存。

元素较多时会改成quicklist   

Redis链表实现的特性：

- 双端：链表节点带有prev和next指针，获取节点前置/后置节点的复杂度为O(1)
- 无环：头节点的prev指针，尾节点的next指针都指向NULL，因此对链表的访问以NULL
- 带表头指针和表尾指针：通过list结构的head和tail指针，可在O(1)时间获取头尾节点
- 带链表长度计数器：属性值len可以O(1)时间获取链表长度
- 多态：使用void*指针来保存节点值，可通过list结构的dup、free、match为节点设置类型特定函数



#### 字典

使用哈希表作为底层实现

```c
// dict.h/dict.c
typedef struct dictht {
    dictEntry **table;  // 哈希表数组
    unsigned long size;  // 哈希表大小
    unsigned long sizemask;  // 哈希表大小掩码，用于计算索引值，总是等于size - 1 和hash值一起确定一个key放到table哪个索引上
    unsigned long used;  // 该哈希表已有节点的数量
}dictht;
```



将一个新的键值对添加到字典里面时，需要根据键值对的键计算出哈希值和索引值，

然后根据索引值，将包含新键值对的哈希表节点放到哈希表数组的指定索引上。





数组+链表二维结构(开链方法解决冲突)；

Rehash很耗时，为了高性能，采取渐进式rehash策略。

**渐进式rehash**：在rehash同时，**保留新旧两个hash结构**，查询时会同时查询两个hash结构，

然后在后续定时任务中以及hash操作指令中，循序渐进地将旧hash的内容一点点迁移至新的hash结构。搬迁完再取而代之。

最后一个元素移除后，数据结构自动被删除，内存被回收。



##### hashes类型及操作

Redis hash 是一个string类型的field和value的映射表。添加、删除操作都是O(1)。



#### 有序集合zset

分为ziplist或者skiplist；



##### 压缩列表ziplist

为节约内存，由一系列特殊编码的连续内存块组成的顺序型数据结构,是列表键和哈希键的底层实现之一；



##### 跳跃表skiplist

skiplist是一种有序数据结构，通过在每个节点中维持多个指向其它节点的指针，从而快速访问节点。

支持平均O(logN), 最坏O(N)复杂度的节点查找，还可通过顺序性操作来批量处理节点

在大部分情况下， 跳表的效率可以和平衡树相媲美， 并且因为跳表的实现比平衡树要来得更为简单， 所以有不少程序都使用跳表来代替平衡树。

调表解决的问题是实现对链表的二分查找，牺牲空间换取时间，如果n个元素的链表，需要额外n/2 + n/4 + ... + 2 = n - 2个索引节点。

支持功能

- 插入元素
- 删除元素
- 查找元素
- 查找区间元素
- 输出有序序列

为何redis选择跳表而不是红黑树？

- 红黑树在查找区间元素的效率没有跳表高，其它操作时间复杂度一致
- 相比红黑树，跳表实现比较简单，易读，易维护
- 跳表更加灵活，通过改变索引构建策略，有效平衡效率和内存消耗

zset应用场景

分布式限流

实时排行榜

优先级消息队列



**丰富的编码格式的好处？**

通过 encoding 属性来设定对象所使用的编码， 而不是为特定类型的对象关联一种固定的编码， 极大地提升了 Redis 的灵活性和效率， 因为 Redis 可以根据不同的使用场景来为一个对象设置不同的编码， 从而优化对象在某一场景下的效率。

举个例子， 在列表对象包含的元素比较少时， Redis 使用压缩列表作为列表对象的底层实现：因为压缩列表比双端链表更节约内存， 并且在元素数量  较少时， 在内存中以连续块方式保存的压缩列表比起双端链表可以更快被载入到缓存中；
但是，随着列表对象包含的元素越来越多， 使用压缩列表来保存元素的优势逐渐消失时， 对象就会将底层实现从压缩列表转向功能更强、也更适合保存大量元素的双端链表上面。

**SDS优势，为什么不直接用C字符串？以及SDS如何扩容**

获取一个 C 字符串的长度， 程序必须遍历整个字符串， 对遇到的每个字符进行计数， 直到遇到代表字符串结尾的空字符为止， 这个操作的复杂度为 O(N) ，和 C 字符串不同， SDS 在 len 属性中记录了本身的长度， 所以获取一个 SDS 长度的复杂度仅为 O(1)，通过使用 SDS 而不是 C 字符串， Redis 将获取字符串长度所需的复杂度从 O(N) 降低到了 O(1) ， 这确保了获取字符串长度的工作不会成为 Redis 的性能瓶颈。

设置和更新 SDS 长度的工作是由 SDS 的 API 在执行时自动完成的， 使用 SDS 无须进行任何手动修改长度的工作。

**为什么跳表要用压缩列表指向头指针？**

但通过使用一个 zskiplist 结构来持有这些节点， 程序可以更方便地对整个跳表进行处理， 比如快速访问跳表的表头节点和表尾节点， 又或者快速地获取跳表节点的数量（也即是跳表的长度）等信息。

**新增数据时，如何确定跳表节点多少层？**

每次创建一个新跳跃表节点时，根据幂次定律（经济学家提出，越大的数出现的概率越小），随机生成一个介于1和32之间的值作为level的数组大小，这个大小就是层高度

**如何在遍历跳表时快速得到当前元素排名？**

遍历过程中所有的跨度(span)之和即为当前rank

**zset何时使用ziplist何时使用skiplist编码？**

当有序集合对象可以同时满足以下两个条件时，对象使用ziplist编码，否则使用skiplist

- 有序集合保存的元素数量小于128（可配置修改）
- 有序集合保存的所有元素成员的长度都小于64字节

**为什么有序集合需要同时使用跳表和hash来实现？**

hash：O(1)查找，还是无序，排序至少需要O(nlogn)

跳表：查找为O(n)，但是有序




















































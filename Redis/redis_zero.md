redis  kv数据库  内存数据库

hash表

key --- 都存储为string   同通过hash函数得到整数得到hash表的索引；特征：有规律性的key，(hash)具备强随机分不性

value --- string list, set, zset, hash, stream

hash表 = 数组(uint) + hash函数 (md5, )

siphash函数（redis采用）

冲突：

- 抽屉原理：n+1个苹果放在n个抽屉里面，至少一个抽屉有两个
- 根据当前的数据量设置当前的数组，redis数组初始值为4

解决冲突：

- 数组+链表（redis采用），链表采用头插法(最近插入，也是最近要使用)
- 再hash法
- 两个数组
- 布隆过滤器（减少冲突）

hash表的扩容与缩容，需要重新映射rehash：

- 扩容：翻倍，(持久化不会进行扩容)，
- 缩容：当前数量小于数组长度的10%时需要进行缩容，防止扩容/缩容过于频繁

取余操作改成位运算，因为取余操作比较耗cpu：

(以4为例)    x % 4 == x & (3)    

采用**渐进式rehash**，因为redis为单线程，当数量特别大时，会阻塞线程，无法处理其它的请求

```c
typedef struct dict {
    ...
    dictht ht[2];  // hash[0] 扩容前  hash[1] 扩容后，  实现渐进式rehash
    long rehashidx;  //
    ...
}dict;
```

- 每次操作(增删改查)，rehash一次(`dictRehash(, 1)`，数组的槽位

- 如果没有持久化操作，那么根据1ms内操作多轮100次(`dectRehash(, 100)`)



hash结构遍历

`keys（redisdb的操作），hkeys()，scan，hscan，zscan`

尽量不要使用keys，hkeys，返回hash表所有的key，会耗时比较久，

scan/hscan

从高位开始加1，方便扩容

- 数组两次缩容后，会产生重复的数据
- 巧妙的利用扩容和缩容整好整数倍或者rehash操作，从而避免数据重复或者遗漏遍历



跳表`skiplist`

排行榜，动态搜索结构：边插入边排序；

红黑树、b树、堆、b+树(可以支持范围查找)

跳表支持范围查找

- 单链表：搜索时间复杂度O(n)

redis跳表层数增加原则：

第一层每一个节点有50%的概率有第二层

第二层有50%概率有第三层

如果数据量足够大，趋向于理想跳表结构

缺点：不适合少量数据  < 128

1-n^(1 ^ c)

`zset`数量小于128或者key长度小于64，使用压缩列表(字节数组)




























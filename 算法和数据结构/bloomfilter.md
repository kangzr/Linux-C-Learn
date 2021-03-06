

###  布隆过滤器

**核心问题**：判断元素是否在集合中；

**应用场景**：（上千万级别数据才有意义）；

- word文档单词拼写检查
- 网络爬虫，过滤相同`url`页面
- 垃圾邮件过滤算法如何设计？ 误判为黑名单，需要增加白名单（黑白hash种子不一样）
- 缓存崩溃后造成的缓存击穿？  过滤，缓存崩溃后，先去布隆过滤器取，如果有就不用再去读内存
- 名字是否在嫌疑人名单。
- 推荐系统，通过布隆过滤器去重
- 检测网络`ip`是否为新用户
- 新手机统计，每个手机都有唯一标识码，厂家，型号等。
- 屏蔽广告

> 某样东西一定不存在或者可能存在

**布隆过滤器与`hashmap`的区别？**

布隆过滤器不会存储key，hashmap会存储key，因此hashmap占用更多的内存。

hashmap 元素超过表的一半后就需要扩容，hash冲突太多会退化成链表，降低性能。

**为什么不用hashtable？**

哈希表存储效率一般只有50%，很废内存，hash冲突；



**hashmap和hashtable的区别**



**布隆过滤器数据结构：**

是一个bit向量或者说bit数组（最耗内存部分），增加key反而不会耗内存（因为根本不存储key）

![bloomfilter](..\pic\bloomfilter.png)



hash(key)后的值的表示范围会影响到hash冲突

- bit向量表的长度，对hash冲突影响非常大，越长冲突越低，也越占内存。
- 布隆过滤器使用多个hash函数去做映射，因此一个key可以通过多个hash来表示，即**多个hash函数对应的bit都置为1才算命中**
- 用多个hash是为了解决冲突的问题，
- hash是稳定的，同一hash，同一key，不管什么时候返回值都是一样的。

**优点**：占用内存少，查询效率高

**缺点**：结果是概率性的（存在误差），而不是确切的；不能删除，因为有些bit有冲突，如果删除会影响其他元素的命中。



**设计和应用布隆过滤器的方法：**

可用现成hash算法：`murmurhash`，双重散列

用户决定要增加的最多元素个数n，误差率P，hash种子，根据m和p算出m(bit长度)，再算出k(hash函数个数)





---

#### Hashtable哈希表

**哈希冲突解决策略：**

1. 开放寻址法:

- 线性探测
- 二次探测
- 双重hash/Rehashing

2. 开链法

**哈希函数设计：**

- 除法哈希法
- 乘法哈希法
- 全域哈希法

**完美哈希：**

尽量降低冲突的发生












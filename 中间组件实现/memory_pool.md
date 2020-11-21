### 内存池原理，以及为何使用内存池

分配一整块内存备用，每次需要内存则从这里取，而不需要再次直接向系统申请。

避免频繁的对内存进行`malloc/free`，减少内存碎片，防止内存泄漏

#### 一.  实现方法一

参照nginx内存池

![mm_pool](../pic/mm_pool.png)

##### 内存池实现

```c
// 对齐，找到下一个对齐的数值
// 比如aligment为2，11-->16
#define MP_ALIGNMENT	32  // 对齐方式
#define MP_PAGE_SIZE	4096 // 页大小
#define MP_MAX_ALLOC_FROM_POOL	(MP_PAGE_SIZE-1) // 
#define mp_align(n, alignment) (((n)+（alignment-1)) & ~(alignment-1))
#define mp_align_ptr(p, aligment) (void*)((((size_t)p)+(alignment-1)) & ~(alignment-1)))

// 结构体

// 大块内存结构体
struct mp_large_s {
    struct mp_large_s *next;
    void *alloc;
};

// 小块内存节点，没有小块内存构成一个链表
struct mp_node_s {
 	unsigned char *last;  // 下一次内存从此分配
    unsigned char *end;  // 内存池结束位置
    struct mp_node_s *next;  // 指向下一个内存块
    size_t failed;  // 改内存块/node分配失败的次数
};

// 内存池结构
struct mp_pool_s {
  	size_t max;  // 能直接从内存池中申请的最大内存，超过需要走大块内存申请逻辑
    struct mp_node_s *current;  // 当前分配的node
    struct mp_large_s *large;  // 大块内存结构体
    struct mp_node_s head[0];  // 柔性数组不占用大小，其地址为紧挨着结构体的第一个node
};

// 需要实现的接口
struct mp_pool_s *mp_create_pool(size_t size);  // 创建内存池
void mp_destory_pool(struct mp_pool_s *pool);	// 销毁内存池
void *mp_alloc(struct mp_pool_s *pool, size_t size);  // 分配内存 对齐？
void *mp_nalloc(struct mp_pool_s *pool, size_t size);  // 不对齐？
void *mp_calloc(struct mp_pool_s *pool, size_t size);  // 分配内存，并初始化为0
void mp_free(struct mp_pool_s *pool, void *p); // 释放p节点内存

// posix提供的malloc,calloc,realloc返回的地址都是对齐的，32位8字节, 64位16字节，不可调；所以有了posix_memalign
// 
// 介绍一个函数posix_memalign，类似malloc，分配的内存由free释放
// int posix_memalign(void**memptr, size_t alignment, size_t size)
// memptr: 分配好的内存空间的首地址
// alignment: 对齐边界，Linux中32位系统8字节，64位系统16字节，必须为2的幂
// size: 指定分配size字节大小的内存
// 发牛size字节的动态内存，这块内存为alignment倍数；

// 创建线程池，内配线程池整块内存
struct mp_pool_s *mp_create_pool(size_t size) {
    struct mp_pool_s *p;
  	// 分配内存池内存：mp_pool_s + mp_node_s + size
    int ret = posix_memalign((void**)&p), MP_ALIGNMENT, size + sizeof(struct mp_pool_s) + sizeof(struct mp_node_s));
    if (ret) {
        return NULL;
    }
    // 可从内存池申请的最大内存
    p->max = (size < MP_MAX_ALLOC_FROM_POOL) ? size : MP_MAX_ALLOC_FROM_POOL;
    p->current = p->head;  // 当前可分配的第一个节点mp_node_s
    p->large = NULL;
    // 下一个可分配的地址
    p->head->last = (unsigned char *)p + sizeof(struct mp_pool_s) + sizeof(struct mp_node_s);
    p->head->end = p->head->last + size;
    p->head->failed = 0;
    return p;
}

// 销毁线程池
void mp_destroy_pool(struct mp_pool_s *pool) {
    struct mp_node_s *h, *n;
    struct mp_large_s *l;
    // 销毁大块内存
    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            free(l->alloc);
        }
    }
    // 销毁小块内存
    h = pool->head->next;
    while (h) {
        n = h->next;
        free(h);
        h = n;
    }
    free(pool);
}

// 线程池重置
void mp_reset_pool(struct mp_pool_s *pool) {
		struct mp_node_s *h;
  	struct mp_large_s *l;
  	// 大块内存全部销毁
  	for (l = pool->large; l; l = l->next) {
      	if (l->alloc) {
        		free(l->alloc);
        }
    }
  	pool->large = NULL;
  	for (h = pool->head; h; h = h->next) {
      	// 每个节点的last位置重置
      	h->last = (unsigned char *)h + sizeof(struct mp_node_s);
    }
}


// 分配大块内存
static void *mp_alloc_large(struct mp_pool_s *pool, size_t size) {
    void *p = malloc(size);
    size_t n = 0;
    struct mp_large_s *large;
    for (large = pool->large; large; large = large->next) {
        if (large->alloc == NULL) {
            large->alloc = p; // 存入large链表中
            return p;
        }
        if (n ++ > 3) break;  // ???只遍历三次？
    }
    large = mp_alloc(pool, sizeof(struct mp_large_s));
    if (large == NULL) {
        free(p);
        return NULL;
    }
    large->alloc = p;
    large->next = pool->large;
    pool->large = large;
    return p;
}

// 分配新的内存块/node: mp_node_s + psize
static void *mp_alloc_block(struct mp_pool_s *pool, size_t size) {
    unsigned char *m;
    struct mp_node_s *h = pool->head;
    size_t psize = (size_t)(h->end - (unsigned char *)h);  // 第一个内存块大小
    // 分配psize大小的内存块
    int ret = posix_memalign((void **)&m, MP_ALLIGMENT, psize);
   	if (ret) return NULL;
    
    struct mp_node_s *p, *new_node, *current;
    new_node = (struct mp_node_s*)m;  // 新的节点
    new_node->end = m + psize;  // end位置
    new_node->next = NULL; 
    new_node->failed = 0;
    
    m += sizeof(struct mp_node_s);  // 移可分配的内存位置起始位置
    m = mp_align_ptr(m, MP_ALIGMENT);
    new_node->last = m + size;  // 分配完当前size后下一个分配内存的起点
    current = pool->current;
    // 寻找内存池下一个分配的节点，5次分配失败的过滤
    for (p = current; p->next; p = p->next) {
        if (p->failed++ > 4) {
            current = p->next;
        }
    }
    p->next = new_node;  // 加入链表
    pool->current = current ? current : new_node;
    return m;
}

// mp_alloc 分配内存
void *mp_alloc(struct mp_pool_s *pool, size_t size) {
    unsigned char *m;
    struct mp_node_s *p;
    if (size <= pool->max) {
        p = pool->current;
        do {
            // 32位对齐的下个位置
            m = mp_align_ptr(p->last, MP_ALIGMEENT);
            if ((size_t)(p->end - m) >= size) {
                // 当前节点内存足够，分配成功，返回
                p->last = m + size;
                return m;
            }
            p = p->next; // 不够则找下一个节点
        } while (p);
        // 内存池中所有节点内存都不以满足分配size内存，需要再次分配一个block
        return mp_alloc_block(pool, size);
    }
    return mp_alloc_large(pool, size);
}

// 分配内存并初始化
void *mp_calloc(struct mp_pool_s *pool, size_t size) {
    void *p = mp_alloc(pool, size);
  	if (p) {
      	memset(p, 0, size);
    }
  return p;
}

// 大块节点内存释放
void mp_free(struct mp_pool_s *pool, void *p) {
  	struct mp_large_s *l;
  for (l = pool->large; l; l = l->next) {
    	if (p == l->alloc) {
        	free(l->alloc);
        	l->alloc = NULL;
        	return ;
      }
  }
}
```



#### 二.  Nginx内存池

**内存结构：**

![nginx_mm](..\pic\nginx_mm.png)

**数据结构**

```c
// 内存块结构体，每个内存块都有，在最开头的部分，管理本块内存
// 64位系统大小为32字节
typedef struct {
    u_char 		*last;  // 可用内存的起始位置，小块内存每次都从这里分配
    u_char		*end;  // 可用内存的结束位置
    ngx_pool_t	*next;	// 写一个内存池节点
    ngx_unit_t	failed;  // 本节点分配失败次数，超过4次，认为本节点满，不参与分配，满的内存块也不会主动回收
}ngx_pool_data_t;

// 大块内存节点
typedef struct ngx_pool_large_s ngx_pool_large_t;

struct ngx_pool_large_s {
    ngx_pool_large_t	*next;  // 多块大内存串成链表，方便回收利用
    void			   *alloc;  // 指向malloc分配的大块内存
};


// nginx内存池结构体
// 多个节点串成的单向链表，每个节点分配小块内存
// max，current，大块内存链表旨在头节点
// 64位系统大小位80字节，结构体没有保存内存块大小的字段，由d.end - p得到
struct ngx_pool_s {
    // 本内存节点信息
    ngx_pool_data_t		d;
    // 下面的字段旨在第一个块中有意义
    size_t			   max;  // 块最大内存
    ngx_pool_t		   *current;  // 当前使用的内存池节点
    ngx_chain_t		   *chain;
    ngx_pool_large_t    *large;   // 大块内存
    ngx_pool_cleanup_t  *cleanup;  // 清理链表头指针
    ngx_log_t		   *log;
};

// 创建内存池
ngx_pool_t *ngx_create_pool(size_t size, ngx_log_t *log);

// 销毁内存池
// 调用清理函数链表，检查大块内存链表，直接free，遍历内存池节点，逐个free
void ngx_destroy_pool(ngx_pool_t *pool);

// 重置内存池，释放内存，但不归还系统
// 之前分配的内存块依旧保留，重置空闲指针位置

void ngx_reset_pool(ngx_pool_t *pool);

// 分配内存 8字节对齐，速度快，少量浪费 >4k则直接malloc分配大块内存
void *ngx_palloc(ngx_pool_t *pool, size_t size);

void *ngx_pnalloc(ngx_pool_t *pool, size_t size);  // 不对齐

void *ngx_pcalloc(ngx_pool_t *pool, size_t size); // 对齐分配，且初始化

// 大块内存free
ngx_int_t ngx_pfree(ngx_pool_t *pool, void *p);
```



#### 三.  Limux内核内存管理之伙伴算法

linux内核内存管理就采用伙伴算法，首先从分配内存和释放内存来简单介绍一下伙伴算法的原理

**分配内存 allocate memory**：

1. 找到大于分配内存size的最小2次幂的块，比如(11-->16)
2. 如果当前块大于最小2次幂，则对半分，然后继续1
3. 直到找到合适的块，进行分配

**释放内存free memory**

1. 释放内存块
2. 检查相邻内存块是否释放，如果没有释放则退出
3. 如果已经释放则进行合并，然后继续2

**具体说明：**

![buddy_m](..\pic\buddy_m.png)

上图为一个2 ^ 4 * 64K = 1024K的内存块，最小可分配内存块为64K，即最多可分成16个64K的块

1. 初始化内存

2. A申请34K的内存，因此需要分配给64K的块

   2.1 2.2 2.3 2.4：全部执行对半分操作

   2.5：找到满足条件的块，进行内存块

3. B申请66K内存，因此需要分配128K的块，由现成的直接分配

4. C申请35K内存，需要64K的块，直接分配

5. D申请67K内存，需要128K的块

   5.1：对半分(黄色)

   5.2：分配128K

6. 释放B内存块，没有相邻的内存可以合并

7. 释放D内存块

   7.1：释放内存

   7.2：与相邻内存合并

8. A释放内存，没有相邻的内存可合并

9. C释放内存

   9.1：释放内存

   9.2-9.5：进行合并

**实现思路**

通过一个数组形式的完全二叉树来管理内存，二叉树的节点标记使用状态，高层节点对应大的块，底层节点对应小的块，在分配和释放中通过节点的状态来进行块的分离合并。如下图,32K为5层(2^5)结构，Level0-->

![buddy_tree](..\pic\buddy_tree.png)

[接下贴出云风大佬的实现](https://github.com/cloudwu/buddy.git)

```c
// 节点使用状态
#define NODE_UNUSED 0
#define NODE_USED 1
#define NODE_SPLIT 2
#define NODE_FULL 3

// 二叉树的部分请才参考上图来，比较容易理解
struct buddy {
  	int level;  // 二叉树深度
    uint8_tree[1]; // 记录二叉树用来存储内存块(节点)使用情况,柔性数组，不占内存
};


// 为内存池申请内存块
struct buddy *
buddy_new(int level) {
    // 分配内存大小，5层即为32K
    int size = 1 << level;
    // 分配size * 2 - 2 + buddy头部大小的内存？   sizesof(uint_8_t) == 1 
    // 因此level =  5 其实是分配62kb可供分配的空间
    struct buddy * self = malloc(sizeof(struct buddy) + sizeof(uint_8_t) * (size * 2 - 2));
    // 初始化buddy结构
    self->level = level;
    memset(self->tree, NODE_UNUSED, size * 2 - 1);
    return self;
}

// 释放内存块
void
buddy_delete(struct buddy * self) {
    free(self);
}

// 判断是否为2的幂
static inline int
is_pow_of_2(uint32_t x) {
    // 2 ^ n: 最高位为1，其它位为0
    // 2 ^ n - 1: 最高位为0，其它位为1
    // 因此 (2^n) & (2^n-1) == 0
    return !(x & (x - 1));
}

// 获取大于x的下一个最小2次幂
static inline uint32_t
next_pow_of_2(uint32_t x) {
    if (is_pow_of_2(x))
        return x;
    x |= x>>1;
    x |= x>>2;
    x |= x>>4;
    x |= x>>8;
    x |= x>>16;
    return x + 1;
}

// 根据index, level, max_level取得当前节点在数组中的偏移量
// 比如0, 0, 5 就是整块内存
static inline int
_index_offset(int index, int level, int max_level) {
    return ((index + 1) - (1 << level)) << (max_level - level);
}

// 判断父节点是否状态应该变为FULL
static void
_mark_parent(struct buddy * self, int index) {
    for (;;) {
        int buddy = index - 1 + (index & 1) * 2;  // 兄弟节点
        // 兄弟节点是否被使用
        if (buddy > 0 && (self->tree[buddy] == NODE_USED || self->tree[buddy] == NODE_FULL)) {
            index = (index + 1) / 2 - 1;  // 父节点
            self->tree[index] = NODE_FULL;
        } else {
            return;
        }
    }
}

// 分配内存
int
buddy_alloc(struct buddy * self, int s) {
    // 分配大小s的内存，返回分配内存偏移量地址(首地址)
    int size;
    if (s == 0) {
        size = 1;
    } else {
        // 获取大于s的最小2次幂
        size = (int)next_pow_of_2(s);
    }
    int length = 1 << self->level;
    if(size > length)
        return -1;
    int index = 0;
    int level = 0;
    
    while (index >= 0) {
        if (size == length) {
            if (self->tree[index] == NODE_UNUSED) { // 分配成功
                self->tree[index] = NODE_USED;
                _mark_parent(self, index);
                return _index_offset(index, level, self->level);
            } else {
                switch (self->tree[index]) {
                    case NODE_USED:
                    case NODE_FULL:
                        break;
                    case NODE_UNUSED:
                        self->tree[index] = NODE_SPLIT;
                        self->tree[index * 2 + 1] = NODE_UNSED;
                        self->tree[index * 2 + 2] = NODE_UNSED;
                    default:
                        // 往下层走
                        index = index * 2 + 1;
                        length /= 2;
                        level++;
                        continue;
                }
            }
            
            // 遍历兄弟右节点
            if (index & 1) {
                ++index;
                continue;
            }
            
            // 左右节点都USED，回溯
            for ( ;; ) {
                level--;
                length *= 2;
                index = (index + 1) / 2 - 1;
                if (index < 0)
                    return -1;
                if (index & 1) {
                    ++index;
                    break;
                }
            }
        }
    }
    return -1;
}

static void
_combine(struct buddy * self, int index) {
    // 释放内存尝试合并
    for (;;) {
        int buddy = index - 1 + (index & 1) * 2;  // 兄弟节点
        if (buddy < 0 || self->tree[buddy] != NODE_UNUSED) {
            self->tree[index] = NODE_UNSED;
            // 回溯更新父节点状态
            while (((index = (index + 1) / 2 - 1) >= 0) && self->tree[index] == NODE_FULL) {
                self->tree[index] = NODE_SPLIT;
            }
            return;
        }
        index = (index + 1) / 2 - 1;
    }
}

// 释放内存
void
buddy_free(struct buddy * self, int offset) {
    // 释放偏移量offset开始的内存块
    int left = 0;
    int length = 1 << self->level;
    int index;
    
    for (;;) {
        switch(self->tree[index]) {
            case NODE_USED:
                 // 释放内存并合并
				_combine(self, index);
                return;
            case NODE_UNUSED:
                return;
            default:
                length /= 2;
                if (offset < left + length) {
                    index = index * 2 + 1;
                } else {
                    left += length;
                    index = index * 2 + 2;
                }
                break;
        }
    }
}
```



#### 四. python内存管理

内存池，对象池，GC

Python针对小对象（小于512字节）的内存分配采用内存池来进行管理，大对象直接使用标准C的内存分配器；

对小对象内存的分配器Python进行了3个等级的抽象：arena，pool和block

![python_mem_arch](..\pic\python_mem_arch.png)

- 第0层：操作系统提供的内存管理接口，比如malloc，free
- 第1层：包装malloc，free等接口PyMem_API，提供统一的raw memory管理接口，为了可移植性
- 第2层：构建了更高抽象恩赐的内存管理策略
- 第3层：对象缓冲池

内存池可视为一个层次结构，自下而上分为四层：block，pool，arena和内存池(概念)



**Block：**

内存管理器的最小单元，一个Block存储一个Python对象，大小：[8, 16, 24, 32, 40, 48 ... 512]，(8*n)，以8字节为一个单位，为了字节对齐。

block释放时，会引起pool的状态改变

**Pool：**

一系列Block则组成一个Pool，一个Pool中所有Block大小一样，也就是Pool是一种Block的容器，每个Pool为4K大小(一个虚拟/系统内存页的大小)。一个小对象被销毁后，其内存不会马上归还系统，而是在pool中被管理着，用于分配给后面申请的内存对象。pool的三种状态：used，full，empty

```c
struct pool_header {
    union {
        block *_padding;
        uint count;  // Pool被使用的block数量
    }ref;
    block *freeblock;  // 指向pool中可用block的单向链表
    struct pool_header *nextpool;
    struct pool_header *prevpool;
    uint szidx;  // 记录pool保存的block的大小，一个pool中所有block都是szidx大小
};
```

拥有相同block大小的pool通过双向链表连接起来，

python使用一个数组usedpools来管理使用中的pool

![python_pool](..\pic\python_pool.png)

**Arena**

Arena则是Python从系统分配申请和释放的单位，有256KB，每个Arena中包含了**64个Poo**l（256KB，因此只有<=256KB的数据才会在内存池中申请内存，否则采用malloc），也为双向链表，Python在分配Pool的时候优先选择可用Pool数量少的Arena进行内存分配。因为Python只有在Arena中所有的Pool全为空时才释放Arena中的内存，如果选择上可用Pool数量最多的Arena的话，大量内存会被占用不会销毁。

```c
struct arena_object {
    uintptr_t address;
    block* pool_address;
    struct pool_header* freepools;
    struct arena_object* nextarena;
    struct arena_object* prearena;
};
```

由于Python对内存的申请和释放是以Arena为单位的，所以，如果存在一些对象迟迟没有被释放，那么此时Arena将不会被释放掉，一直占用着这块内存

python中会存在多个arena，这些arena通过一个数组arenas来统一管理。

![python_arena](..\pic\python_arena.png)



小块内存申请与释放流程：

申请：

- 当申请内存时，先取SMALL_REQUEST_THRESHOLD来判断，如果小于它则进行小块分配函数调用
- 通过要申请的nbytes来计算其相应的size index, 再用usedpool[index + index]来优先查看已有的pool，如果没有，则从usable_arenas里找一个可用的pool。如果usable_arenas == NULL, 则调用new_arena来创建一个新的arena，且让usable_arena指向它。
- 从usable_arenas里HEAD取出一个pool，初始化这个pool把它加入usedpool.
- 初始化pool_header并返回对应的BLOCK*.

释放：

- 检查block的地址是否属于内存池，如果是则进行小块内存释放逻辑
- 如果pool包含了该block, 则将其添加至freeblock
- 如果pool的状态是从FULL到USED，则将其添加至usedpool
- 如果pool的状态是从USED到EMPTY，则将其从usedpool中移除。并且将其加入arena_object的freepools中并将nfreepools++
- 如果此时nfreepools == ntotalpools说明所有的pool都被释放了，则可以安全的释放掉这个arena对象并把它从unsable_arenas移到unusded_arena_objects
- 如果nfreepools == 1， 说明改arena从full变为了usable, 则需把它添加到usable_arenas。



**垃圾回收GC**

引用计数一种垃圾回收机制；

引用计数优点：简单和实时

缺点：

- 执行回收效率低，维护引用计数的额外操作与内存分批和释放，引用赋值次数成正比；因此python才设计了小块内存的内存池
- 循环引用：引用标记-清除和分代收集

垃圾回收机制一般分为：垃圾检测和垃圾回收；python基于三色模型来进行垃圾收集；（白色为未检测/不可达，灰色为可达，黑色为已检查）

步骤：

- 寻找root object（全局引用和函数栈的引用）
- 来及检测阶段，找到可达对象
- 把不可达对象的内存回收



#### 五. ringbuffer

[ringbuffer](https://blog.csdn.net/wangqingchuan92/article/details/106070527)
























































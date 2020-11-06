#### 为什么要有内存池？

避免频繁的对内存进行`malloc/free`

##### 内存池原理

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

// 大块内存节点
struct mp_large_s {
    struct mp_large_s *next;
    void *alloc;
};

// 小块内存节点
struct mp_node_s {
 	unsigned char *last;
    unsigned char *end;
    struct mp_node_s *next;
    size_t failed;
};

// 内存池结构
struct mp_pool_s {
  	size_t max;  // 用于小块内存还是大块内存
    struct mp_node_s *current;
    struct mp_large_s *large;
    struct mp_node_s head[0];
};

// 需要实现的接口
struct mp_pool_s *mp_create_pool(size_t size);  // 创建内存池
void mp_destory_pool(struct mp_pool_s *pool);	// 销毁内存池
void *mp_alloc(struct mp_pool_s *pool, size_t size);  // 分配内存 对齐？
void *mp_nalloc(struct mp_pool_s *pool, size_t size);  // 不对齐？
void *mp_calloc(struct mp_pool_s *pool, size_t size);  // 分配内存，并初始化为0
void mp_free(struct mp_pool_s *pool, void *p); // 释放p节点内存

// posix提供的malloc,calloc,realloc返回的地址都是对齐的，32位8字节, 64位16字节，不可调；所以由了posix_memalign
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
    int ret = posix_memalign((void**)&p), MP_ALIGNMENT, size + sizeof(struct mp_pool_s) + sizeof(struct mp_pool_s) + sizeof(struct mp_node_s));
    if (ret) {
        return NULL;
    }
    p->max = (size < MP_MAX_ALLOC_FROM_POOL) ? size : MP_MAX_ALLOC_FROM_POOL;
    p->current = p->head;
    p->large = NULL;
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
// 
static void *mp_alloc_block(struct mp_pool_s *pool, size_t size) {
    unsigned char *m;
    struct mp_node_s *h = pool->head;
    size_t psize = (size_t)(h->end - (unsigned char *)h);
    // 分配psize大小的内存？？？
    int ret = posix_memalign((void **)&m, MP_ALLIGMENT, psize);
   	if (ret) return NULL;
    
    struct mp_node_s *p, *new_node, *current;
    new_node = (struct mp_node_s*)m;
    new_node->end = m + psize;
    new_node->next = NULL;
    new_node->failed = 0;
    
    m += sizeof(struct mp_node_s);
    m = mp_align_ptr(m, MP_ALIGMENT);
    new_node->last = m + size;
    current = pool->current;
    
    for (p = current; p->next; p = p->next) {
        if (p->failed++ > 4) {
            current = p->next;
        }
    }
    p->next = new_node;
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
                // 当前节点内存足够，分配成功
                p->last = m + size;
                return m;
            }
            p = p->next; // 不够则找下一个节点
        } while (p);
        // 现有内存不够，需要再次malloc
        return mp_alloc_block(pool, size);
    }
    return mp_alloc_largepool, size);
}

// 分配内存并初始化
void *mp_calloc(struct mp_pool_s *pool, size_t size) {
    
}
```
















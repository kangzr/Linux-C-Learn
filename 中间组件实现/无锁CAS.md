#### 什么是CAS？

比较并交换Compare and Swap, CAS，是一种原子操作，可用国语多线程编程中实现不被打断的数据交换操作，从而避免多线程同时改写某一数据时由于执行顺序不确定以及中断的不可预知性产生的数据不一致问题。该操作通过将内存中的值与指定数据进行比较，当数值一样时将内存中的数据替换为新的值。

```c
bool CAS(int *pAddr, int nExpected, int nNew)
atomically {
  if (*pAddr == nExpected) {
    *pAddr = nNew;
    return true;
  }
  else
    return false;
}
```



#### 操作的原子性

i++对应汇编的三个操作：

```assembly
# 1. 把变量i从内存RAM加载到寄存器
movl i(%rip), %eax
# 2. 把寄存器的值加1
addl $1, %eax
# 3. 把寄存器的值写回内存RAM
movl %eax, i(%rip)
```

我们所有的变量首先是存储在主存RAM上，CPU要去操作时首先会加载到寄存器，然后再操作，操作完成再写回主存；

当多个线程同时操作时，会出现顺序错乱；



#### 原子操作

gcc，g++编译器提供了一组API来作原子操作

```c
type __sunc_fetch_and_add (type *ptr, type value, ...)
```

```c++
// c++ 11 也有一组atomic接口
// atmoic_is_lock_free
```

Intel X86指令集提供了指令**前缀lock**用于**锁定前端串行总线FSB**，保证了执行执行时不会收到其它处理器的干扰；

如下，使用lock指令前缀后，处理期间对count内存的并发访问(Read/Write)被禁止，从而保证了指令的原理性

```c++
static int lxx_atomic_add(int *ptr, int increment)
{
    int old_value = *ptr;
    __asm__ volatile("lock; xadd %0, %1 \n\t"
                     :"=r"(old_value), "=m"(*ptr)
                     : "0"(increment), "m"(*ptr)
                     : "cc", "memory");
    return *ptr;
}
```

<img src="..\pic\cas_lock.png" alt="cas_lock" style="zoom:75%;" />



#### 自旋锁

posix提供了一组自旋锁(spin lock)的API

```c
int pthread_spin_destroy(pthread_spinlock_t *);
int pthread_spin_init(pthread_spinlock_t *， int pshared);
int pthread_spin_lock(pthread_spinlock_t *);
int pthread_spin_trylock(pthread_spinlock_t *);
int pthread_spin_unlock(pthread_spinlock_t *);
```

```c++
// mutex spin atomic_flag 无锁三种方式实现原子操作
#include <pthread.h>

#define FOR_LOOP_COUNT 1000
static pthread_mutex_t mutex;
static pthread_spinlock_t spinlock;
static atomic_flag_spinlock s_atomic_flag_spinlock;


void *mutex_thread_main(void *argv)
{
    int i;
    for (i = 0; i < FOR_LOOP_COUNT; i++)
    {
        pthread_mutex_lock(&mutex);
        do_for_add(FOR_ADD_COUNT);
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

void *spin_thread_main(void *argv)
{
    int i;
    for (i = 0; i < FOR_LOOP_COUNT; i++)
    {
        pthread_spin_lock(&spinlock);
        do_for_add(FOR_ADD_COUNT);
        pthread_spin_unlock(&spinlock);
    }
}

void *atomic_flag_spinlock_thread_main(void *argv)
{
    int i;
    for (i = 0; i < FOR_LOOP_COUNT; i++)
    {
        s_atomic_flag_spinlock.lock();
        do_for_add(FOR_ADD_COUNT);
        s_atomic_flag_spinlock.unlock();
    }
    return NULL;
}

static int lxx_atomic_add(int *ptr, int increment)
{
    int old_value = *ptr;
    __asm__ volatile("lock; xadd %0, %1 \n\t"
                     :"=r"(old_value), "=m"(*ptr)
                     : "0"(increment), "m"(*ptr)
                     : "cc", "memory");
    return *ptr;
}

void *atomic_thread_main(void *argv)
{
    int i;
    for (i = 0; i < FOR_LOOP_COUNT; i++)
    {
        lxx_atomic_add(&counter, 1);
        return NULL;
    }
}


int main(int argc, char **argv)
{
    printf("use mutex -------->\n");
    test_lock(mutex_thread_main, NULL);
    printf("counter = %d\n", counter);
    
    printf("use spin -------->\n");
    pthread_spin_init(&spinlock, PTHREAD_PROCESS_PRIVATE);
    test_lock(spin_thread_main, NULL);
    printf("counter = %d\n", counter);    
    
    printf("use atomic_flag_spinlock -------->\n");
    test_lock(atomic_flag_spinlock_thread_main, NULL);
    printf("counter = %d\n", counter);     
    
    printf("use atomic -------->\n");
    test_lock(atomic_thread_main, NULL);
    printf("counter = %d\n", counter);  
}
```



#### 无锁队列实现

mutex，自旋锁，无锁队列对比












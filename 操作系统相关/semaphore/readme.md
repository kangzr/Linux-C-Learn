**信号量用作互斥体**

初始化为1，表明互斥体未锁，只有一个线程可以通过信号量而不被阻塞

但，信号量的行为和pthread互斥体不完全相同

```c
Semaphore *mutex = make_semaphore(1);
semaphore_wait(mutex);
// protected code goes hear
semaphore_signal(mutex)
```

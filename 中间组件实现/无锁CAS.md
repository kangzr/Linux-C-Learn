### 什么是CAS？

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



### 操作的原子性

i++对应汇编的三个操作：

1. 把变量i从内存RAM加载到寄存器
2. 把寄存器的值加1
3. 把寄存器的值写回内存RAM

我们所有的变量首先是存储在主存RAM上，CPU要去操作时首先会加载到寄存器，然后再操作，操作完成再写回主存


















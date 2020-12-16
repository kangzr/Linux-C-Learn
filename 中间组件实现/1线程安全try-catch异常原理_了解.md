setjmp / longjmp：可以跨越函数/栈/线程进行跳跃，但是不能在进程间跳跃

goto: 同一个栈/函数内



- 定义一组异常的值
- 嵌套
- 线程安全

pthread_key_local 线程的私有数据/空间
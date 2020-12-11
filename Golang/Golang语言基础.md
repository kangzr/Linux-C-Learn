[参考](https://draveness.me/golang/)

编译原理，运行时，基础知识，进阶知识



词法和语法分析

类型检查

中间代码生成

机器码生成



[Go并发](https://www.cnblogs.com/show58/p/12699083.html)

天然并发：

- 语言层面支持并发，非常简单
- goroutine，轻量级线程，创建成千上万个goroutine成为可能
- 基于CSP（Communicating ）模型

1. 创建goroutine

   ```go
   go loop()   // 开启一个goroutine
   ```

2. 信道

   ```go
   // goroutine之间相互通讯使用信道，其实是做goroutine之间的内存共享
   // 创建信道
   var channel chan int = make(chan int)
   // 或
   channel := make(chan int)
   // 从信道存消息和取消息，默认是阻塞的(叫做无缓冲的信道)
   // 无缓冲的信道在取消息和存消息的时候都会挂起当前的goroutine，除非另一端已经准备好
   
   func main() {
       var message chan string = make(chan string)
       go func(message string) {
           message <- message  // 存消息
       }("Ping!")
       fmt.Println(<-message) // 取消息
   }
   ```

3. 死锁



c协程，一个线程运行多个协程

go协程（协程安全，channel中有锁）：可在多个线程运行多个协程



不支持重载

闭包减少全局变量使用



```go
:=  // 局部变量
```



#### Go语言接口与反射

结构 struct

接口 interface：可以定义一组方法；是实现多态的一种形式；

反射 reflect：计算机程序载运行时可以访问、检测和修改它本身状态或行为的一种能力。也即程序在运行的时候能够“观察”并修改自己的行为	

反射目标：获取变量的类型信息；动态的修改变量内部字段值；




































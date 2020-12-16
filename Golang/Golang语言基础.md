[参考](https://draveness.me/golang/)



Go 语言是一门需要编译才能运行的编程语言



#### 编译原理和流程

编译器前端：词法分析，语法分析，类型检查，中间代码生成

编译器后端：目标代码生成和优化，将中间代码翻译成目标机器能够运行的二进制机器码

![complication process](..\pic\complication process.png)



#### Go编译器逻辑上分为以下四阶段

##### 词法和语法分析

所有编译过程其实都是从解析代码源文件开始；

**词法分析作用**：解析源代码文件，生成Token序列；(lexer词法解析器，可理解为正则匹配的生成器)

**语法分析作用**：输入Token序列，解析Token（根据编程语言定义好的Grammar文法），转换成有意义结构体（语法树）

```go
"json.go": SourceFile {
    PackageName: "json",
    ImportDecl: []Import{
        "io",
    },
    TopLevelDecl: ...
}
```

Token 到上述抽象语法树（AST）的转换过程会用到语法解析器，每一个 AST 都对应着一个单独的 Go 语言文件，这个抽象语法树中包括当前文件属于的包名、定义的常量、结构体和函数等。

AST：是源代码语法的结构的一种抽象表示，它用树状的方式表示编程语言的语法结构；



##### 类型检查和AST转换

对AST中定义和使用的类型进行检查

- 常量、类型和函数名及类型
- 变量的赋值和初始化
- 函数和闭包的主体
- 哈希键值对的类型
- 导入函数体
- 外部的声明



##### (中间代码)通用SSA生成

将AST转换成中间代码



##### 机器码生成









---





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



 

小写：private；大写：public
































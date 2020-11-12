



zeromq：消息队列（偏异步处理）

rpc：偏同步



BRPC：

1. 支持协议更多，支持grpc
2. BRPC百度开源，资料详细
3. 无锁队列，cpu cache，协程M(协程)：N(pthread)

reactor模型：一个fd绑定一个连接connection



REST与RPC区别：大部分都是对外提供公共服务

REST：HTTP接口的风格，更容易理解，方便对外提供服务

gflag：命令行解析



RPC（remote procedure call）远程过程调用

常见RPC框架

- gRPC：Google公布的开源软件，基于谷歌的HTTP2.0协议，并支持场间的众多编程语言，RPC框架基于HTTP协议实现
- Thrift：Facebook的开源RPC框架，主要是一个跨语言的服务开发框架，用户只在其上进行二次开发即可，应用对于底层的RPC通讯等都是透明的。
- Dubbo：阿里集团开源的一个极为出名的RPC框架，在很多互联网公司和企业应用中广泛使用。
- BRPC：百度开源的RPC框架，已在百度有上百万实例的应用，C++语言实现，适合C++程序员学习

RPC完整框架如下图，其中核心功能为RPC协议。

![rpc_arch](pic\rpc_arch.png)

一个RPC的核心功能主要有5个部分组成：客户端、客户端Stub、网络传输模块、服务端Stub、服务端等。

![rpc_stub](pic\rpc_stub.png)



RPC核心功能主要由5个模块组成，其中最核心的三个技术点：

- 服务寻址

  可使用Call ID映射，在RPC中，所有函数都必须有自己的一个ID，本地调用中，函数体是直接通过函数指针来指定的，但远程调用中，函数指针是不行的， 因为两个进程的地址空间完全不一样。

  客户端在做远程调用时，必须附上这个ID，然后还需在客户端和服务端分别维护一个函数和Call ID的对应表

  当客户端需要进行远程调用时，查表找到相应的Call ID，然后传给服务端；服务端也通过查表，来确定客户端需要调用的函数，然后执行相应函数的代码。

- 数据流的序列化和反序列化

  protobuf

- 网络传输

  远程调用往往用在网络上，客户端和服务端通过网络连接

  

  - 短连接：每次请求创建一个新连接，完成后关闭
  - 连接池：每次请求前从pool中获取一个新连接，结束后归还
  - 单连接：所有发往同一地址的请求/返回，共用一个连接（）

常见的传输协议：

**基于TCP协议实现的RPC调用**：由于TCP协议处于协议栈的下层，能够更加灵活地对协议字段进行定制，减少网络开销，提高性能，实现更大的吞吐量和并发数

基于HTTP协议实现的RPC：可使用JSON和XML格式的请求或响应数据，HTTP协议是上层协议，发送包含同等内容的信息，使用HTTP协议传输所占用的字节数会比使用TCP协议传输所占用的字节数更高。



##### 1. brpc同步调用：

client对远端的server进行调用，同时自己原地等待，等待rpc返回后，再进行之后的操作

stub.Echo调用可以看到回调函数被设置为NULL，那么此时调用被设置为同步操作，原地等待。

`stub.Echo(&cntl, &request, &response, NULL);`

同步访问中的response/controller不会在CallMethod后被框架使用，它们都可以分配在栈上

##### 2. brpc异步调用

rpc调用后，callmethod结束，继续执行后续动作，等rpc返回之后，会调用事先注册的回调函数执行

```c++
// 一般在堆上创建，防止done中调用时被弹出栈，但是访问无效块
example::EchoResponse* response = new example::EchoResponse();
// 注册回调函数，rpc返回之后的操作都在done的回调函数中进行
google::protobuf::Closure* done = brpc::NewCallback(&HandleEchoResponse, cntl, response);
stub.Echo(cntl, &request, response, done);
```

##### 3. 半同步

等待rpc完成操作实际是提供一种等待机制，在任意时刻可以出发等待操作，即使在异步调用过程中，可以等待异步的操作完成后，再进行后续操作，也被称之为半同步操作

```c++
example::EchoResponse* response_1 = new example::EchoResponse();
brpc::Controller* cntl_1 = new brpc::Controller();
example::EchoRequest request1;
request1.set_message("hello world from request1");

example::EchoResponse* response_2 = new example::EchoResponse();
brpc::Controller* cntl_2 = new brpc::Controller();
example::EchoRequest request2;
request2.set_message("hello world from request2");

cntl_1->set_log_id(log_id ++);  // set by user
cntl_2->set_log_id(log_id ++);

google::protobuf::Closure* done_1 = brpc::NewCallback(&HandleEchoResponse, cntl_1, response_1);
google::protobuf::Closure* done_2 = brpc::NewCallback(&HandleEchoResponse, cntl_2, response_2);

stub.Echo(cntl_1, &request1, response_1, done_1);
stub.Echo(cntl_2, &request2, response_2, done_2);

//等待两个rpc调用的完成
brpc::Join(id1);
brpc::Join(id2);
```





#### 高性能RPC框架BRPC核心机制分析

BRPC以其高性能，低延迟，易用性等优势，使得其在高性能的C++开发领域非常受欢迎。





---



### Protobuf 

一种轻便高效的结构化数据存储格式，平台无关、语言无关、可扩展，可用于**通讯协议**和**数据存储**等领域。

#### 协议概念：

协议是一种约定，通过约定，不同进程可以对一段数据产生相同的理解，从而可以相互写作，存在进程间通信的程序就一定需要协议

#### 协议设计目标：

- 解析效率：互联网业务具有高并发的特点，解析效率决定了使用协议的CPU成本；
- 编码长度：信息编码出来的长度，编码长度决定了使用协议的网络带宽及存储成本
- 易于实现：互联网业务需要一个轻量级的协议
- 可读性：编码后的数据可读性决定了使用协议的调式及维护成本。
- 兼容性：
- 跨平台语言：互联网的业务涉及到不同的平台和语言
- 安全可靠：防止数据被破解

#### 序列化与反序列化

序列化：把对象转换为字节序列的过程

反序列化：把字节序列恢复为对象的过程

**序列化方法：**（采用比较成熟的开源）

- TLV编码及其变体（tag，length，value）：比如Protobuf（二进制格式）
- **文本**流编码：比如XML/JSON
- 固定结构编码：协议约定传输字段类型和字段含义，比如TCP/IP
- 内存dump：把内存中的数据直接输出，不做任何序列化操作，反序列化直接还原内存(很快)

**主流序列化协议**：xml，json，protobuf

xml：可扩展标记语言，是一种通用和重量级的数据交换格式，以**文本格式**存储

json：一种通用和轻量级的数据交换格式，以**文本格式**进行存储

protocol buffer：Google的一种独立和轻量级的数据交换格式，**以二进制格式进行存储**



```protobuf
syntax = "proto3";  // 语法
packge IM.Login;  // 生成对应的c++命名空间 IM::Login package生成命名空间
import "IM.BaseDefine.proto";  // 引用其它proto文件

// message关键字，代表一个对象
message Phone {
	
	string number = 1;  // Tag   生成 set_XX 等
	IM.BaseDefine.PhoneType phone_type = 2;
}

message Book {
	string name = 1;
	string price = 2;
}

message Person {
	// rule(repeated / optional) 数据类型  字段名  tag顺序 选项	
	string name = 1;
	int32 age = 2;  // set_XX
	repeated string languages = 3;  // 重复,可嵌套，比如会多种语言  add_
	Phone phone = 4;  // 
	repeated Book book = 5; // 自定义类型
}

```



````shell
# shell脚本
protoc -I=$SRC_DIR --cpp_out=$DST_DIR $SRC_DIR/*.proto
````

将proto文件生成对应的.cc和.h文件，用对应生成的对象去做序列化和反序列化。



redis协议为明文协议

protobuffer流程：

1. 编写.proto文件，定义协议内容
2. protoc .proto文件，生成对应文件(.cc .h)
3. 将文件引入项目



**Protobuffer: 数据压缩**

-1在内存中如何存储

-1 ： 32位 则为32个1

在protobuf中：(n << 1) ^ (n >> 31)：



c++中怎么存储二进制：len+string



protobuffer   长度+数组

protobuffer组织形式：key+value

key：tag << 3 | ware_type



































####  RPC（remote procedure call）远程过程调用

常见RPC框架

- gRPC：Google公布的开源软件，支持场间的众多编程语言
- Thrift：Facebook的开源RPC框架
- Dubbo：阿里集团开源的一个极为出名的RPC框架，在很多互联网公司和企业应用中广泛使用。
- BRPC：百度开源的RPC框架，已在百度有上百万实例的应用，C++语言实现



#### 一. 如何实现一个简单的RPC框架？





一个RPC的核心功能主要有5个部分组成：客户端、客户端Stub、网络传输模块、服务端Stub、服务端等。

RPC Client：服务调用方

RPC Client Stub：存放rpc服务端地址信息，将客户端的请求数据打包成网络信息，再通过网络传输发送给服务端

RPC Server：服务提供者

RPC Server Stub：接收Rpc Client发送过来的请求消息并解包，然后调用本地服务进行处理



一个完整的RPC通信需要包含三部分：

- protobuf自动生成的代码(服务发现，服务暴露)
- RPC框架
- 用户填充代码



- 服务寻址

  可使用Call ID映射，在RPC中，所有函数都必须有自己的一个ID，本地调用中，函数体是直接通过函数指针来指定的，但远程调用中，函数指针是不行的， 因为两个进程的地址空间完全不一样。

  客户端在做远程调用时，必须附上这个ID，然后还需在客户端和服务端分别维护一个函数和Call ID的对应表

  当客户端需要进行远程调用时，查表找到相应的Call ID，然后传给服务端；服务端也通过查表，来确定客户端需要调用的函数，然后执行相应函数的代码。

- 数据流的序列化和反序列化

  protobuf

- 网络传输

  远程调用往往用在网络上，客户端和服务端通过网络连接



#### 二. Protobuf介绍

- protocol buffers 是一种语言无关、平台无关、可扩展的序列化结构数据的方法，它可用于（数据）通信协议、数据存储等
- Protocol Buffers 是一种灵活，高效，自动化机制的结构数据序列化方法－可类比 XML，但是比 XML 更小（3 ~ 10倍）、更快（20 ~ 100倍）、更为简单。
- 你可以定义数据的结构，然后使用特殊生成的源代码轻松的在各种数据流中使用各种语言进行编写和读取结构数据。你甚至可以更新数据结构，而不破坏由旧数据结构编译的已部署程序。



protobuf协议在编码和空间效率都是上非常高效的，这也是很多公司采用protobuf作为数据序列化和通信协议的原因









**主流序列化协议**：xml，json，protobuf

xml：可扩展标记语言，是一种通用和重量级的数据交换格式，以**文本格式**存储

json：一种通用和轻量级的数据交换格式，以**文本格式**进行存储

protocol buffer：Google的一种独立和轻量级的数据交换格式，**以二进制格式进行存储**



protobuffer流程：

1. 编写.proto文件，定义协议内容

   ```protobuf
   syntax = "proto3";  // 语法
   packge echo;  // 生成对应的c++命名空间 echo package生成命名空间
   
   // message关键字，代表一个对象
   message Phone {
   	string number = 1;  // Tag   生成 set_XX 等
   	IM.BaseDefine.PhoneType phone_type = 2;
   }
   
   message Person {
   	repeated string languages = 3;  // 重复,可嵌套，比如会多种语言  add_
   	Phone phone = 4;  // 
   	repeated Book book = 5; // 自定义类型
   }
   ```

   

2. protoc .proto文件，生成对应文件(.cc .h) `protoc xx.proto --cpp_out=.`（可生成对应语言的pb文件）

3. 将文件引入项目；`g++ test_msg.cc msg.pb.cc -lprotobuf`



google protobuf只负责消息的打包和解包，并不包含RPC的实现，但其包含了RPC的定义。也不包括通讯机制。

protobuf rpc的实现主要包括编写proto文件并编译生成对应的service_pb2.proto文件，继承RpcChannel并实现CallMethod和调用Service的CallMethod，继承Service来实现暴露给客户端的函数。

1. 编写*.proto文件，用probuf语法编写IEchoService     及其相应的method，echo; (IEchoClient 与 echo_reply)
2. 把proto文件，编译成*py文件，会生成相应的IEchoService     (被调用方)和 IEchoService_Stub(调用方)的类，IEchoService_Stub继承IEchoService，(Client同)
3. 当Server作为被调用方，需要实现自己的Service(MyEchoService)，继承IEchoService并自己实现具体的rpc处理逻辑，如echo方法，供client调用
4. 当Client作为被调用方，需要实现自己的Service(MyEchoClientReply)，继承IEchoClient并实现具体rpc处理逻辑，例如echo_reply，供server调用

5. echo流程：client 创建TcpClient，并连接服务端，连接建立后，利用MyEchoClientReply创建RpcChannel，IEchoService_Stub利用RpcChannel创建client的stub；  client.stub.echo("123")，会调用RpcChannel的CallMethod发送出去（**任何rpc最终都调用CallMethod**），接收方`TcpConnection.handle_read`调用RpcChannel的input_data，反序列化后，会调用**EchoService的CallMethod**，protobuf会自动调用我们实现的echo方法。

6. echo_reply流程：server收到client的echo调用后，利用IEchoClient_Stub构建client_stub，     通过client_stub.echo_reply(msg)，调用MyEchoClientReply中的echo_reply



- 解析效率：互联网业务具有高并发的特点，解析效率决定了使用协议的CPU成本；
- 编码长度：信息编码出来的长度，编码长度决定了使用协议的网络带宽及存储成本
- 易于实现：互联网业务需要一个轻量级的协议
- 可读性：编码后的数据可读性决定了使用协议的调式及维护成本。
- 兼容性：
- 跨平台语言：互联网的业务涉及到不同的平台和语言
- 安全可靠：防止数据被破解



#### 三. Protobuf+Python实现Rpc框架

调用方：

- 调用方主动调用echo发送message数据
- protobuf rpc会自动调用rpc_channel的CallMethod方法
- CallMethod调用具体的通信过程发送数据（需要自己实现通信）

被调用方：

- 读到数据，并通过protobuf的定义解析message
- 找到指定的IEchoService的echo方法，并执行

客户端调用服务端和服务端调用客户端是两个单独的过程



1. 编写proto文件，生成相应的rpc接口定义对应的.py文件

   ```protobuf
   syntax = "proto3"; // 指定使用proto3语法
   import "google/protobuf/empty.proto"; // 用于使用empty类型
   package echo;  // 可理解为python模块
   option py_generic_services = true;  // 这个必须要，不然无法生成两个完整的类
   message RequestMessage
   {
   	string msg = 1; // 1 可理解为 tag   或者   id
   }
   message ResponseMessage
   {
   	string msg = 1;
   }
   // service会生成两个类 一个IEchoService(作为被调用端) 一个IEchoService_Stub(继承IEchoService 作为调用方)
   
   // 客户端发给服务端
   service IEchoService
   {
   	rpc echo(RequestMessage) returns(google.protobuf.Empty);
   }
   
   // 服务端发给客户端
   service IEchoClient
   {
   	rpc echo_reply(ResponseMessage) returns(google.protobuf.Empty);
   }
   ```

2. 实现通信层

   使用python的asyncore，包括TcpConnection, TcpServer, TcpClient；

   Rpc客户端要实现google.protobuf.RpcChannel，主要实现CallMethod接口，任何rpc接口，最终都是调用到CallMethod方法。

   这个函数的典型实现是将RPC调用参数序列化，然后给网络模块发送

   

3. RPC序列化及流程

   无论是调用端还是被调用端，一个method_descriptor在其Service的index一致，因此只需要对index进行序列化即可

   RPC调用的参数可直接使用protobuf的SerializeToString方法进行序列化，接收端通过ParseFromString进行反序列化

   被调用方调用IEchoService.CallMethod，根据RPC传过来的method_descriptor和request参数，找到对应方法的接口并执行。

   （1. 接收数据； 2. 对数据进行反序列化，解析得到method_descriptor和request；3. 调用CallMethod方法）





区分网络通信的服务端/客户端与RPC服务端/客户端

RpcServiceStub和RpcService类是protobuf编译器根据proto定义生成的类。

RpcService定义了服务端暴露给客户端的函数接口，具体实现需要用户自己继承这个类来实现。

RpcServiceStub定义了服务端暴露函数的描述，并将客户端对RpcServiceStub中函数的调用统一转换到调用RpcChannel中的CallMethod方法

CallMethod通过RpcServiceStub传过来的函数描述符和函数参数对该次rpc调用进行encode，最终通过RpcConnecor发送给服务方。

对方以客户端相反的过程最终调用RpcSerivice中定义的函数

事实上，protobuf rpc的框架只是RpcChannel中定义了空的CallMethod，所以具体怎样进行encode和调用RpcConnector都要自己实现。

RpcConnector在protobuf中没有定义，所以这个完成由用户自己实现，它的作用就是收发rpc消息包。

在服务端，RpcChannel通过调用RpcService中的CallMethod来具体调用RpcService中暴露给客户端的函数。











#### 四. 项目Rpc原理



#### 五. Protobuf + C++ Boost.Asio实现Rpc框架

- 同步：client对远端的server进行调用，同时自己原地等待，等待rpc返回后，再进行之后的操作
- 异步：rpc调用后，callmethod结束，继续执行后续动作，等rpc返回之后，会调用事先注册的回调函数执行



#### RPC客户端

需要实现`google::protobuf::RpcChannel`，主要实现`RpcChannel::CallMethod`接口，RPC客户端调用任何一个RPC接口，最终都是调用`CallMethod`，这个接口的典型实现就是将RPC调用参数序列化，然后投递给网络模块进行发送；



#### RPC服务端

需要实现RPC接口，直接实现MyService中定义的接口



#### 标识service&method

建立RPC接口到protobuf service对象的映射；每个service/rpc对应一个唯一的id；

```cpp
// 注册
_rpcCallMap[rpcIdx] = make_pair<rpcService, pDes>;
```



#### RpcChannel

可理解为一个通道，连接rpc服务的两端，本质上通过socket通信；google protobuf实现为一个纯虚函数；

因此需要实现一个子类，基类为`RpcChannel`，并实现`CallMethod`方法，应该实现两个功能

- 序列化request，发送到对端，同时需要标识机制使得对端知道如何解析(schema)和处理(method)这类数据
- 接受对端数据，反序列化到response

```cpp
// Abstarct interface for an RPC channel.
// You should not call an RpcChannel directly, but instead construct a stub Service wrapping it
// RpcChannel* channel = new MyRpcChannel("remotehost.exmaple.com:1234");
// MyService* service = new MyService::Stub(channel);
// service->MyMethod(request, &response, callback);
```



[极简版RPC](https://izualzhy.cn/demo-protobuf-rpc)

[minRPC](https://github.com/shinepengwei/miniRPC)
















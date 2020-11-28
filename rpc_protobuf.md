###  RPC（remote procedure call）远程过程调用

常见RPC框架

- gRPC：Google公布的开源软件，支持场间的众多编程语言
- Thrift：Facebook的开源RPC框架
- Dubbo：阿里集团开源的一个极为出名的RPC框架，在很多互联网公司和企业应用中广泛使用。
- BRPC：百度开源的RPC框架，已在百度有上百万实例的应用，C++语言实现



#### 一. Protobuf介绍

**主流序列化协议**：

xml：可扩展标记语言，是一种通用和重量级的数据交换格式，以**文本格式**存储

json：一种通用和轻量级的数据交换格式，以**文本格式**进行存储

protocol buffer：Google的一种独立和轻量级的数据交换格式，**以二进制格式进行存储**

protobuf协议在编码和空间效率都是上非常高效的（做了大量的优化），这也是很多公司采用protobuf作为数据序列化和通信协议的原因

- 是一种语言无关、平台无关、可扩展的序列化结构数据的方法，它可用于（数据）通信协议、数据存储等
- 是一种灵活，高效，自动化机制的结构数据序列化方法－可类比 XML，但是比 XML 更小（3 ~ 10倍）、更快（20 ~ 100倍）、更为简单。
- 你可以定义数据的结构，然后使用特殊生成的源代码轻松的在各种数据流中使用各种语言进行编写和读取结构数据。你甚至可以更新数据结构，而不破坏由旧数据结构编译的已部署程序。

**protobuffer使用流程：**

1. 编写.proto文件，定义协议内容

2. protoc .proto文件，生成对应文件(.cc .h) `protoc xx.proto --cpp_out=.`（可生成对应语言的pb文件）

3. 将文件引入项目；`g++ test_msg.cc msg.pb.cc -lprotobuf`

google protobuf只负责消息的打包和解包，包含了RPC的定义(但不包括实现)。也不包括通讯机制。



#### 二. 如何实现一个简单的RPC框架？

**远程调用**：将要远方执行的方法和相关参数通过网络传到对端，对端收到数据后，对其进行解析然后执行；

1. 如何知道远端有哪些接口是暴露的？（服务发现）
2. 如何高效的将数据序列化并发送至远端？（序列化+网络传输）
3. 远端如何知道传输过来的数据rpc调用以及调用哪个方法？（服务寻址）

**Protobuf RPC框架如下图：**

- 客户端/服务端 service_stub: 用于获取和调用远端rpc方法
- 客户端/服务端 rpc_service：实现暴露给远端的rpc方法
- 网络传输模块

![protobuf_rpc_flow](pic\protobuf_rpc_flow.png)





#### 三. Protobuf+Python实现Rpc框架

**1. 编写game_service.proto文件，并生成对应py文件**

```protobuf
syntax = "proto3"; // 指定使用proto3语法
import "google/protobuf/empty.proto"; // 用于使用empty类型
package pluto;  // 可理解为python模块
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

// 客户端调用服务端和服务端调用客户端是两个单独的过程(因为各自暴露的给对方的接口是不一样的)

// 客户端发给服务端  IEchoSerice供服务端实现对应方法 IEchoService_Stub客户端调用rpc方法
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

通过`protoc game_service.proto --python_out=.` 生成相应一个game_service_pb2.py文件，针对每一个service会生成 RpcServiceStub和RpcService类：

- RpcService：定义了被调用端暴露给调用端的函数接口，具体实现需要用户自己继承这个类来实现。
- RpcServiceStub：定义了被调用端暴露的函数的描述，调用端通过其调用远端rpc方法，它会把调用统一转换到RpcChannel中的CallMethod方法



**如何解决服务发现和服务寻址问题？**

- 在RPC中，所有函数都必须有自己的一个ID，本地调用中，函数体是直接通过函数指针来指定的，但远程调用中，函数指针是不行的， 因为两个进程的地址空间完全不一样。
- 客户端和服务端分别维护一个函数和Call ID的对应表（descriptor），在远程调用时，查表找到相应的Call ID，然后传给服务端
- 服务端收到数据后，也通过查表，来确定客户端需要调用的函数，然后执行相应函数的代码。

```python
_IECHOSERVICE = _descriptor.ServiceDescriptor(
  name='IEchoService',
  full_name='pluto.IEchoService',
  file=DESCRIPTOR,
  index=0,
  options=None,
  serialized_start=100,
  serialized_end=158,
  methods=[
  _descriptor.MethodDescriptor(
    name='echo',
    full_name='pluto.IEchoService.echo',
    index=0,
    containing_service=None,
    input_type=_REQUESTMESSAGE,
    output_type=_VOID,
    options=None,
  ),
])

IEchoService = service_reflection.GeneratedServiceType('IEchoService', (_service.Service,), dict(
  DESCRIPTOR = _IECHOSERVICE,
  __module__ = 'game_service_pb2'
  ))

IEchoService_Stub = service_reflection.GeneratedServiceStubType('IEchoService_Stub', (IEchoService,), dict(
  DESCRIPTOR = _IECHOSERVICE,
  __module__ = 'game_service_pb2'
  ))
```



##### 2. RpcChannel实现

需要继承google.protobuf.RpcChannel，实现CallMethod和input_data接口：

- CallMethod：用于调用远端rpc方法，序列化+网络传输
- input_data：接收rpc调用，反序列化，找到对应的method执行

```python
# rpc_channel.py
from google.protobuf import service
class RpcChannel(service.RpcChannel):
	def __init__(self, rpc_service, conn):
		super(RpcChannel, self).__init__()
		self.rpc_service = rpc_service
		self.conn = conn
        
    # 所有对远端的rpc调用都会走CallMethod，protobuf rpc的框架只是RpcChannel中定义了空的CallMethod，因此需要自己实现
    # 其会从method_descriptor获取对应的index，然后对数据进行序列化，发送至远端
	def CallMethod(self, method_descriptor, rpc_controller, request, response_class, done):
		index = method_descriptor.index
		data = request.SerializeToString()   # 序列化
		self.conn.send_data(''.join([struct.pack('!ih', total_len, index), data]))

    # 所有接收远端rpc数据都走input_data，同样需要自己实现
    # 无论是调用端还是被调用端，一个method_descriptor在其Service的index一致
    # 根据index找到对应的method，然后rpc_sevice.CallMethod找到对应的rpc方法并执行
	def input_data(self, data):
		total_len, index = struct.unpack('!ih', data[0:6])
		rpc_service = self.rpc_service
		s_descriptor = rpc_service.GetDescriptor()
		method = s_descriptor.methods[index]
		request = rpc_service.GetRequestClass(method)()
		request.ParseFromString(data)  # 反序列化
		rpc_service.CallMethod(method, self.rpc_controller, request, None)
		return True
```

##### 3. 实现通信层

使用python的asyncore（使用select IO多路复用技术来处理socket连接），包括TcpConnection, TcpServer, TcpClient；



![python_rpc_flow](pic\python_rpc_flow.png)



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
















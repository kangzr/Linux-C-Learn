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

**google protobuf只负责消息的打包和解包，包含了RPC的定义**(但不包括实现)。也不包括通讯机制。



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
- 客户端和服务端分别维护一个函数和rpc ID的对应表（descriptor），在远程调用时，查表找到相应rpc的ID，然后传给服务端
- 服务端收到数据后，也根据rpc ID查表，来确定客户端需要调用的函数，然后执行相应函数的代码。

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



##### 2.rpc接口定义的实现

```python
# echo_server.py
# 被调用方的Service要自己实现具体的rpc处理逻辑
class MyEchoService(IEchoService):

    def echo(self, controller, request, done):

        rpc_channel = controller.rpc_channel
        msg = request.msg

        response = ResponseMessage()
        response.msg = "echo:"+msg

        print "response.msg", response.msg

        # 此时，服务器是调用方，就调用stub.rpc，客户端时被调用方,实现rpc方法。
        client_stub = IEchoClient_Stub(rpc_channel)
        client_stub.echo_reply(controller, response, None)
        
        
# echo_client.py
class MyEchoClientReply(IEchoClient):
    def echo_reply(self, rpc_controller, request, done):
        print "MyEchoClientReply:%s"%request.msg

    if __name__ == "__main__":
        request = RequestMessage()
        request.msg = "hello world"
        client = TcpClient(LISTEN_IP, LISTEN_PORT, IEchoService_Stub, MyEchoClientReply)
        client.sync_connect()
        client.stub.echo(None, request, None)
        asyncore.loop()  # 启动poll
```



##### 3. 实现通信层

使用python的asyncore（使用select IO多路复用技术来处理socket连接），以下为python2.7源码中IO多路复用的部分代码：

```python
# python/asyncore.py
def poll(timeout=0.0, map=None):
    # ...
        try:
            r, w, e = select.select(r, w, e, timeout)
        except select.error, err:
            if err.args[0] != EINTR:
                raise
            else:
                return

        for fd in r:
            obj = map.get(fd)
            if obj is None:
                continue
            read(obj)  # obj即为asyncore.dispatcher

        for fd in w:
            obj = map.get(fd)
            if obj is None:
                continue
            write(obj)
```

通信模块需要实现TcpConnection, TcpServer, TcpClient

```python
# TcpConnection.py
# 底层连接，服务数据读和写
class TcpConnection(asyncore.dispatcher):
    def handle_close(self):
        asyncore.dispatcher.handle_close(self)
        self.disconnect()

    def handle_read(self):  # recv
        data = self.recv(self.recv_buff_size)
        self.rpc_channel.input_data(data)
	
    def handle_write(self):  # 如果writable为true，则send
        if self.writebuff:
            sizes = self.send(self.writebuff)
            self.writebuff = self.writebuff[sizes:]
    
    def writable(self):  # 是否可写
        return len(self.writebuff) > 0
	
    # rpcChannel CallMethod调用
    def send_data(self, data):  # 发送数据
        self.writebuff += data

# TcpServer
# 负责accpet, 建立一条TcpConnection通道
class TcpServer(asyncore.dispatcher):
    def __init__(self, ip, port, service_factory):
        asyncore.dispatcher.__init__(self)
        self.ip = ip
        self.port = port
        self.service_factory = service_factory
        self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
        self.bind((self.ip, self.port))
        self.listen(50)

    def handle_accept(self):
        sock, addr = self.accept()  # accept连接
        conn = TcpConnection(sock, addr)
        self.handle_new_connection(conn)

    def handle_new_connection(self, conn):
        service = self.service_factory()
        rpc_channel = RpcChannel(service, conn)
        conn.attach_rpc_channel(rpc_channel)

# TcpClient 用户客户端连接服务端
class TcpClient(TcpConnection):
    def __init__(self, ip, port, stub_factory, service_factory):
		pass
    def sync_connect(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(self.peername)
        sock.setblocking(0)
        self.set_socket(sock)
        self.setsockopt()
        self.handle_connect()
    
    def handle_connect(self):
        self.status = self.ST_ESTABLISHED
        # service 是被动接收方 stub是主动发送方
        self.service = self.service_factory()
        self.channel = RpcChannel(self.service, self)
        self.stub = self.stub_factory(self.channel)
        self.attach_rpc_channel(self.channel)

    def writable(self):
        return TcpConnection.writable(self)
```



##### 4. RpcChannel实现

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

![python_rpc_flow](pic\python_rpc_flow.png)

![python_rpc](pic\python_rpc.png)

#### 四. Protobuf + C++ Boost.Asio实现Rpc框架

Boost.Asio主要用于网络编程。其为网络编程提供了很多I/O objects，比如boost::asio::ip::tcp::socket

- 同步：client对远端的server进行调用，同时自己原地等待，等待rpc返回后，再进行之后的操作
- 异步：rpc调用后，callmethod结束，继续执行后续动作，等rpc返回之后，会调用事先注册的回调函数执行

##### 1. 同步实现方案（单向）

```cpp
// test_Server
class MyEchoService : public echo::EchoService {
    virtual void Echo(::google::protobuf::RpcController*,
                     const ::echo::EchoRequest* request,
                     ::echo::EchoResponse* response,
                     ::google::protobuf::Closure* done) {
		std::cout << request->msg() << std::endl;
         response->set_msg(std::string("I have recv '") + request->msg() + std::string("'"));
        done->Run();
    }
};

int main() {
    MyServer my_server;
    MyEchoService echo_service;
    my_server.add(&echo_service);  // 将echo_service添加到server的_services中
    my_server.start("127.0.0.1", 8888)
}

// MyServer
class MyServer {
public:
    void add(::google::protobuf::Service* sevice) {
        _services[service_info.sd->name()] = service_info;
    }
    void start(const std::string& ip, const int port);
private:
    void dispatch_msg(...);  // 
    void on_resp_msg_filled(...);
    void pack_message(...);  // 序列化
}

// 创建socket，并accept
void MyServer::start(const std::string& ip, const int port) {
    boost::asio::io_service io;
    boost::asio::ip::tcp::acceptor acceptor(
        io, 
        boost::asio::ip::tcp::endpoint(boost::asio:ip::address::from_string(ip), port)
    );
    while (true) {
        // 处理连接，接收数据，发送数据
        auto sock = boost::make_shared<boost::asio::ip::tcp::socket>(io);
        acceptor.accept(*sock);  // 阻塞
        sock->receive(boost::asio::buffer(meta_size));  // 接收数据
        // 反序列化
        dispatch_msg(...);  // 分发数据，处理数据
        } 
    }
}

void MyServer::dipatch_msg(){
    // 根据index获取对应method
    auto service = _services[service_name].service;
    // 回调函数
    DoneParams params = {recv_msg, resp_msg, sock};
    auto done = ::google::protobuf::NewCallback(this, &MyServer::on_resp_msg_filled, params);
    // 调用test_Server实现的Echo方法(rpc方法)并执行
    service->CallMethod(md, &controller, recv_msg, resp_msg, done);
}

// test_Server/Echo/done->Run驱动，填充数据后调用
void MyServer::on_resp_msg_filled(DoneParams params) {
    // 序列化
    pack_message(params.resp_msg, &resp_str);
    // 发送至客户端
    params.sock->send(boost::asio::buffer(resp_str));
}


// test_client
int main() {
    MyChannel channel;
    channel.init("127.0.0.1", 8888);  // 创建sock 连接服务端
    echo::EchoRequest request;
    echo::EchoResponse response;
    echo::EchoService_Stub stub(&channel);
    MyController cntl;
    stub.Echo(&cntl, &request, &respnose, NULL);
    std::cout << "resp: " << response.msg() << std::endl;
    return 0;
}

// Channel
// 一个纯虚函数 需要继承并实现
class MyChannel : public ::google::protobuf::RpcChannel {
public:
    void init(const std::string& ip, const int port) {
        _io = boost::make_shared<boost::asio::io_service>();
        _sock = boost::make_shared<boost::asio::ip::tcp::socket>(*_io);
        boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string(ip), port);
        _sock->connect(ep);
    }
    virtual void CallMethod(const ::google::protobuf::MethodDecriptor* method,
                           ::google::protobuf::RpcController*,
                           const ::google::protobuf::Message* request,
                           ::google::protobuf::Message* response,
                           ::google::protobuf::Closure*) {
        // 序列化
        // 发送数据
        _sock->send(boost::asio::buffer(serialized_str));
        // 接收数据
        _sock->recieve(boost::asio::buffer(resp_data_size));  // 先收大小
        _sock->recieve(boost::asio::buffer(resp_data));  // 再收数据
        response->ParseFromString(std::string(&resp_data[0], resp_data.size())); // 反序列化到response
    }
};
```



##### 2. 异步实现方案

ASIO异步的核心就是一个`boost::asio::io_service`类，（I/O Service 被 I/O context概念取代）

I/O service代表一个操作系统接口，在Windows中，boost::asio::io_service基于IOCP，在Linux中，其基于epoll；

```cpp
// TcpServer
class TcpServer
{
public:
    TcpServer(boost::asio::io_service & io);
    void sendMessageToAllClient(std::string str);
    void echo(std::string str);
private:
    boost::asio::ip::tcp::acceptor acceptor;
    std::vector<TcpConnection *> m_cons;  // 管理客户端连接
    TcpConnection * m_waitConn;
    boost::asio::io_service * m_ios;
    void _start();
    // 客户端连接时，回调
    void accept_handler(const boost::system::error_code & ec);
};

void TcpServer::_start(){
	m_waitCon = new TcpConnection(*m_ios);
	m_waitCon->addService(new EchoBackImplService(m_waitCon));//con有service的句柄。
	//目前只能接受一次连接 async_accept即为异步接口
	acceptor.async_accept(*m_waitCon->getSocket(), boost::bind(&TcpServer::accept_hander,this,boost::asio::placeholders::error));
}


// TcpClient
class TcpClient
{
public:
	TcpClient(io_service & io);
	TcpConnection * getConnection();
private:
	TcpConnection * m_con;
	ip::tcp::endpoint ep;
	void conn_hanlder(const boost::system::error_code & ec,TcpConnection * con);
};


// TcpConnection
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio/io_service.hpp>
#include <vector>
#include <google/protobuf/service.h>


using namespace boost::asio;
typedef boost::asio::ip::tcp::socket* sock_pt;
class TcpConnection:
	public google::protobuf::RpcChannel
{
public:
	TcpConnection(boost::asio::io_service & io);
	~TcpConnection();
	//发送数据
	void sendMessage(std::string str);
	//发送数据回调
	void write_handler(const boost::system::error_code &);
	//接收到数据
	void read_handler(const boost::system::error_code& ec,boost::shared_ptr<std::vector<char>> str);
	//rpc server service
	void addService(google::protobuf::Service *serv);
	sock_pt getSocket();
    void CallMethod(const google::protobuf::MethodDescriptor* method,
                              google::protobuf::RpcController* controller,
                              const google::protobuf::Message* request,
                              google::protobuf::Message* response,
                              google::protobuf::Closure* done);
private:
	sock_pt _sock;
	std::vector<google::protobuf::Service*> rpcServices;
	//解析rpc string
	void deal_rpc_data(boost::shared_ptr<std::vector<char>> str);
};


void TcpConnection::sendMessage(std::string str){
	std::cout<<"发送:"<<str<<std::endl;
    // async_write_some 异步写
	_sock->async_write_some(
        boost::asio::buffer(str.c_str(), strlen(str.c_str())),
        boost::bind(&TcpConnection::write_handler, this,boost::asio::placeholders::error)
    );
}
void TcpConnection::read_handler(const boost::system::error_code& ec,boost::shared_ptr<std::vector<char>> str){
	deal_rpc_data(str);
	boost::shared_ptr<std::vector<char>> str2(new std::vector<char>(100,0));
    // async_read_some 异步读
	_sock->async_read_some(
        boost::asio::buffer(*str2),
        boost::bind(&TcpConnection::read_handler,this,boost::asio::placeholders::error,str2));
}
```

[Python_RPC](https://github.com/yingshin/Tiny-Tools/tree/master/protobuf-rpc-demo)

[C++版同步RPC实现](https://izualzhy.cn/demo-protobuf-rpc)

[C++版异步RPC实现](https://github.com/shinepengwei/miniRPC)
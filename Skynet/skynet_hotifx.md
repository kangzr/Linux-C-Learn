线程间通信，使用管道；

比如work线程和socket线程间的通信

网络编程：单线程读，多线程写

skynet哪些地方需要向epoll进行注册？123 `sp_add就需要绑定通过void*ud`

- connectfd
- clientfd worker
- listenfd clientfd=accept()
- pipe读端，worker线程往管道写端写数据，socket线程在管道读端读数据

main.lua就是一个actor

客户端连接skynet：`new_fd`

listenfd与actor进行绑定



Redis epoll中通过event.fd进行绑定





skynet 业务是由lua来开发，与底层沟通以及计算密集的都需要用c

提升：lua调用c，c怎么调用lua，才能看懂所有的skynet代码；

目的：openresty（nginx+lua，将lua嵌入到nginx中，是的开发更加简单），nginx开发（filter，handler，upstream）




















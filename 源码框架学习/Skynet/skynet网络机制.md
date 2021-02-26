服务模块要将数据，通过socket发送给客户端时，并不是将数据写入消息队列，而是通过管道从worker线程，发送给socket线程，并交由socket转发。











[from](https://manistein.github.io/blog/post/server/skynet/skynet%E7%BD%91%E7%BB%9C%E6%9C%BA%E5%88%B6/)

#### Listen整体流程

我们在创建watchdog服务的时候，watchdog服务在执行初始化流程时，会创建一个新的gate服务，并且将main服务里填写的ip和port传给gate服务，gate服务收到后就要开始监听该ip和端口

![skynet_listen](..\..\pic\skynet_listen.png)

gate服务逻辑在work线程，socket逻辑在socket线程，worker线程和socket线程通过管道pipe进行通信，即worker线程写入(通过sendctrl_fd)，socket线程读取(通过recvctrl_fd)，pipe保障了worker线程向socket线程写入请求的原子性。



#### Accept整体流程

绑定和监听一个ip和端口的socket被创建以后，我们就可以处理连接事件了，假设我们现在有个客户端，向我们的skynet服务器发起了连接请求，流程如下

![skynet_accept](..\..\pic\skynet_accept.png)

epoll监听io事件到来，唤醒socket线程，

![skynet_accept_2](..\..\pic\skynet_accept_2.png)



#### Read整体流程

读取数据包的整体流程，其实比较简单，当客户端向skynet服务端发送数据包的时候，socket线程则会收到client_fd的EPOLLIN事件，此时，socket线程则会从buffer中读取client_fd的数据包，因为客户端的应用层数据包很可能被分割成若干个segment，因此，socket线程则一次能读多少，就给gate服务push多少，gate服务则会进行分包和粘包处理，在收齐完整的数据包后，再将完整的complete数据包转发给agent服务。

![skynet_read](..\..\pic\skynet_read.png)

#### Write整体流程

agent发送数据包，则要经历两个步骤，第一个步骤是将数据包发送给socket线程，socket线程收到后，将发送的buffer放到skynet socket实例的send buffer列表中，此时注册client_fd的EPOLLOUT事件，等待数据包可发送的时候，将send buffer里的数据包发送给客户端。

![skynet_write](..\..\pic\skynet_write.png)

 当client_fd可写时，epoll实例会唤醒socket线程，此时socket线程，会将client_fd的发送队列中，取出头部的buffer，并将其写入到内核中，他有可能一次写不完，不过作为整体流程，这里做最简化处理，详细内容我们将在后续进行讨论，数据包在写入内核后，会在合适的时机，发送数据包给客户端。此外，每个skynet socket数据实例，都会有一个高优先级队列和低优先级队列，skynet从1.2开始还增加了网络并发处理的机制

![skynet_send](..\..\pic\skynet_send.png)

#### Close整体流程










































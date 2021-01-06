TCP可靠性表现在：

- 对每个包提供校验和
- 包的序列号解决了乱序，重复问题
- 超时重传机制
- 流量控制，拥塞控制机制

TCP面向字节流：没有固定报文边界

TCP全双工：每一端都可以是主动端或者被动端



packetdrill原理：在执行脚本前创建了一个名为tun0的虚拟网卡，脚本执行完后，tun0会被销毁。该虚拟网卡对应于操作系统中/dev/net/tun文件，每次程序通过write等系统调用将数据写入到这个文件fd时，这些数据会经过tun0这个虚拟网卡，将数据写入到内核协议栈，read系统调用读取数据的过程类似。协议栈可以向操作普通网卡一样操作虚拟网卡tun0.



TCP包头PSH标记：告知对方这些数据包收到以后应该马上交给上层应用，不能缓存起来

数据链路层传输的帧大小限制为MTU，最大帧尾1518字节，出去14字节头部(以太网帧头)和4字节CRC，有效荷载为1500字节，这个值就是MTU，因此传输100KB数据，至少需要(100 * 1024 / 1500)= 69个以太网帧

<img src="..\pic\tcpip_mtu.png" alt="tcpip_mtu" style="zoom:75%;" />

网络中的木桶效应：路径MTU，一个包从发送端传输到接收端，中间要跨越多个网络，每条链路的MTU都可能不一样，这个通信过程中最小的MTU称为路径MTU。

MSS(1460字节) = MTU(1500字节) - IP头(20字节) - TCP头(20字节)

##### 为什么有时候抓包可看到单个数据大于MTU？

TSO(TCP Segment Offload)特性，指由网卡代替CPU实现packet的分段和合并，节省系统资源。因此TCP可抓到超过MTU的包，但不是真正传输的单个包会超过链路的MTU。tcpdump抓包的点。

MTU是数据链路层的概念。

MSS是传输层的概念（最大报文段长度）



分层结构中每一层都有一个唯一标识，链路层为MAC地址，网络层为IP地址，传输层为端口号（传输层用端口号来区分同一个主机上不同的应用程序）



端口相关命令

**如何查看对方端口是否打开**

`telnet ip port`

`nc -v ip port`

**如何查看端口被什么进程监听占用**

`netstat -tnlp | grep 22`

使用lsof因为Linux上一切皆文件，Tcp Socket连接也是一个fd，因此可用。-n表示不将IP转为hostnum，-P不将port转为service name，-i:port

`lsof -n -P -i:22`

**如何查看进程监听的端口号**

第一步`ps -ef | grep sshd` 找到sshd进程号pid

第一种方法：`netstat -atnp | grep pid`

第二种：`lsof -n -P -p pid | grep TCP`





![tcp_shake_handle](..\pic\tcp_shake_handle.png)



<img src="..\pic\tcp_ip_closing.png" alt="tcp_ip_closing" style="zoom:75%;" />

![tcp_ip_state](..\pic\tcp_ip_state.png)





服务端调用listen函数时，TCP状态从CLOSE变成LISTEN，同时在内核创建了两个队列

- 半连接队列（SYN队列）
- 全连接队列（Accept队列）(accept系统调用从全连接队列取出连接信息)

<img src="..\pic\tcp_syn_queue.png" alt="tcp_syn_queue" style="zoom:75%;" />





tcp_abort_on_overflow参数

- 0，全连接队列满后，server会丢掉client发过来的ACK，随后重传SYN+ACK
- 1，全连接队列满后，服务端直接发送RST给客户端

##### 如何应对SYN Flood攻击

增加SYN连接数：tcp_max_syn_backlog

减少SYN+ACK重试次数：tcp_synack_retries

SYN Cookie机制：tcp_syncookies, 原理为最后阶段才分配连接资源，服务端收到SYN包后，根据这个包计算一个Cookie值，作为握手第二步的序列号回复SYN+ACK，等对方回应ACK包时校验回复的ACK值是否合法，合法才握手成功分配资源。具体实现`linux/syncookies.c`



##### TFO(TCP Fast Open)快速打开

分为两阶段：请求Fast Open Cookie和TCP Fast Open

- 客户端发送一个SYN包，头部包含Fast Open选项，且该选项的Cookie为空，这表明客户端请求Fast Open Cookie
- 服务端收到SYN包后，生成一个cookie值
- 服务端发送SYN+ACK包，在Options的Fast Open选项中设置cookie的值
- 客户端缓存服务端的IP和收到的cookie值
- 进行数据传输...

之后，客户端有了缓存在本地的cookies值：

- 客户端发送SYN数据包，里面包含**数据**和之前缓存在本地的Fast Open Cookie（之前SYN包不含数据）
- 服务都那检验TFO Cookie和是否合法，合法则返回SYN+ACK+数据
- 客户端发送ACK包
- 进行数据传输...

##### 参数SO_REUSEADDR参数

端口复用，四元组确定一个socket；

模拟FIN_WAIT2状态：c1客户端，c2服务端，c1利用防火墙拦截所有发出的FIN包`sudo iptables --append OUTPUT --match tcp --protocol tcp --dport 8080 --tcp-flags FIN FIN --jump DROP`。

客户端用临时端口，因此不需要使用端口复用。



##### MSL：Max Segment LiftTime

报文最大生存时间是TCP报文在网络中的最大生存时间，这个值与IP报文头的TTL（经过一个路由器为一跳）字段有密切关系。



##### TCP头部时间戳选项(TCP Timestamps Option, TSopt)

除了MSS，Window Scale外，还有一个很重要的选项时间戳(TSopt)，它由四部分组成：类型kind，长度length，发送方时间戳TS value，回显时间戳TS Echo Reply

`net.ipv4.tcp_tw_reuse`和`net_ipv4.tcp_tw_recycle`都依赖timestamp

但是经过NAT或者负载均衡就麻烦了。

<img src="..\pic\tcp_nat.png" alt="tcp_nat" style="zoom:75%;" />

NAT（Network Address Translator）出现是为了缓解IP地址耗尽的临时方案。

- 出口IP共享：通过一个公网地址可以让许多机器连上网络，解决IP地址不够用的问题
- 安全隐私防护，实际的机器可以隐藏自己真实的IP。

当tcp_tw_recycle遇上nat，客户端出口IP一样，不同客户端携带的timestamp可能由差异，会导致丢包，甚至无法连接的情况



##### RST常见的几种情况

- 服务进程挂掉或者未启动，客户端connect时，会收到RST包，出现“Connection Reset”提示
- 用这种机制检测对端端口是否打开。

在RST没有丢失的情况下，发出RST以后服务端马上释放连接，进入CLOSED状态，客户端收到RST后，也立刻释放连接，进入CLOSED状态



##### 快速重传

当发送端收到3个或以上重复ACK，就意识到之前发的包可能丢了，马上重传。

##### SACK（Selective Acknowledgment）

只重传丢失的包



##### TCP滑动窗口

<img src="..\pic\tcp_socket.png" alt="tcp_socket" style="zoom:75%;" />

TCP把要发送的数据放入发送缓冲区(Send Buffer)，接收到的数据放入接收缓冲区(Receive Buffer)，应用程序会不停的读取接收缓冲区的内容进行处理

流量控制做的事情就是，如果接收缓冲区已满，发送端应该停止发送数据，为了控制发送端速率，接收端会告知客户端自己接收窗口rwnd，也就是接收缓冲区中的空闲部分。

<img src="..\pic\tcp_receive_win.png" alt="tcp_receive_win" style="zoom:75%;" />



##### 禁用Nagle算法

`setsockopt(..., TCP_NODELAY)`，通过设置TCP_NODELAY来禁用Nagle算法

##### 零窗口探测Zero Window Probe

如果窗口一直为0，会尝试探测，探测几次后关闭连接



##### TCP拥塞控制

流量控制只考虑发送端和接收端的情况，无法知道整个网络的通信状况。因此就需要拥塞控制，主要涉及以下几个算法

- 慢启动，(初始化拥塞窗口)，每收到ACK cwnd = cwnd+1, 每RTT过后，cwnd=cwnd*2；遇到慢启动阈值（ssthresh），则踩刹车进入拥塞避免阶段
- 拥塞避免，每一个往返RTT，cwnd+=1
- 快速重传，（正常重传需要等几百毫秒）当接收端收到一个不按序到达的数据段时，TCP立刻发送1个重复ACK，当发送端收到3个或以上重复ACK，就意识到之前发的包可能丢了，就立马重传，而不用等重传定时器超时再重传。并进入快速恢复阶段
- 快速恢复，解释为网络轻度拥塞，1. 拥塞阈值ssthread降低为cwnd的一半;2. 拥塞窗口cwnd设置为ssthresh 3. 拥塞窗口线性增加

##### 选择确认（Selective Acknowledgement，SACK）



##### TCP keepalive

##### RST攻击

伪造TCP重置报文攻击，通过伪造RST报文来关闭一个正常的连接。RST攻击最重要的一点就是构造的包的序列号要落在对方的滑动窗口内，否则这个RST包会被忽略掉。

##### 定时器类型

- 连接建立定时器
- 重传定时器：
- 延迟ACK定时器：
- PERSISTT定时器
- KEEPALIVE定时器：KeepAlive心跳包，多次探测后对方没有回复则关闭
- FIN_WAIT_2定时器：`/proc/sys/net/ipv4/tcp_fin_timeout`决定，timeout后还没有收到FIN，则主动关闭连接
- TIME_WAIT 定时器



##### telnet，netcat，netstat

```shell
netstat
-a : 命令可以输出所有套接字
-t : 只列出TCP的套接字
-u : 宣誓UDP连接
-l : 列出监听状态的连接
-p : 显示连接归属的进程信息
-i ：列出网卡信息，比如MTU
```



##### tcpdump

```shell
$ tcpdump -i any
-i 指定哪个网卡，any表示任意，有哪些网卡可以用ifconfig来查看
# 过滤主机：host选项
tcpdump -i any host 10.211.55.2  # 只查看10.211.55.2的网络包，ip可以源地址，也可以是目的地址
# 过滤源地址，目的地址：src，dst
tcpdump -i any src 10.211.55.10  # 只抓取hostnum发出的包
tcpdump -i any dst hostnum # 只抓取hostunm收到的包
# 过滤端口，port选项
tcpdump -i any port 80  # 只查看80端口
```





[实例参考](https://github.com/arthur-zhang/tcp_ebook)

##### packetdrill实例一

实际模拟路径MTU发现

```shell
0.000 setsocketopt(3, SOL_SOCKET, SO_REUSEADDR, [1], 4) = 0
0.000 bind(3, ..., ...) = 0
0.000 listen(3, 1) = 0

// 至此三次握手完成
0.100 < S 0:0(0) win 32792 <mss 1460,nop,wscale 7>
0.100 > S. 0:0(0) ack 1 <mss 14600,nop,wscale 7>
0.200 < . 1:1(0) ack 1 win 257
0.200 accept(3, ..., ...) = 4

// 发送第一个数据包
+0.2 write(4, ..., 14600) = 1460
// 断言内核会发送1460大小的数据包出来
+0.0 > P. 1:1461(1460) ack 1
// 发送ICMP错误报文，告知包太大，需要分片
+0.01 < icmp unreachable frag_needed mtu 1200 [1:1460(1460)]
// TCP立刻选择对方告知的较小MTU计算自己的MSS
+.0 > . 1:1161(1160) ack 1
+0.0 > P. 1161:1461(300) ack 1
// 确认所有的数据
+0.1 < . 1:1(0) ack 1461 win 257
+0 `sleep 1000000`
```



##### packetdrill实例二

构造一个SYN_SENT状态的连接

```shell
+0 socket(..., SOCK_STREAM, IPPROTO_TCP) = 3  // 新建一个server socket
+0 connect(3, ..., ...) = -1  // 客户端connect
```

执行netstat命令可以看到相关状态

```shell
netstat -atnp | grep -i 8080
tcp 0 ... SYN_SENT
```

SYN包重传了6次后关闭连接

```shell
cat /proc/sys/net/ipv4/tcp_syn_retries
```



##### packetdrill实例三

```shell
0 socket(..., SOCK_STREAM, IPPROTO_TCP) = 3  // 创建socket fd为3
+0 setsockopt(3, SOL_SOCKET, SO_REUSEADDR, [1], 4) = 0
+0 bind(3, .., ...) = 0  // 绑定fd为3的socket
+0 listen(3, 1) = 0  // 监听fd为3的socket
// 向协议栈注入SYN包
+0 < S 0:0(0) win 40000 <mss 1000>
// 检查协议栈返回的包是否符合预期
+0 > S. 0:0(0) ack 1 <...>
// 向协议栈注入ACK包
+.1 < . 1:1(0) ack 1 win 1000
+0 accept(3, ..., ...) = 4
+0 < P. 1:201(200) win 4000
+0 > . 1:1(0) ack 201
+0 `sleep 1000000`

```



##### packetdrill实例四

模拟同时关闭连接

```shell
0.000 socket(..., SOCK_STREAM, IPPROTO_TCP) = 3
0.000 setsockopt(3, SOL_SOCKET, SO_REUSEADDR, [1], 4) = 0
0.000 bind(3, ..., ...) = 0
0.000 listen(3, 1) = 0

+0 < S 0:0(0) win 65535 <mss 1460>
+0 > S. 0:0(0) ack 1 <...>
+0.1 < . 1:1(0) ack 1 win 65535
+0.010 accept(3, ..., ...) = 4

0.150 close(4) = 0
0.150 > F. 1:1(0) ack 1 <...>

// Client closes the connection
0.150 < F. 1:1(0) ack 2 win 65535

0.150 > .  2:2(0) ack 2 <...>

// Client sends an ACK
0.150 < . 2:2(0) ack 2 win 65535

+0 `sleep 1000000`
```


















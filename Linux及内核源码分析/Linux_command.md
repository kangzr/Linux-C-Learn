### 1. tcpdump  (dump the traffic on a network)

它使用 `libpcap` 库来抓取网络数据包，对网络上的数据包进行截获的包分析工具。 tcpdump可以将网络中传送的数据包的“头”完全截获下来提供分析。它支持针对网络层、协议、主机、网络或端口的过滤，并提供and、or、not等逻辑语句来帮助你去掉无用的信息



##### 监听指定网络接口的数据包

```shell
tcpdump -i eth1 # 指定eth1网卡
```

##### 监听指定主机的数据包

```shell
tcpdump host ip/hostname # 指定ip/hostname，截获所有ip的主机收到/发出的数据包

# 截获主机210.27.48.1 和主机210.27.48.2 或210.27.48.3的通信
tcpdump host 210.27.48.1 and \ (210.27.48.2 or 210.27.48.3 \) 

# 如果想要获取主机210.27.48.1除了和主机210.27.48.2之外所有主机通信的ip包
tcpdump ip host 210.27.48.1 and ! 210.27.48.2

# 截获主机hostname发送的所有数据
tcpdump -i eth0 src host hostname

# 监视所有送到主机hostname的数据包
tcpdump -i eth0 dst host hostname

```

##### 监听指定主机和端口的数据包

```shell
# 获取主机210.27.48.1接收或发出的telnet包
tcpdump tcp port 23 and host 210.27.48.1

# 对本机的udp 123 端口进行监视 123 为ntp的服务端口
tcpdump udp port 123 
```

##### tcpdump 与wireshark

Wireshark(以前是ethereal)是Windows下非常简单易用的抓包工具。但在Linux下很难找到一个好用的图形化抓包工具。
还好有Tcpdump。我们可以用Tcpdump + Wireshark 的完美组合实现：在 Linux 里抓包，然后在Windows 里分析包。

```shell
tcpdump tcp -i eth1 -t -s 0 -c 100 and dst port ! 22 and src net 192.168.1.0/24 -w ./target.cap
# -i eth1: 只抓接口eth1接口
# -t： 不显示时间戳
# -s 0： 抓取数据包时默认抓取长度68字节，加上-s 0后可以抓取完整的数据包
# -c 100: 抓取100个数据包
# dst port ! 22: 不抓取目标端口是22的数据包
# src net : 数据包的源网络地址为
# -w ./target.cap: 保存成cap文件，方便用ethereal(即wireshark)分析
```

##### TCP数据包

```shell
src > dst: flags data-seqno ack window urgent options

# src 和 dst 是源和目的IP地址以及相应的端口. 
# flags 标志由S(SYN)，F(FIN)，P(PUSH)，R(RST), W(ECN CWT)
```

example redis

`sudo tcpdump -i any dst host 127.0.0.1 and port 6379`

##### 抓取协议数据

```shell
# 抓取udp包
tcpdump -i eth0 udp
tcpdump -i proto 17
# 抓取tcp包
tcpdump -i eth0 tcp
tcpdump -i eth0 6
# 抓取icmp数据
tcpdump -n icmp
```



### 2. netstat

用于显示各种网络相关信息，如网络连接，路由表，接口状态 (Interface Statistics)，masquerade 连接，多播成员 (Multicast Memberships) 等等

从整体上看，netstat的输出结果可以分为两个部分：

- Active Internet connections ：

  有源TCP连接，其中`Recv-Q`和`Send-Q`指接收队列和发送队列，这些数字一般是0，如果不是，则表示软件包正在队列堆积。

  <img src="..\pic\netstate_aic.png" alt="netstate_aic" style="zoom:75%;" />

- Active UNIX domain sockets：

  有源Unix域套接字（和网络套接字一样，只能用于本机通信，性能可提高一倍）；

  **Proto**显示连接使用的协议；**RefCnt**表示连接到本套接口上的进程号；**Types**显示接口好类型，**State**显示套接字当前状态，**Path**表示连接到套接口的其它进程使用的路径名

<img src="..\pic\netstate_unix.png" alt="netstate_unix" style="zoom:75%;" />

**常见参数：**

```shell
-a (all)显示所有选项，默认不显示LISTEN相关
-t (tcp)仅显示tcp选项
-u (udp)仅显示udp选项
-n 拒绝显示别名，能显示数字的全部转化为数字
-l 仅列出由listen的服务状态
-p 显示建立相关连接的进程
-r 显示路由信息，路由表
-e 显示扩展信息
-s 按各个协议进行统计
-c 每隔一个固定时间，执行改netstate
```



### 3.  top

![top_command](..\pic\top_command.png)

**第一行**：时间和负载信息

系统当前时间，系统已经运行时间，当前连接系统的终端数(users)，平均负载(0.18, 0.15, 0.17, 表示1分钟，5分钟，15分钟的CPU负载，单核满载为1，N个核满载为N)

**第二行**：进程信息

总进程数(total)，正在运行的进程数(running)，正在睡眠(sleeping)，停止的进程数(stopped)，僵死的进程数(zombie)

**第三行**：CPU状态信息

当前显示CPU的平均值，若要看某个cpu的情况，按1即可

用户态进程占用CPU百分比(us)，内核占用CPU百分比(sy)，改变过优先级的进程占用cpu百分比(ni)，空闲CPU时间百分比(id)，等待I/O的CPU百分比(wa)，

CPU硬/软中断占用CPU百分比(hi/si)；

**第四行**：物理内存使用情况

物理内存总量(total)，空闲的物理内存(free)，使用的物理内存量(used)，用作内核缓存的物理内存量(buff/cache)；

**第五行**：交换区使用信息

交换区总大小(swap)，空闲的交换区大小(free)，使用的交换区大小(used)，可用物理内存大小(avail Mem)

**第六行**：进程的状态信息

进程的ID(PID)，进程所有者(USER)，进程优先级(PR)，nice值(NI)，进程占用的虚拟内存总量(VIRT)，进程占用的物理内存总量(RES, 包含SHR)，进程使用的共享内存总量(SHR)，进程状态(S, D=不可中断睡眠状态，R=运行，S=睡眠，T=跟踪/停止，Z=僵死进程)，进程使用的CPU百分比(%CPU)，进程使用的物理内存百分比(%MEM)，进程总共使用的CPU时间(TIME+)

**常用交互命令**

退出(q)，立即刷新(<space>)，显示完整的命令(c)，按%CPU从大到小(P)，按%MEM从大到小(M)，杀死指定进程(k)，显示所有线程信息(H)



查看进程情况

```shell
pidstat -urd -p 33078
ps -Hp pid
```



### 4. ps

Process Status；

ps命令用来列出系统中当前运行的那些进程。ps命令列出的是当前那些进程的快照，就是执行ps命令的那个时刻的那些进程，如果想要动态的显示进程信息，就可以使用top命令。

使用该命令可以确定有哪些进程正在运行和运行的状态、进程是否结束、进程有没有僵死、哪些进程占用了过多的资源等等

**linux上进程有5种状态**：

- 运行：runnable (on run queue)
- 中断：(休眠中sleeping)
- 不可中断：uninterruptible sleep 收到信号不唤醒和不可运行，进程必须等待直到有中断发生
- 僵死：zombie 进程已终止，但进程描述符存在，直到父进程调用wait4()系统调用后释放
- 停止：stopped 进程收到SIGSTOP, SIGSTP, SIGTIN, SIGTOU信号后停止运行运行

**命令参数**

```shell
-a/A/e 显示所有进程
-f 显示程序间的关系
-H 显示树状结构
-u 指定用户的所有进程
-aux 显示所有包含其它使用者的行程
```

**实例**

```shell
ps -ef | grep ssh
ps -aux # 所有在内存当中的程序
ps -u root # 显示指定用户信息
ps -A # 显示所有进程信息
```



### 5. nohup

no hang up(不挂起)，用于系统后台不挂断运行命令，退出终端后不会影响程序的运行；

```shell
nohup /root/runoob.sh &
ps -aux | grep "runoob.sh" 
# kill -9 PID
# 2>&1 :将标准错误 2 重定向到标准输出 &1

```



### 6. grep

```shell
-A n # 除了匹配行，并显示该行之后的n行
-b n # 除了匹配行，并显示该行之前的n行
-C n # 除了匹配行，并显示前后n行
-v  # 反转查找
-c  # 统计匹配行数
--include  # 包含文件
--exclude  # 排除文件

# 查看cpu信息: cat/proc/cpuinfo
# 直接获取cpu核数：grep "model name" /proc/cpuinfo | wc -l
```



### 7. awk



### 8. xargs



### sed



### head和tail

tail -f file：只显示增量文件数据

### find

```shell
find -iname login
```



### ulimit -a

用于查看文件描述符，栈大小等最大限制为多少



### free -h

```shell
available = free + buffer + cache
buffer： 攒数据，批量写入，比如硬盘数据的写入
cache：低速设备到高速设备的缓存

```

### df  -h

磁盘信息

swap分区：内存不够用时，把一些数据放到硬盘去， 下次访问不在物理内存则从硬盘去加载。

> telnet明码传输，ssh加密传输

### nc （netcat） 

命令丰富，短小精悍，简单实用，能通过TCP和UDP在网络中读写数据。非常强大，可在两台电脑之间建立连接并返回两个数据流；

可建立一个服务器，传输文件，与朋友聊天，传输流媒体等。

```c
nc -l 9999
```



```shell
# 1. 聊天
# 主机A建立服务器 ip 192.168.x.x port 1567  
nc -l 1567  

# 主机B作为客户端连接
nc ip port

# 这样两者之间可进行聊天

# 2. 文件传输
# server
$nc -l 1567 < file.txt
# client
$nc -n ip port > file.txt

# 3. 远程连接
$nc ip port
```



### lsof  

list open files，列出当前系统打开文件的工具；一切皆文件

```shell
lsof -i:9999
```



##### 查看物理CPU个数，核数

```shell
#总核数 = 物理CPU个数 X 每颗物理CPU的核数 

# 查看物理CPU个数  (2)
cat /proc/cpuinfo | grep "physical id" | sort | uniq | wc -l
# 查看每个物理CPU中core的个数（即核数）(8核)
cat /proc/cpuinfo | grep "cpu cores" | uniq

# 查看物理内存  100G
cat /proc/meminfo
```



### cut

```shell
# cut取行中特定数据域
-d : 后面接分割字符，与-f一起使用
-f : 依据-d的分割字符的切分，取出对应列
-c : 取出第几个字符
-b : 以字节为单位进行分割


```











日志格式

````
[g55-gs21-10015.i.nease.net] Feb  4 09:07:00 g55-gs21-10015 supervisord: game351 2021-02-04 09:07:00,044384 - game351 - Avatar - INFO - 乄清丨欢丷(V56m5BM4ICQe7aOC) updateSubmitItemScore addValue(10), submitItemScore(10)
````

取出要用信息

```shell
# awk -F指定分隔符，默认空格
$ cat g55_logtail.log | awk '{print $1, $17, $20}'
[g55-gs17-10015.i.nease.net] 魑心(Xe87Eg2tmBBGLOYL) submitItemScore(10)
```

去掉括号，留服务器信息，名字，ID，对应数据

```shell
$ awk -F"[()]" '{print$1,$2,$4}'
[g55-16066-gm-35397fce-product/10.211.199.230] 温酒相随丶 X9JBEcfx0CDJOZDj 4900
```

按照第n列进行过滤，(按照第n列进行排序)

```shell
# 取出第4列大于10000, 并按照第4列进行排序， 倒序使用(sort -k4,4nr)
awk '$4>10000{print$1,$2,$4}' | sort -k4  
```

根据指定的列去重

```shell
# 根据第3列去重
sort -k3,3 -u
```



```shell
cat g55_logtail.log | awk '{print$1,$17,$20}' | awk -F"[()]" '$4>10000{print$1,$2,$4}' | sort -k4 | sort -k3,3 -u
```




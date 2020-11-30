

#### Nginx安装

需要安装组件

1. openssl

   openssl是一个强大的安全套接字密码库，囊括密码算法、常用密钥和证书封装管理功能以及SSL协议；并提供丰富的应用程序测试；nginx不仅支持http，还支持https

   ````shell
   # tar zxvf openssl-fips*.tar.gz
   # cd openssl-fips
   # tar zxvf openssl-fips*.tar.gz
   
   # 以下三个组件同
   ````

2. zlib

   zlib提供了多种压缩和解压方式，nginx使用zlib对http包的内容进行gzip

3. pcre

   Perl Compatible Regular Expressions，包括perl兼容的正则表达式库，nginx的http模块使用pcre来解析正则表达式

4. nginx

Nginx工作原理及安装配置

Nginx由内核和模块组成，其中内核设计非常微小和简洁，完成工作也简单，仅通过查找配置文件将客户端请求映射到一个location block（location是Nginx配置中的一个指令，用于URL匹配）；

Nginx模块从结构分为：

- 核心模块：HTTP模块、EVENT模块和MAIL模块
- 基础模块：HTTP Access模块，HTTP FastCGI模块，HTTP Proxy模块和HTTP Rewrite模块
- 第三方模块：HTTP Upstream Request Hash 模块、Notice模块和HTTP Access Key模块

Nginx的高并发得益于采用了epoll模型，与传统服务器程序架构不同；Nginx采用epoll模型，异步非阻塞，而Apache采用的Select模型；



#### Nginx简介

Nginx是一个web服务器，也可用来做负载均衡及反向代理，目前使用最多的就是负载均衡；

是一款高性能的http服务器/反向代理服务器及电子邮件(POP3/IMAP)代理服务器；官方测试支持5万并发连接，且cpu、内存等资源消耗非常低，运行非常稳定；

（占用内存少，并发能力强，Nginx由内核和模块组成，其内核设计非常微小和简洁）



**与Apache相比：**

- 高并发响应性能非常好，官方Nginx处理静态文件5w/s
- 反向代理能力非常强（可用于负载均衡）
- 内存和cpu占用率低
- 对后端服务由健康检查功能
- 支持PHP cgi方式和fastcgi方式
- 配置代码简洁且容易上手



**应用场景：**

- http服务器，nginx是一个http服务可以独立提供http服务，可做网页静态服务器
- 虚拟主机，可实现一台服务器虚拟多个网站；例如个人网站使用的虚拟主机
- 反向代理，负载均衡；当网站的访问量达到一定程度后，单台服务器不能满足用户的请求时，需多台服务器集群可以使用nginx做反向代理，并且多台服务器可以平均分担负载，不会因为某台服务器高宕机而某台服务器闲置的情况。



##### 13.1.0 请解释一下什么是Nginx?

##### 13.1.1 请列举Nginx的一些特性?

##### 13.1.2 请列举Nginx和Apache 之间的不同点?

##### 13.1.3 请解释Nginx如何处理HTTP请求。

##### 13.1.4 在Nginx中，如何使用未定义的服务器名称来阻止处理请求?

##### 13.1.5 使用“反向代理服务器”的优点是什么?

##### 13.1.6 请列举Nginx服务器的最佳用途。

##### 13.1.7 请解释Nginx服务器上的Master和Worker进程分别是什么?

##### 13.1.8 请解释你如何通过不同于80的端口开启Nginx?

##### 13.1.9 请解释是否有可能将Nginx的错误替换为502错误、503?

##### 13.2.0 在Nginx中，解释如何在URL中保留双斜线?

##### 13.2.1 请解释ngx_http_upstream_module的作用是什么?

##### 13.2.2 请解释什么是C10K问题，后来是怎么解决的？

##### 13.2.3 请陈述stub_status和sub_filter指令的作用是什么?

##### 13.2.4 解释Nginx是否支持将请求压缩到上游?

##### 13.2.5 解释如何在Nginx中获得当前的时间?

##### 13.2.6 用Nginx服务器解释-s的目的是什么?

##### 13.2.7 解释如何在Nginx服务器上添加模块?

##### 13.2.8 nginx中多个work进程是如何监听同一个端口的？如何处理客户连接的惊群问题？

##### 13.2.9 nginx程序的热更新是如何做的？






































### Skynet Socket线程

```c
// 
struct skynet_socket_message {    //发送到 skynet 各个服务去的套接字消息
    int type;        //套接字消息的类型，取上面的预定义值
    int id;            //定位存储套接字信息的id
    int ud;            //套接字消息的数据的大小
    char * buffer;    //套接字消息的数据
};

//全局的信息
struct socket_server {
    int recvctrl_fd;                    //读管道fd
    int sendctrl_fd;                    //写管道fd
    int checkctrl;                        //默认值为1，是否需要检查管道中的命令的标记
    poll_fd event_fd;                    //epoll句柄
    int alloc_id;                        //当前分配到的socket ID
    int event_n;                        //epoll触发的事件数量
    int event_index;                    //当前已经处理的epoll事件的数量
    struct socket_object_interface soi;    //初始化发送对象时用
    struct event ev[MAX_EVENT];            //事件的相关数据
    struct socket slot[MAX_SOCKET];        //所有套接字相关的信息
    char buffer[MAX_INFO];                //open_socket发起TCP连接时，用于保存套接字的对端IP地址，如果是客户端套接字保存客户端的ip地址和端口号
    uint8_t udpbuffer[MAX_UDP_PACKAGE];    //接收UDP数据
    fd_set rfds;                        //select的读描述符集合
};
```


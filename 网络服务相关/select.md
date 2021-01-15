select

```c
// 原型
int select(int numfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
/*
 * 参数含义如下
 * numfds: 		需要监听的最大文件描述符 + 1
 * readfds: 	需监听的读文件描述符(fd)集合
 * writefds: 	需监听的写文件描述符(fd)集合
 * exceptfds:	需监听的异常文件描述符(fd)集合
 * timeout: 	
 * 		1. NULL则永远等待，直到信号或文件描述符准备好；
 *		2. 0: 从不等待，测试所有执行的fd立即返回
 *		3. >0: 等待timeout时间，还没有fd准备好，立即返回
 *
 * return：返回三个fd集合(readfds, writefds, exceptfds)中fd总数；
 */
// fd处理函数
FD_ZERO(fd_set *set);			// 清除fd_set
FD_SET(int fd, fd_set *set);	// 将fd加入set，被监听
FD_CLR(int fd, fd_set *set);	// 将fd从set中移除
FD_ISSET(int fd, fd_set *set);	// 测试set中的fd是否准备好， 测试某个位是否被置为。
```

1.假设fd_set为一个1字节，每一个bit对应一个文件描述符，通过FD_SET将readfds中3，4，5，7 文件描述符设为1，表示对这三个文件描述符的读事件感兴趣，如下图：

![select_readfds](D:\MyGit\Linux-C-Learn\pic\select_readfds.png)

2.调用select，将readfds copy到内核空间，并poll感兴趣的fd，因为需要监听的最大文件描述符为6，因此select的第一个参数为7，内核只需要关心<7的文件描述符即可，此时文件描述符5，7有读事件到来，内核将对应位置为1并将结果返回用户空间，这时用户的readfds变成了下图：

![select_readed](D:\MyGit\Linux-C-Learn\pic\select_readed.png)

3.用户空间遍历readfds，通过FD_ISSET对应的fd是否置位，如果置位则调用read读取数据。



优点：可以监听多个文件描述符

缺点：

- 最大可监听文件描述符有上限，由fd_set决定
- 需要将fd_set在用户态和内核态之间进行copy，开销大
- 无法精确知道哪些fd准备就绪，需要遍历fd_set并通过FD_ISSET来检测，事件复杂度为O(n)

Linux 2.6版本

```c
// unistd.h 系统调用编号
#define __NR_select	82 
#define __NR_epoll_create  254
#define __NR_ctl 255
#define __NR_wait 256
```

select为系统调用，其系统调用的编号为 82，用户态执行系统调用会触发0x80中断，并将其编号，以及相关参数传入内核空间，内核态根据其系统调用编号在`sys_call_table`找到对应接口`sys_select`

```c
typedef struct {
    unsigned long fds_bits [__FDSET_LONGS];
} __kernel_fd_set;

typedef __kernel_fd_set	fd_set;

/*
* __copy_to_user: copy a block of data into user space, with less checking.
* @to: Destination address, in user space.
* @from: Source address, in kernel space.
* @n: Number of bytes to copy
*/
static inline unsigned long
__copy_to_user(void __user *to, const void *from, unsigned long n){
    ...
    return __copy_to_user_ll(to, from, n);
}
/*
* __copy_from_user: copy a block of data from user space, with less checking.
* @to: Destination address, in kernle space.
* @from: Source address, in data space.
* @n: Number of bytes to copy
*/
static inline unsigned long
__copy_to_user(void *to, const void __user *from, unsigned long n){
    ...
    return __copy_from_user_ll(to, from, n);
}
// poll.h
typedef struct {
    unsigned long *in, *out, *ex;  // 需要关注的fds
    unsigned long *res_in, *res_out, *res_ex;  // 保存结果
} fd_set_bits;
// select.c
asmlinkage long
sys_select(int n, fd_set __user *inp, fd_set __user *outp, fd_set __user *exp, struct timeval __user *tvp) {
    fd_set_bits fds;
    // get_fd_set 会调用__copy_from_user接口将用户态关心的fds(inp, outp, exp)copy到内核空间fds中
    get_fd_set(n, inp, fds.in);
    get_fd_set(n, outp, fds.out);
    get_fd_set(n, exp, fds.ex);
    // 清空结果
    zero_fd_set(n, fds.res_in);
    zero_fd_set(n, fds.res_out);
    zero_fd_set(n, fds.res_ex);
    // 真正select, 会poll fds中需要关注的fd，如果有事件到达则保存至结果字段
    ret = do_select(n, &fds, &timeout);
    // set_fd_set 会调用__copy_to_user接口将事件结果从内核空间 fds copy到用户空间
    set_fd_set(n, inp, fds.res_in);
    set_fd_set(n, outp, fds.res_out);
    set_fd_set(n, exp, fds.res_ex);
}

```

example

```c
int main(){
    // 创建服务器进行监听
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bind(sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
   
    listen(sockfd, 5);
    fd_set rfds, rset;
    FD_ZERO(&rfds);             // 清空rfds, rfds保存需要监听的fd
    FD_SET(sockfd, &rfds);      // 将sockfd加入rfds集合中
    int max_fd = sockfd;        // 最新生成的sockfd 为最大fd
    
    while (1) {
        rset = rfds;  // 将需要监听的rfds copy到rset中
        int nready = select(max_fd + 1, &rset, NULL, NULL, NULL);
        if (FD_ISSET(sockfd, &rset)) {  // 检测sockfd是否有事件到来, 即是否有客户端连接
            struct sockaddr_in client_addr;
            memset(&client_addr, 0, sizeof(struct sockaddr_in));
            socklen_t client_len = sizeof(client_addr);
              // accept客户端，连接成功后分配clientfd
            int clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);
            FD_SET(clientfd, &rfds);    // 将clientfd加入rfds集合中
            // 处理完一个fd，则更新nready值，直到为0则表示全部处理完，然后退出
            if (--nready == 0) continue;
        }

        // 遍历所有的文件描述符, 因此select时间复杂度为O(n)
        for (i = sockfd + 1; i <= max_fd; i ++) {
            if (FD_ISSET(i, &rset)) {
                char buffer[BUFFER_LENGTH]= {0};
                int ret = recv(i, buffer, BUFFER_LENGTH, 0);
                if (ret < 0) {
                    // 多个线程同时操作，可能导致数据被其它线程读取完
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        printf("read all data");
                    }
                    FD_CLR(i, &rfds);   // 将文件描述符i从rfds中移除
                    close(i);
                } else if (ret == 0) {
                    // 断开连接
                    printf("disconnect %d\n", i);
                    FD_CLR(i, &rfds);
                    close(i);
                    break;
                } else {
                    printf("Recv: %s, %d Bytes\n", buffer, ret);
                }
                // 更新nready
                if (--nready == 0) break;
            }
        }
    }
    return 0;
}
```




























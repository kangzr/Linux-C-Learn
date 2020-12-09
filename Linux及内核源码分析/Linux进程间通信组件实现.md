![linux_study](..\pic\linux_study.png)

IPC组件：管道，命名管道，消息队列，信号量，共享内存，信号





### 一. 共享内存

##### 共享内存段被映射进进程空间之后，存在于进程空间的什么位置？共享内存段最大限制是多少？

共享内存为最快的IPC，**同一块物理内存**被映射到进程A、B各自的进程地址空间，由于多个进程共享同一块内存区域，必然需要某种同步机制，互斥锁和信号量都可以；



<img src="..\pic\linux_shared_mm.png" alt="linux_shared_mm" style="zoom:75%;" />



##### Linux2.2.x内核支持多种共享内存方式

mmap()系统调用，Posix共享内存，以及系统V共享内存

**系统V共享内存**

- 进程间需要共享的数据被放在一个叫做IPC共享内存区域的地方，所有需要访问该共享区域的进程都要把该共享区域映射到本进程的地址空间中去；
- 系统V共享内存通过shmget获得或创建一个IPC共享内存区域，并返回相应的标识符。
- 所有这一切都是系统调用shmget完成的。

**mmap原理**

- 把一个文件映射到进程的地址空间（进程使用的虚拟内存），这样进程就可以通过读写这个进程地址空间来读写这个文件
- mmap的优点主要在为用户程序随机的访问,操作,文件提供了一个方便的操作方法

**使用内存映射文件来处理大文件可以提高效率。**

- 不使用内存映射文件：先读磁盘文件的内容到内存（copy到内核空间的缓冲区，再copy到用户空间），修改后再写回磁盘（同样需要两次copy）共需要4次数据拷贝
- 使用内存映射文件：减少不必要的数据拷贝，由mmap将文件直接映射到用户空间，mmap并没有进行数据拷贝，真正的数据拷贝是再缺页中断处理时进行，由于mmap将文件直接映射到用户空间，所以中断处理根据这个映射关系，直接将文件从硬盘拷贝到用户空间，只进行了依次数据拷贝。



Nginx中提供了三种实现方法

```c
// 真正操作共享内存的对象
typedef struct {
    u_char		*addr;  // 共享内存的开始地址，使用ngx_slab_pool_t
    size_t		size;  // 共享内存大小
    ngx_str_t	name;  // 共享内存名字
    ngx_log_t	*log;
    ngx_uint_t	exists;  // 是否存在
}ngx_shm_t;
ngx_int_t ngx_shm_alloc(ngx_shm_t *shm);
void ngx_shm_free(ngx_shm_t *shm);


#if (NGX_HAVE_MAP_ANON)  // 第一种 mmap 使用MAP_ANON 创建一块共享内存
ngx_init_t
ngx_shm_alloc(ngx_shm_t *shm)
{
    shm->addr = (u_char *)mmap(NULL, shm->size,
                              PROT_READ|PROT_WRITE,
                              MAP_ANON|MAP_SHARED,  -1, 0);
    if (shm->addr == MAP_FAILED) {
        return NGX_ERROR;
    }
    return NGX_OK;
}

// 销毁共享内存
void
ngx_shm_free(ngx_shm_ *shm) {
    munmap((void*) shm->addr, shm->size);
}

#elif (NGX_HAVE_MAP_DEVZERO)   //  第二种 mmap 映射文件
ngx_int_t
ngx_shm_alloc(ngx_shm_t *shm)
{
    ngx_t fd;
    fd = open("/dev/zero", O_RDWR);
    shm->addr = (u_char *)mmap(NULL, shm->size, PROT_READ|PROT_WRITE,
                              MAP_SHARED, fd, 0);
}

#elif (NGX_HAVE_SYSVSHM)   // 第三种 shmget 系统V共享内存
ngx_int_t
ngx_shm_alloc(ngx_shm_t *shm){
    int id;
    id = shmget(IPC_PRIVATE, shm->sizse, (SHM_R|SHM_W|IPC_CREAT));
    shm->addr = shmat(id, NULL, 0);
}
```



##### slab实现



### 二. 实现一个进程间通信组件

进程广播，一个进程写，多个进程读；采用共享内存来实现（相当于NIO，因此需要自己实现一个相当于异步IO的方式）

相当于Andriod的Binder；（四大组件Activity，Service，Provider，Binder）



字符设备/模块（少量存储）；块设备/模块（数量大）

一切皆文件：源于file_operations结构体





insmod  *.ko  把.ko文件加入内核

```c
// 采用vmalloc，内存不会涨，vmalloc分配完后，等用到的时候才给(物理)内存
// kmalloc会涨内存；
while(1) malloc(1k);

while(1){
    char * p = malloc(1k);
    p[5] = 'a'; // 如果对每一位进行赋值的话，涨；
}
```



file：文件属性

inode：文件具体数据



copy_to_user：内核空间到用户空间（内核空间先到寄存器(cpu)，再通过中断返回到用户空间）

copy_from_user：用户空间到内核空间



memcpy区别？（不经过寄存器，纯内存操作，不需要cpu）

```c
struct channel {
    char *data;
    unsigned long size;
    wait_queue_head_t inq;
};

int channel_open(struct inode *inode, struct file *filp) {
	struct channel *channel;
    int num = MINOR(inode->i_rdev);
    channel = &channel_devp[num];
    filp->private_data = channel;
    return 0;
}

int channel_release(struct inode *inode, struct file *file){
}

ssize_t channel_read(struct file *filp, char __user *buffer, size_t size, loff_t *ppos) {
    wait_event_interruptible(channel->inq, have_data);
    copy_to_user(buffer, (void*)(channel->data+p), count);
}
ssize_t channel_write(struct file *filp, const char __user *buffer, size_t size, loff_t *ppos) {
    copy_from_user(channle->data+p, buffer, count);
 
  	wake_up();
}

static const struct file_operations channel_fops = {
    .owner = THIS_MODULE,
    .read = channel_read,
    .write = channel_write,
    .open = channel_open,
    .release = channel_release,
    .poll = channel_poll,
    .mmap = channel_mmap,
}


static int channel_init(void) {
    int result, i;
    dev_t devno = MKDEV(channel_major, 0);
    result = register_chrdev_region(devno, NR_DEVS, "channel");
    cdev_init(&cdev, &channel_fops);
    cdev_add(&cdev, MKDEV(channel_major, 0), NR_DEVS);
    channel_devp = kmalloc(NR_DEVS * sizeof(struct channel), GFP_KERNERL);
    memset(channel_devp, 0, sizeof(struct channel));
}

```












































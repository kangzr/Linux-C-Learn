#### Linux内核如何学习

- 虚拟机安装linux，`kernel.org`
- 编译内核版本，替换现有内核版本
- 自己学习写一些模块
- `wwww.kernel.org` 中查看bugzilla



##### 为什么采用中断，而不是函数调用？

用户空间与内核空间，完全隔离，通过中断方式隔开；所有的进程共用内核代码；



##### 0x80中断被多次调用时如何处理？

保护好现场，执行下一个，再返回继续；



linux包含以下几个重要部分

1. 进程管理
2. 网络（从网络开始看）`tcp_input.c tcp_output.c tcp_ipv4.c sock_init接口net_syssctl_init tcp_init tcp_init_sock` `tcp.c`



dmsg查看linux打印信息  printk

##### fort函数创建进程如何实现？--  总结

copy on write (COW)



---



#### Linux内核的整体架构

<img src="..\pic\linux_total_sys.png" alt="linux_total_sys" style="zoom:75%;" />

- 进程管理（Process Scheduler）：负责管理CPU资源，以便让各个进程可以尽量公平的访问CPU
- 内存管理（Memory Manager）：负责管理内存资源，以便让各个进程可以安全地共享机器的内存资源；且提供了虚拟内存的机制，该机制可让进程使用多于系统可用的内存，不用的内存通过文件系统保存在外部非易失存储器中，需要时候，再换回内存
- 虚拟文件系统（Virtual File System）：Linux内核将不同功能的外部设备抽象为可以通过统一的文件操作接口（open，close等）来访问。
- 设备驱动：负责管理第三方设备接入/终端
- 网络（Network），网络子系统，负责管理系统的网络设备。

#### 进程调度（Process Scheduler）

提供对CPU的访问控制。

<img src="..\pic\linux_process.png" alt="linux_process" style="zoom:75%;" />

- scheduling policy：实现进程调度的策略，决定哪个进程将拥有CPU
- Architecture-specific Schdulers，体系结构相关部分，用于对不同CPU的控制，抽象为统一的接口。
- System Call Interface，系统调用接口，

#### 内存管理（Memory Manager，MM）

对内存资源访问控制

#### 网络子系统

<img src="..\pic\linux_networkd_sub.png" alt="linux_networkd_sub" style="zoom:75%;" />

Network protocols：实现各种网络传输协议



##### CFS完全公平调度器

基于时间最短的最公平调度；

主要思想：维护为任务提供处理器时间方面的平衡（公平性）；

CFS通过虚拟运行时间（vruntime）来实现平衡，维护提供给某个任务的时间量；CFS调度器中有一个计算虚拟时间的核心函数`calc_delta_fair()`

`fair.c/update_curr sched.c`

```c
struct task_struct {
  	struct sche_entity se;  // 用来跟踪调度信息  
};
struct sched_entity {
  	struct rb_node run_node;  // 红黑树节点
};

struct rb_node {
  	unsigned long __rb_parent_color;
    struct rb_node * rb_right;
    struct rb_node * rb_left;
};

struct rb_root_cached {
  	struct rb_root rb_root;
    struct rb_node * rb_leftmost;  // 最左节点指针
};

// CFS调度器的核心函数  kernel/sched/core.c/__schedule()  
// pick_next_task()让进程调度器从就绪队列中选择一个最合适的进程next，即红黑树最左边的节点
// context_switch()切换到新的地址空间，从而保证next进程运行
// 时钟周期结束时，调度器调用entity_tick()函数来更新进程负载，进程状态以及vruntime>
// 最后将进程的虚拟时间与就绪队列红黑树中最左边的调度实体的虚拟时间比较，如果小于，则不用触发调度，继续当前实体
```

#### 虚拟内存管理

内核管理某个进程的内存时使用了红黑树，每个进程都有一个`active_mm`的成员来管理该进程的虚拟内存空间。`struct mm_struct`中的成员`mm_rb`是红黑树的根，该进程的所有虚拟空间块都以起始虚拟地址为key值挂在该红黑树上。该进程新申请的虚拟内存区间会插入到这棵树上，当然插入过程中可能会合并相邻的虚拟区域。删除时会从树上摘除相应的node

![linux_rb_mm](..\\pic\linux_rb_mm.png)

虚拟内存区域结构`struct vm_area_struct`描述。从进程角度来讲，VMA其实是虚拟空间的内存块，一个进程的所有资源由多个内存块组成，所以一个进程的描述结构包含linux的内存描述`mm_struct`

```c
struct task_struct {
    ...
    struct mm_struct *m;
    ...
}

struct mm_struct {
    struct vm_area_struct *mmap;
    struct rb_root mm_rb;
    struct vm_area_struct * mmap_cahce;
}
```







---

#### linux 0.0.1版本分析

##### 1. 目录结构：

##### boot/

`boot.s`：实现计算机加电自检引导扇区，第一次加载扇区和第二次加载操作系统的功能；由BIOS加载执行；

`head.s`：主要包括初始设置的代码、时钟中断`int 0x08`的过程代码、系统调用中断`int 0x80`的过程代码以及任务A 和任务B 等的代码和数据；是32位引导代码，最后会调用main函数，完成系统的引导；`head.s`由`boot.s`调用执行；

##### fs/

文件系统file system，里面的源程序关于打开文件，关闭文件，管道，读写，获取文件信息，释放块，串行io 等等

##### include/

这个里面的东西很复杂，各大部件（内存、io 、文件系统、进程）的头文件都聚集在这里

##### init/

里面只有一个main函数，该函数大致作用如下：

- 如何分配使用系统物理内存
- 调用内核各部分的初始化函数
- 手动把自己移动到任务0中去运行（此时cpu运行模式进入ring3）
- 任务0创建一个进程（进程1），来运行init函数，任务0从此就是空转状态
- 进程1运行init函数，init作用就是创建一个新的进程，运行shell程序，shell退出后再重新创建

main函数执行后就循环等待，操作全部交给用户

 main.c 根据机器 CMOS 的值来初始化系统时钟，然后启动 tty 设备，启动系统陷阱，启动进程调度器，启动文件系统，启动硬盘中断处理程序；

下来，内核开启中断，切换到用户模式下执行，最后，调用 init() 函数，进入进程调度循环。init() 函数会调用 setuup() 函数，setup() 函数会检查硬盘分区表，并装载磁盘分区。接下来，init() 函数 fork() 另外一个进程建立一个会话，然后使用 execve() 建立一个登录 shell 进程，这个 shell 进程的 HOME 被设置为 /usr/root 。在 main 不能执行函数调用，直到 调用 fork() 为止。



##### kernel/

包含内核代码，比如进程创建`fork.c`，内存分配`malloc.c`等，还包含很多系统调用功能的实现函数`sys.c`

##### lib/

库文件

##### mm/

memory manage内存管理，就一个memory.c文件

##### 2. 启动过程分析

- **加载BIOS**

  打开计算机电源首先加载BIOS信息，BIOS中包含CPU的相关信息，设备启动顺序信息，硬盘信息，内存信息，时钟信息等。

  它是一组固化到计算机内主板上一个ROM芯片上的程序，它保存着计算机最重要的基本输入输出的程序、系统设置信息、开机后自检程序和系统自启动程序

- **读取MBR**

  磁盘上第0磁道第一个扇区被称为MRB（Master Boot Record），即主引导记录，大小为512字节，存有与启动信息，分区表信息等；

  系统找到BIOS所指定的硬盘的MBR后，就会将其复制到0x7c00地址所在物理内存中，其实被复制到物理内存的内容就是Boot Loader，具体到你的电脑就是grub。

  ```assembly
  BOOTSEG = 0x07c0
  INITSEG = 0x9000
  SYSSEG  = 0x1000
  ENDSEG  = SYSSEG + SYSSIZE
  
  entry start
  start:
  	mov ax, # BOOTSET
  ```

- **Boot Loader**

  Boot Loader为操作系统内核运行之前运行的一段小程序；通过这段小程序，可以初始化硬件设备、建立内存空间的映射图，从而将系统的软硬件环境带到一个合适的状态，以便为最终调用操作系统内核做好一切准备；

  Boot Loader有若干种，其中Grub，Lilo等为常见Loader，系统读取内存中的grub配置信息

- **加载内核**

  根据grub设定的内核映像所在路径，系统读取内存映像，并进行解压缩等操作，屏幕会输出`Uncompressing Linux`的提示，当解压缩内核完成后，屏幕输出`OK, booting the kernel`。

  系统将解压后的内核放置到内存中，并调用`start_kernel`函数来启动一系列的初始化函数并初始化各种设备，完成Linux核心环境的建立。

  至此，Linux内核已经建立起来，基于Linux的程序应该可以正常运行。

- **用户层init依据inittab文件来设定运行等级**

  内核加载后，第一个运行的程序便是`/sbin/init`，该文件会读取`/etc/inittab`文件，并依据此文件来进行初始化工作；

  `/etc/inittab`文件最主要的作用就是设定linux的运行等级，其设定形式：`id:5:initdefault`，表明linux运行在等级5上

- **init进程执行rc.sysinit**

  在设定运行等级后，Linux系统执行的第一个用户层文件就是`/etc/rc.d/sysinit`脚本，所作工作非常多，包括PATH，设定网络配置(`/etc/sysconfig/network`)，启动swap分区，设定`/proc`等。

- **启动内核模块**

  具体依据`/etc/modules.conf`文件或`/etc/modules.d`目录下的文件来装载内核模块

- **执行不同运行级别的脚本程序**

  根据运行级别的不同，系统会运行rc0.d到rc6.d中的相应的脚本程序，来完成相应的初始化工作和启动相应的服务

- **执行/etc/rc.d/rc.local**

  rc.local就是在一切初始化工作后，Linux留给用户进行个性化的地方。你可以把你想设置和启动的东西放到这里。

- **执行/bin/login程序，进入登录状态**

  系统已经进入到了等待用户输入username和password的时候了，你已经可以用自己的帐号登入系统了



---



> 基于0.01版本 以open为例 说明linux系统调用过程

#### 1. 0.01版本源码目录结构

##### boot/

`boot.s`：自检引导扇区，加载操作系统，由BIOS加载执行

`head.s`：由boot.s调用，进行初始化设置（比如），最后调用main函数完成系统引导

```assembly
# head.s
after_page_tables:
	pushl $0
	pushl $0
	pushl $0
	pushl $L6
	pushl $_main  # main.c地址入栈，设置分页(setup_paging)结束后执行ret返回指令时将main.c的地址pop并执行
	jmp setup_paging
L6:
	jmp L6
```

##### fs/

文件系统，对文件的各种操作比如open，stat等

##### include/

一些组件头文件，比如内存，进程，io等

##### init/

main函数进行一系列初始化：时钟初始化，tty初始化，进程调度初始化等，并fork一个进程来运行init函数进入进程调度循环，init会fork另外一个进程建立一个shell。main函数进入循环等待，操作全部交给用户。

##### lib/

库文件

##### mm/

内存管理

##### kernel/

内核代码，比如进程创建fork.c，内存分配malloc.c，进程调度sched.c等



具体代码不细展开，可自行[前往](https://github.com/zavg/linux-0.01)阅读



#### 2. 系统调用过程（以open为例）

系统调用接口对应的ID申明在`include/unistd.h`中

```c
// /include/unistd.h
#define __NR_fork 2  // fork系统调用
#define __NR_read 3  // read系统调用
#define __NR_open 5  // open系统调用
```



##### open接口调用入口

从以下代码可看出，open会触发0x80软中断，并将其flag `__NR_open`作为参数传入

```c
// lib/open.c
int open(const char* filename, int flag, ...)
{
    register int res;
    va_list arg;
    va_start(arg, flag);
    __asm__("int $0x80"  // 触发0x80软中断，从用户空间进入内核空间
           :"=a" (res)
           :"0" (__NR_open),"b"(filename),"c"(flag),  // 将__NR_open作为参数传入
           "d" (var_arg(arg,int)));
    if (res >= 0)
        return res;
    errno = -res;
    return -1;
}


```



##### 那么，触发0x80软中断会调到哪个接口？

0x80软中断与系统调用`system_call`绑定

```c
// init/main.c (从head.s中调过来)
// main接口中会启动进程调度器
void main(void) {
    time_init();  // 时钟初始化
    tty_init();	  // tty设备初始化
    trap_init();  // 系统陷阱初始化
    sched_init();  // 进程调度器初始化
    ...
}

// 进程调度代码在kernel/sched.c中
void sched_init(void) {
    ...
    set_system_gate(0x80, &system_call);  // 绑定系统调用软中断
}

// system_call接口通过汇编实现，具体代码在kernel/system_call.s中
_system_call:
	...
     call _sys_call_table(,%eax,4)  // 根据寄存器%eax中的参数找到对应的接口并执行
        
// sys_call_table实现在include/linux/sys.h
fn_ptr = sys_call_table[] = {sys_fork, sys_read, sys_write, sys_open...};
```

因此，open调用触发0x80软中断后会走到system_call中，system_call根据open传入的参数`__NR_open`偏移后在`_sys_call_table`中找到对应的系统调用接口`sys_open`

```c
// sys_open实现在fs/open.c中，返回filename对应文件的文件描述符fd
int sys_open(const char * filename, int flag, int mode)
{
    struct m_inode *inode;
    struct file *f;
    int i, fd;
    mode &= 0777 & ~current->umask; // 打开模式
    ...
    return (fd);
}
```



以上为linux open系统调用的整个过程




















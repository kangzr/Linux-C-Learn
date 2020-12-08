#### Linux内核如何学习

- 虚拟机安装linux，`kernel.org`
- 编译内核版本，替换现有内核版本
- 自己学习写一些模块
- `wwww.kernel.org` 中查看bugzilla



##### 系统调用的过程？

linux  0.11版本看的比较清楚；（linux 0.01版本实现了系统调用，移植了gcc，编译可执行程序）

80中断的入口函数在哪？（sys_call_table系统用调用表，每个元素对应一个系统调用，所有系统调用的中断）

80软中断 跟`system_call`接口，`system_call`用汇编实现；

`Unistd.h`中申明了所有系统调用的id，比如open对应`_NR_open`



##### 详细说明open系统过程的具体过程

触发80软中断(int 0x80) -->  system_call (Sched.c中init时绑定`set_system_call`) --> call sys_call_table(, %eax, 4)（%eax系统调用号）--> `sys_open` 



##### 为什么采用中断，而不是函数调用？

用户空间与内核空间，完全隔离，通过中断方式隔开；所有的进程共用内核代码；



##### 0x80中断被多次调用时如何处理？

保护好现场，执行下一个，再返回继续；



linux包含以下几个重要部分

1. 进程管理
2. 网络（从网络开始看）`tcp_input.c tcp_output.c tcp_ipv4.c sock_init接口net_syssctl_init tcp_init tcp_init_sock`





dmsg查看linux打印信息



















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
















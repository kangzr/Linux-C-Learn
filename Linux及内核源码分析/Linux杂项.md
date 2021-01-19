##### 孤儿进程

父进程退出，其子进程还在运行，那些子进程便成为孤儿进程，孤儿进程将被init进程（进程号为1）所收养，并由init进程对它们完成状态收集工作

##### 僵尸进程

一个进程使用fork创建子进程，如果子进程退出，而父进程并没有调用wait或waitpid获取子进程的状态信息，那么子进程的进程描述符仍然保存在系统中，则子进程为僵尸进程。

##### 僵尸进程危害：

如果进程不调用wait / waitpid的话， 那么保留的那段信息就不会释放，其进程号就会一直被占用，但是系统所能使用的进程号是有限的，如果大量的产生僵死进程，将因为没有可用的进程号而导致系统不能产生新的进程. 此即为僵尸进程的危害，应当避免

孤儿进程由init进程抚养，因此不会有什么危害。

##### 守护进程

Linux Daemon（守护进程）是运行在后台的一种特殊进程。它独立于控制终端并且周期性地执行某种任务或等待处理某些发生的事件。它不需要用户输入就能运行而且提供某种服务，不是对整个系统就是对某个用户程序提供服务。Linux系统的大多数服务器就是通过守护进程实现的。常见的守护进程包括系统日志进程syslogd、 web服务器httpd、邮件服务器sendmail和数据库服务器mysqld等



##### 查看进程所占内存

```shell
cat /proc/[pid]/statm
Provides information about memory usage, measured in pages.
The columns are:
size       (1) total program size 任务虚拟内存大小 
			(same as VmSize in /proc/[pid]/status)
resident   (2) resident set size 应用程序正在使用的物理内存的大小
			(same as VmRSS in /proc/[pid]/status)
share      (3) shared pages (i.e., backed by a file) 共享页
text       (4) text (code) 程序所拥有的可执行虚拟内存的大小
lib        (5) library (unused in Linux 2.6) 被映射到任务的虚拟内存空间的库的大小
data       (6) data + stack 程序数据段和用户态的栈的大小
```

##### 系统进程整体信息

```shell
cat /proc/stat

cat /proc/uptime
- 从开机到现在的累积时间(单位为秒)
- 从开机到现在的CPU空闲时间

# 用于获取某一个进程的统计信息
proc/[pid]/stat

utime: 该进程处于用户态的时间，单位jiffies，此处等于166114
stime: 该进程处于内核态的时间，单位jiffies，此处等于129684
```



从我个人角度来说，对这个项目以及相关的技术都是很有意向的，但是这个薪资水平对我真的一点吸引力都没有，我现在如果月奖金高的话都能拿到这个数了。所以我希望你们能够再商量一下。
















##### Ubuntu安装Boost库与使用

```shell
# 安装
sudo apt-get install libboost-all-dev
# 使用-lboost_system
g++ main.cpp -lboost_system
```



#### Boost ASIO

[参考](https://theboostcpplibraries.com/boost.asio)

#####  ASIO概念

Asio stands for asynchronous input/output. This library makes it possible to process data asynchronously.

Boost.Thread: access resources inside of a program

Boost.Asio: access resources outside of a program  (Network connections are an example of external resources)



##### 理解I/O Services 和 I/O Objects

io_service实现了一个任务队列，任务就是void(void)函数，常用的两个接口：post和run，post向任务队列中投递任务，run是执行任务队列中的任务，直到全部执行完毕，并且run可被多个线程调用。io_service是完全线程安全的队列。

io_service接口：run、run_one、poll、poll_one、stop、reset、dispatch、post，最常用的是run、post、stop

`io_service::post`：向队列投递任务，然后激活空闲线程执行任务，wake_one_thread_and_unlock尝试唤醒当前空闲的线程，其实现中特别之处在于，若没有空闲线程，但是有线程在执行task->run，即阻塞在epoll_wait上，那么先中断epoll_wait执行任务队列完成后再执行epoll_wait。

`io_service::run`：执行队列中的所有任务，直到任务执行完毕；run方法实际执行的是epoll_wait，run阻塞在epoll_wait上等待事件到来，并且处理完事件后将需要回调的函数push到io_servie的任务队列中，虽然epoll_wait是阻塞的，但是它提供了interrupt函数，

它向epoll_wait添加一个文件描述符，该文件描述符中有8个字节可读，这个文件描述符是专用于中断epoll_wait的，他被封装到select_interrupter中，select_interrupter实际上实现是eventfd_select_interrupter，在构造的时候通过pipe系统调用创建两个文件描述符，然后预先通过write_fd写8个字节，这8个字节一直保留。在添加到epoll_wait中采用EPOLLET水平触发，这样，只要select_interrupter的读文件描述符添加到epoll_wait中，立即中断epoll_wait。很是巧妙。！！！实际上就是因为有了这个reactor，它才叫io_servie，否则就是一个纯的任务队列了。



I/O Service是操作系统异步处理数据接口的一种抽象，它与操作系统API交互；I/O object发起一个异步操作，面向开发人员需要处理的任务。

后续版本中，I/O Service 被 I/O context概念取代，用法基本不变



run方法原则：

- 有任务立即执行任务，尽量使所有的线程一起执行任务
- 若没有任务，阻塞在epoll_wait上等待io事件
- 若有新任务到来，并且没有空闲线程，那么先中断epoll_wait,先执行任务
- 若队列中有任务，并且也需要epoll_wait监听事件，那么非阻塞调用epoll_wait（timeout字段设置为0），待任务执行完毕在阻塞在epoll_wait上。
- 几乎对线程的使用上达到了极致。
-  从这个函数中可以知道，在使用ASIO时，io_servie应该尽量多，这样可以使其epoll_wait占用的时间片最多，这样可以最大限度的响应IO事件，降低响应时延。但是每个io_servie::run占用一个线程，所以io_servie最佳应该和CPU的核数相同。

```cpp
void epoll_reactor::run(bool block, op_queue<operation>& ops){
    epoll_event events[128];
    int num_events = epoll_wait(epoll_fd_, events, 128, timeout);
    for (int i = 0; i < num_events; i++)
    {
        void * ptr = events[i].data.ptr;
        if (ptr == &interrupter_){
            check_timers = true;
        }
    }
}
```





[参考](https://theboostcpplibraries.com/boost.asio-io-services-and-io-objects)

```c++
// 使用Boost ASIO，启动一个异步定时器
// boost::asio::io_service, a single class for an I/O service object
// Every program based on boost.asio uses an object of type boost::asio::io_service.
#include <boost/asio/io_service.hpp>
// if data has to be sent or received over a TCP/IP connection, an I/O object of type boost::asio::ip::tcp::socket can be used

// wait for a time period to expire, you can use the I/O object boost::asio::steady_timer.
// is like an alarm clock
#include <boost/asio/steady_timer.hpp>
#include <chrono>
#incude <iostream>

using namespace boost::asio;
int main()
{
    // creates an I/O service object, ioservice, uses it to initialize the I/O object timer
    io_service ioservice;
    // Like boost::asio::steady_timer, all I/O objects expect an I/O service object as a first parameter in their constructor
    steady_timer timer{ioservice, std::chrono::seconds{3}};
   
    // 3s后调用lambda回调函数
    // Instead of calling a blocking function that will return when the alarm clock rings, 
    // Boost.Asio lets you start an asynchronous operation. 
    // To do this, call the member function async_wait(), which expects a handler as the sole parameter.
    // A handler is a function or function object that is called when the asynchronous operation ends. 
    
    // aync_wait是一个非阻塞的，其初始化一个异步操作然后立即返回，io_service.run()阻塞
    timer.async_wait([](const boost::system::error_code &ec){std::cout << "3 sec\n";});
    
    // This call passes control to the operating system functions that execute asynchronous operations.
    ioservice.run();  // 启动 阻塞
    return 0;
}
```







##### Scalability and Multithreading 多线程

如果采用多线程方式，应该采用一个IO service对应一个线程的模式；



I/O service代表一个操作系统接口，在Windows中，boost::asio::io_service基于IOCP，在Linux中，其基于epoll；

有多少个I/O service objects就有多少个epoll；



##### Network programming

Boost.Asio主要用于网络编程。其为网络编程提供了很多I/O objects，比如boost::asio::ip::tcp::socket



#### Coroutines







ASIO异步的核心就是一个`boost::asio::io_service`类，可以看出一个发动机







#### Boost.Bind

```cpp
#include <boost/bind.hpp>
#include <vector>
#include <algorithm>
#include <iostream>

bool compare(int i , int j)
{
    return i > j;
}

int main() {
    std::vector<int> v {1, 3, 2};
    std::sort(v.begin(), v.end(), boost::bind(compare, _1, _2));
    for (int i : v)
        std::cout << i << "\n";
}
```






























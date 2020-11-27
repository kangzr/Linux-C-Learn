#### [参考](http://senlinzhan.github.io/2017/09/17/boost-asio/)

#### 两种多线程模型：

- 每个线程持有一个`io_service`，并且每个线程都调用各自的`io_service`的`run`方法
- 全局只分配一个`io_service`，多线程共享，每个线程都调用全局的`io_service`的`run`方法



#### 每个线程一个I/O Service

- 多核机器上，充分利用多核CPU核心
- 某个socket不会再多个线程间共享，不需要引入同步机制
- 在`event handler`中不能执行阻塞的操作，否则将会阻塞`io_service`所在的线程


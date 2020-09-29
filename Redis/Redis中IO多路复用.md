Redis提供evport epoll kqueue select四种方式实现多路复用，默认epoll

单线程epoll，

优点：不存在锁的问题；避免线程间CPU切换

缺点：单线程无法利用多CPU；串行操作，某个操作出问题，会阻塞后续操作


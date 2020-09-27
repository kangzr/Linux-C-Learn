#### Epoll的LT模式和ET模式

**水平触发LT(Level Trigger)：**

一直触发

**边缘触发ET(Edge Trigger)：**

每次有新的数据包到达时`epoll wait`才会唤醒，因此没有处理完的数据会残留在缓冲区；

直到下一次客户端有新的数据包到达时，`epoll_wait`才会再次被唤醒。



Example：

服务端每次最多读取两个字节，客户端最多写入八个字节（`epoll_wait`被唤醒时）

Read Test：

客户端以LT模式启动，并输入aabbcc数据

服务端为LT模式：`epoll_wait`会一直触发直到数据全部读完, 分别读取aa bb cc（`epoll_wait`三次）

服务端为ET模式：只会读取两个字节，其它缓存，直到新数据包到达，`epoll_wait`再次唤醒

Write Test:

服务端为LT模式启动，客户端发送aabbccddeeffgghh

客户端为LT模式：一次写入八字节，分两次写完。(`epoll_wait`被唤醒两次)，读完数据后将EPOLLOUT从该fd移除。

客户端为ET模式：只写入八字节，`epoll_wait`只被唤醒一次。可能导致输入数据丢失。
### OpenSSL开源加密库

作业：堆排序，实现时间轮。

堆排序：存绝对时间(固定时间)

红黑树线程安全问题：

1. insert与delete加锁（大粒度加锁）,比较常用
2. 锁子树，细粒度锁；（可以用无锁cas）

libevent如何实现定时器？ epoll

nginx如何实现定时器？rbtree



openssl到底是做什么的？加密

1. hash

   判断10亿条数据是否存在 ---> 使用bitmap

   解决10亿条数据如何存储？分库分表（hash + 布隆过滤器）

   mongodb --> 插入/最近数据

   ```c
   #define NAME_LENGTH 32
   
   struct student {
   	
       char name[NAME_LENGTH];
       int high;
       char otherInfo[NAME_LENGTH];
       
   };
   
   int main() {
       _LHASH
   }
   ```

   使用。

   

2. BIO

   抽象接口， 研究BIO的封装和实现

3. base64

4. md4，md5，SHA1

5. RSA 非对称加密






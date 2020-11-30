### OpenSSL开源加密库

作业：堆排序，实现时间轮。

堆排序：存绝对时间(固定时间)

红黑树线程安全问题：

1. insert与delete加锁（大粒度加锁）,比较常用
2. 锁子树，细粒度锁；（可以用无锁cas）

libevent如何实现定时器？ epoll

nginx如何实现定时器？rbtree



各个模块阅读源码，并编写测试用例



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

   理解原理

1. md4，md5，SHA1， SHA256, SHA512

   

2. RSA 非对称加密（不需要了解数学原理，非对称加密）

23458章











---

#### 算法分类

- 对称算法：

  原理：加密解密使用同一个密钥；

  例如：电子密码本模式(ECB), 加密块链模式(CBC), 加密反馈模式(CFB), 输出反馈模式(OFB)

- **摘要算法**：

  原理：产生特殊格式算法，无论明文长度多少，采用一定规则对明文进行提取(摘要)，计算后密文长度固定，**不可逆**，

  例如：MD2，MD4，MD5，SHA，SHA-1/256/512等。常用摘要算法**MD5, SHA1**

- **公钥算法**：

  原理：公钥私钥使用不同的密钥，但是两者之间存在相互依存关系，私钥加密必须用相应的公钥才能解密。

  例如：**RSA**，DSA等

#### OpenSSL间接






























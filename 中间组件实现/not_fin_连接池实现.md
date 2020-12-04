#### 一. 什么是数据库连接池(Connection Pooling)？

是程序启动时建立足够的数据库连接，并将这些连接组成一个连接池，由程序动态对池中的连接进行申请、使用和释放。

创建数据库是一个很耗时的操作，也容易对数据库造成安全隐患。所以，在程序初始化时，集中创建多个数据库连接，并把他们集中管理，供程序使用，也可保证较快的数据库读写速度，更加安全可靠。



#### 二. 数据库连接池运行机制

- 从连接池获取/创建可用连接
- 使用完后，把连接归还给连接池
- 在系统关闭前，断开所有连接并释放占用的系统资源

![db_connection_poll](..\pic\db_connection_poll.png)



#### 三. 为什么使用数据库连接池，有什么好处？

如果不使用数据库连接池，则流程如下：

- TCP三次握手
- MySQL认证的三次握手
- SQL执行
- MySQL关闭
- TCP关闭

可见，为执行一条SQL语句，多了很多我们并不关心的网络交互；

![db_connection_pool_1](..\pic\db_connection_pool_1.png)

使用数据库连接池后，流程如下：

只第一次访问需要建立连接，之后的访问，复用之前创建的连接，直接执行SQL语句。

![db_connection_pool_2](..\pic\db_connection_pool_2.png)

**使用数据库连接池好处**：

- **资源重用**：避免了频繁的创建、释放连接引起的性能开销，在减少系统消耗的基础上，也增进了系统运行环境的平稳性（减少内存碎片以及数据库临时进程/线程的数量）
- **更快的系统响应速度**：数据库连接池在初始化过程中，往往已创建了若干数据库连接于池中备用，此时连接的初始化工作均已完成，对于业务请求处理而言，直接利用现有可用连接，避免了从数据库连接初始化和释放过程的开销，从而缩减了系统整体响应时间
- **统一的连接管理**，避免连接数据库连接泄漏：在较为完备的数据库连接池实现中，可根据预先的连接占用超时设定，强制收回被占用连接。从而避免了常规数据库连接操作中可能出现的资源泄漏。



#### 四. 连接池和线程池有什么区别？

**连接池**：被动分配，用完放回

**线程池**：主动干活，有任务到来，线程不断取任务执行

连接池一般配合着线程池使用。



CDBConn 连接

CDBPool 连接池，管理连接



如何使用连接池

连接池重连机制如何做？

线程数量和连接池数量如何选择？

线程数量==连接池数量==CPU核数

如果使用协程的话，连接数 = ((CPU核数 * 2) + 有效磁盘数)



如何更好的设计一个连接池？

- 重连次数统计
- 总的连接次数统计
- 峰值连接次数，连接峰值数量供后续的性能评估（1，5，15s统计一次）
- 超时机制，阻塞，非阻塞



#### MySQL - Pool

```cpp
// Thread.h
#include <stdint.h>
#include <pthread.h>
class CThreadNotify
{
public:
    CThreadNotify()
    {
        pthread_mutexattr_init(&m_mutexattr);
        pthread_mutexattr_settype(&m_mutexattr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&m_mutex, &m_mutexattr);
        pthread_cond_init(&m_cond, NULL);
    }
    ~CThreadNotify()
    {
        pthread_mutexattr_destroy(&m_mutexattr);
        pthread_mutex_destroy(&m_mutex);
        pthread_cond_destroy(&m_cond);
    }
    void Lock()
    {
        pthread_mutex_lock(&m_mutex);
    }
    void Unlock()
    {
        pthread_mutex_unlock(&mutex);
    }
    void Wait()
    {
        pthread_cond_wait(&m_cond, &m_mutex);
    }
    int WaitTime(int ms){
        ...
        return pthread_cond_timedwait(&m_cond, &m_mutex, &outtime);
    }
  	void Signal()
    {
        pthread_cond_signal(&m_cond);
    }
private:
    pthread_mutex_t m_mutex;
    pthread_mutexattr_t m_mutexattr;
    pthread_cond_t m_cond;
};



// DBpool.h
class CDBPool {
public:
    CDBPool() {}
    CDBPool(const char * pool_name, const char * db_server_ip, uint16_t db_server_port,
           const char * username, const char * password, const char* db_name, int max_conn_cnt);
    virtual ~CDBPool();
    int Init();  // 初始化，连接数据库，建立连接
    CDBConn* GetDBConn;  // 获取连接资源，没有空闲则创建
    void RelDBConn(CDBConn* pConn);  // 归还资源
private:
    list<CDBConn*> m_free_list;  // 空闲连接
    CThreadNotify m_free_notify;  // 同步机制
    int m_db_cur_conn_cnt;  // 当前启用的连接数量
    int m_db_max_conn_cnt;  // 最大连接数量
}

// DBpool.c
int CDBPool::Init()
{
    for (int i = 0; i < m_db_cur_conn_cnt; i++) {
        CDBConn * pDBConn = new CDBConn(this);
        int ret = pDBConn->Init(); // mysql_real_connect连接数据库
        m_free_list.push_back(pDBConn);
    }
}

// 保护机制：可分配连接放入一个队列，如果没有空闲连接，检查已分配的连接多久没有返回，如果超过一定时间，则自动收回，放在用户忘了调用释放连接的接口
CDBConn *CDBPool::GetDBConn()
{
    m_free_notify.Lock();
    while (m_free_notify.Lock()) {
        if (m_db_cur_conn_cnt >= m_db_max_conn_cnt)
            m_free_notify_wait();
        else{
            // 创建新连接，加入list
            CDBConn *pDBConn = new CDBConn(this);
            int ret = pDBConn->Init();
            m_free_list.push_back(pDBConn);
            m_db_cur_conn_cnt++;
        }
    }
    CDBConn *pConn = m_free_list.front();  // 获取连接
    m_free_list.pop_front();
    m_free_notify.Unlock();
    return pConn;
}

void CDBPool::RelDBConn(CDBConn *pConn) {
    m_free_notify.Lock();
    list<CDBConn *>::iterator it = m_free_list.begin();
    for (; it != m_free_list.end(); it++)
    {
        if (*it == pConn){
            break;
        }
    }
    if (it == m_free_list.end()){
        m_free_list.push_back(pConn);
    }
    m_free_notify.Signal();
    m_free_notify.Unlock();
}
```



#### MySQL  C  API说明

```cpp
// mysql C API:
// 1. 初始化mysql实例
MYSQL *ms_conn = mysql_init(NULL);
// 2. 连接mysql服务器
MYSQL *ms_res = mysql_real_connect(ms_conn, "localhost", "run", "123456");
// 3. 执行sql语句
// 函数原型
int mysql_real_query(MYSQL *mysql, const char *query, unsigned int length);
// 执行由query指向的SQL查询，其长度为length。对于包含二进制数据的查询，必须使用mysql_real_query而不是mysql_query.
// 返回值：Success (0)   Failed (ErrorCode !0)
// 4. 取出SQL语句执行的结果集
MYSQL_RES *ms_res = mysql_store_result(ms_conn);
// 5. 字段个数（显示列长度）
unsigned int field_num = mysql_num_fields(ms_res);
// 6. 每个字段的数据信息
MYSQL_FIELD* field_info = mysql_fetch_field(ms_res);
// 7. 取出下一行结果
MYSQL_ROW data = mysql_fecth_row(ms_res);
// 8. 释放结果集
mysql_free_result(ms_res);
// 9. 使用完释放资源
mysql_close(ms_conn);
```



#### Redis - Pool


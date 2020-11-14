

###  设计模式

Book《研磨设计模式》



观察者模式

发布订阅模式

工厂模式

单例模式



1) 推模型 把具体内容广播出去

2) 拉模型 粒度，把摘要推给大家，用户点击再去请求全部数据



---

### 单例模式

##### 注意要点

- 全局唯一特性，申明`static`，
- 线程安全
- 禁止构造，赋值和拷贝(全部定义为私有)
- 用户通过`get_instance`获取实例

**version1**

**缺点：**

- 线程不安全
- 存在内存泄漏（只有new，没有delete）

```c++
class Singleton {
  	private:
    	Singleton() {}
    	Singleton(Singleton&) = delete;
    	Singleton& operator=(const Singleton&) = delete;
    	static Singleton * m_instance_ptr;
   	public:
    	~Singleton() {}
    	static Singleton * get_instance() {
            if (m_instance_ptr == nullptr) {
                m_instance_ptr = new Singleton;
            }
            return m_instance_ptr;
        }
};

Singleton* Singleton::m_instance_ptr = nullptr;
```

**version2**

优点：

- lock+双检锁，线程安全；（C++创建对象，new不是原子操作，会执行：1. 分配内存；2. 调用构造；3.赋值；第2步可能晚于第3步）
- shared_ptr管理，不会有内存泄漏

缺点：

- 有些平台双检锁会失败
- 代码量增多

```c++
class Singleton {
 	public:
    	typedef std::shared_ptr<Singleton> Ptr;
    	~Singleton() {}
    	static Ptr get_instance() {
            // std::atomic_thread_fence(std::memory_order_acquire);  //获取内存fence
            if (m_Instance == nullptr) {
                std::lock_guard<std::mutex> lk(m_mutext);
                // tmp = m_instance.load(std::memory_order_relaxed);
                if (m_Instance == nullptr) {
                    m_Instance = std::shared_ptr<Singleton> (new Singleton);
                    // std::atomic_thread_fence(std::memory_order_release);  // 释放内存fence
                }
            }
            return m_Instance;
        }
    private:
    	Singleton() {}
    	Singleton(Singleton&) = delete;
    	Singleton& operator=(const Singleton&) = delete;
    	static Ptr m_Instance;
    	static std::mutex m_mutex;
};

Singleton::Ptr Singleton::m_Instance = nullptr;
std::mutex Singleton::m_mutex;


// c++后支持跨平台实现, 使用atomic_thread_fence;
```



`lock_guard`：构造时上锁，析构时解锁(离开函数作用域会导致局部变量析构函数被调用)

这个类是一个互斥量的包装类，用来提供自动为互斥量上锁和解锁的功能，简化了多线程编程

```c++
// lock_guard实现
template<typename T> class my_lock_guard {
public:
    my_lock_guard(T& mutex) : mutex_(mutex) {
        mutex_.lock();
    }
    ~my_lock_guard() {
        mutex_.unlock();
    }
private:
    my_lock_guard(my_lock_guard const7) = delete;
    my_lock_guard& operator=(my_lock_guard const&);
    T& mutex_;
};
```



**version3**

当变量初始化时，并发同时进入申明语句，并发线程将会阻塞等待初始化结束;

最佳方案

```c++
// 懒汉式
class Singleton {
  	public:
    	~Singleton() {}
    	static Singleton get_instance() {
            // 懒汉式 c++11线程安全
            static Singleton m_Instance;
            return m_Instance;
        }
    private:
    	Singleton() {}
    	Singleton(Singleton&) = delete;
    	Singleton& operator=(const Singleton&) = delete;
};

// 饿汉式，main函数运行前就初始化，绝对安全
class Singleton {
 	public:
    	~Singleton() {}
    	static Singleton get_instance(){
            return &m_Instance;
        }
    private:
    	Singleton() {}
    	Singleton(Singleton&) = delete;
    	Singleton& operator=(const Singleton&) = delete;
    	static Singleton m_Intance; 
};
```



**boost::noncopable实现**

`=default`: 用于显式要求编译器提供合成版本的四大函数（构造、拷贝、析构、赋值）

`=delete`：禁止生成

```c++
class noncopyable {
    protected:
    	// 默认构造函数和析构函数都是protected
		// 不允许创建noncopyable实例，但允许子类创建
		// (即允许派生类构造和析构)
    	noncopyable() = default;
    	~noncopyable() = default;
    private:
   		// 使用delete关键字禁止编译器自动产生copy构造函数和赋值操作符
    	noncopyable(noncopyable&) = delete;
    	noncopyable& operator=(const noncopyable&) = delete;
}
```




















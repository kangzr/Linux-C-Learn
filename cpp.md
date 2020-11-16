#### static

- 修饰普通变量：修改变量的存储区域和生命周期，使用变量存储在静态区，在main函数运行前就分配了空间，有初始化值就初始化，没有则系统默认值
- 修饰普通函数：表明函数的作用范围仅在定义该函数的文件内才能使用（防止重名）
- 修饰成员变量：使得所有的对象只保留一个变量，不需要生成对象就可以访问该成员
- 修饰成员函数：使得不需要生成该对象就可以访问该函数，但在static函数内不能访问非静态成员

static全局变量和普通全局变量的区别：两者存储在静态区，但是static修饰的全局变量仅申明源文件可用，而普通全局变量可以多个源文件共用

static局部变量和普通局部变量（存储方式无区别）

static函数与普通函数区别：普通函数每次调用都会在堆栈中有一份拷贝，而static函数在内存中只有一份



#### #pragma pack(n)

强制字节对齐方式



#### volatile

- volatile关键字声明的变量，编译器不会对其进行优化，**每次访问都必须从内存中取值**，没有被volatile修饰的变量，可能由于编译器的优化，从CPU寄存器。



#### extern  "C"

C++中调用C函数，被extern "C"修饰的变量和函数按照C方式编译和链接的。



#### C++中struct和class

- struct更适合看成一个数据结构的实现体，class更适合看出一个对象的实现体
- struct默认数据访问控制是public，class是private



#### union联合

一个节省空间的特殊类，一个union可以有多个数据成员，但是在任意时刻只有一个数据成员可以有值，**当某个成员被赋值后，其它成员变为未定义状态**，特点如下：

- 默认访问控制为public
- 可以含有构造函数，析构函数
- 不能含有引用类型的成员
- 不能继承自其它类，不能作为基类，不能包含虚函数



#### union来判断大小端

大端是高位值存放在内存的低位地址，小端则相反；比如0x12345678在大端上是12345678，在小端上是87654321；

union是一个联合体，所有变量公用一块内存，只是在不同的时候解释不同。其在内存中存储是按最长的那个变量所需要的位数来开辟内存的

如下，union的实际内存长度是int，即一个字，在32位机上是32位。而char是一个byte，只会取第一个低地址字节。所以他的值可以用来判断大小端;

```c
union {
    int number;
    char ch;
}test;

bool is_big_end() {
    test.number = 0x12345678;
    return (test.ch == 0x12);
}

int main(){
    if (is_big_end())
        cout << "is big end" << endl;
    else
        cout << "is small end" << endl;
    return 0;
}
```



#### C++多态实现原理

##### **C++多态分为静态和动态**

- 静态多态：重载函数(根据参数列表的不同)，在编译期就可以确定函数地址，在编译期就能决定
- 动态多态：通过继承重写基类的虚函数来实现，因为要在运行期才能决定，因此称为动态多态；运行时在虚函数表中寻找调用函数地址，多态表现为指向父类调父类，指向子类调子类

##### 动态多态实现原理（一个接口，多种实现）

- 用virtual关键字申明的函数叫做虚函数，虚函数肯定是类的成员函数
- 纯虚函数：*virtual void fun()=0*，抽象类是指包括至少一个纯虚函数的类，必须在子类实现这个函数(父类无实现，子类必实现)
- 每一个具有虚函数的类都有一个虚函数表(虚表vtable)，编译器会在类中生成一个虚表，该类的所有对象共用
- 虚函数表是一个存储类所有virtual成员函数指针的数据结构，由编译器自动生成和维护
- 每一个该类的对象有一个指向虚表的指针，虚表和类对应，虚表指针(vptr)和对象对应(在程序只读数据段.rodata section)
- vptr一般作为类对象的第一个成员

虚表指针vptr指向该类的虚表，如果该类继承至其它类，该虚表中会保存其父类的虚函数地址，如果对父类虚函数进行了重写，那么会保存自己的虚函数地址，派生类的虚表中虚地址的排列顺序和基类的虚表中虚函数地址排列顺序相同(内存布局依次按声明顺序排列)

![cpp_virtual](pic\cpp_virtual.png)

##### 为什么调用普通函数比调用虚函数高效？

- 普通函数在编译期就确定函数地址，这直接调用；
- 而虚函数的地址要在运行期才能确定；在调用前，解引用对象首地址得到虚表指针，根据虚表指针加上偏移量才能找到要调用的虚函数，因此更耗时

##### 为什么要用虚函数表（存函数指针的数组）？

- 实现多态；运行时根据对象的实际类型来调用相应的函数，如果对象类型是父类，则调用父类虚函数，是子类则调用子类虚函数
- 同一个类的所有对象共用一张虚表，节省空间，保存着自己的虚函数，继承来的虚函数，以及重写的虚函数

##### 为什么要把基类的析构函数定义为虚函数？

- 用基类对象操作派生类时，为防止执行基类的析构函数，不执行派生类的析构函数，造成内存泄漏
- 子类重写父类析构函数(尽管名字不一样，但编译器做过特殊处理，在内部析构函数名一样)，如果不定义成虚析构函数，就无法构成多态，父类析构函数隐藏了子类析构函数，只能调到父类析构函数；若定义成虚析构函数，那会先调用子类析构函数，再调用父类

##### 构造函数可以为虚函数吗？

- 不能，因为虚表指针的初始化发生在构造函数期间，必须要构造完成后，才形成虚表指针

##### 静态函数static/内联函数inline可以为虚函数吗？

- 都不能，static函数绑定发生在编译期间；inline函数不能表现多态性时的虚函数，

##### 虚函数与纯虚函数的区别

- 虚函数在子类中可以不重载，但纯虚函数必须在子类中实现
- 带纯虚函数的类叫抽象类，这种类不能直接生成对象，只有被继承，实现其虚函数后才能使用。



#### 面向对象

面向对象的三大特征：封装、继承、多态



#### new和delete

- new/new[]：先底层调用malloc分配内存，再调用构造函数创建对象，（非原子操作）
- delete/delete[]：先调用析构函数清理资源，再底层调用free释放空间
- 定位new(placement new)，允许我们向new传递额外参数



#### new和malloc

- new在申请内存时会自动计算所需字节数，而malloc则需要我们自己输入申请内存空间的字节数



#### delete this合法吗？

合法，但是

- 必须保证this对象是通过new(不是 new[], 不是placement new, 不是栈上，不是全局，不是其它对象成员)分配的
- 必须保证调用delete this的成员函数是最后一个调用this的成员函数；
- 必须保证delete this后没有人是用来



#### 如何定义一个只能在堆或者栈上生成对象的类？

##### C++中类的对象创建分两种：

- 静态创建类对象：编译器为对象在栈空间中分配内存，是通过直接移动栈顶指针，挪出适当的空间，然后在这片内存空间上调用构造函数形成一个栈对象。使用这种方法，直接调用类的构造函数
- 动态创建类对象：使用new运算符将对象建立在堆空间中，一，执行operator new()函数，在堆空间中搜索合适的内存并进行分配，二，调用构造函数构造对象，初始化这片内存空间；间接调用类的构造函数；

**只能在堆上创建：**

- 方法：将析构函数设置为私有
- 原因：C++是静态绑定语言，编译器管理栈上对象的生命周期，编译器在为类对象分配栈空间时，会先检查类的析构函数的访问性，若析构函数不可访问(编译器调用其来释放内存)，则不能在栈上创建对象
- 缺点：无法实现多态，可将其设置为protected，则类外成员无法访问，子类可以

```c++
class A {
    public:
    	A() {}
    	void destroy() {delete this};  // 必须提供destroy来释放内存
    private:
    	~A();
}
```

**只能在栈上创建：**

- 方法：将new和delete重载为私有
- 原因：在堆上生成对象，使用new关键字操作，分为两阶段，第一个阶段：使用new在堆上寻找可用内存，分配给对象；第二阶段，调用构造函数生成对象。如果将new重载为私有，那么第一阶段就无法完成，不能在堆上生成对象。



#### C++智能指针

C++标准库(STL)中 头文件: `#include <memory>`

**auto_ptr**

- 不能让多个auto_ptr指向同一个对象，否则会出现一个对象被删除多次的现象，为防止这种情况出现，auto_ptr有一个性质：当通过copy构造函数或copy     assignment操作符复制它们时，会让原本指针指向null，而复制所得的指针取得资源的唯一拥有权，因此弃用

**unique_ptr**

- 是一种在异常时可以帮助避免资源泄漏的智能指针；
- 采用独占式拥有，意味着确保一个对象和其相应的资源同一时间只被一个pointer拥有，所指的内存为自己独有，不支持拷贝和赋值

**shared_ptr**

强引用，允许多个该智能指针共享“拥有”同一个堆分配的对象内存，这通过引用计数（reference counting）实现，会记录有多少个shared_ptr共同指向一个对象，一旦最后一个这样的指针被销毁，引用计数变为0，这个对象会被自动销毁，类似垃圾回收gc机制，但是无法解决循环引用问题，于是就有了weak_ptr。（不同与auto_ptr，支持复制和赋值操作）

**weak_ptr**

- 弱引用，它可以指向shared_ptr指向的对象内存，却并不拥有该内存，而使用weak_ptr成员lock，则可返回其指向内存的一个share_ptr对象，且在所指对象内存已经无效时，返回指针空值nullptr 。weak_ptr并不拥有资源的所有权，所以不能直接使用资源；**weak_ptr解决shared_ptr循环引用的问题**
- 用 enable_shared_from_this从this转换到shared_ptr

>  shared_ptr和auto_ptr析构函数都是执行delete操作，而非delete []，因此不能将两者用于array（虽然能够编译通过）



#### 空类，编译器会给它生成哪些函数

```c++
class Empty() {};
```

1. 缺省构造函数: `Empty() {…}`
2. 缺省拷贝构造函数: `Empty(const Empty& rhs) {…}`
3. 缺省析构函数: `~Empty() {…}`
4. 缺省赋值运算符: `Empty& operator=(const     Empty& rhs) {…}`
5. 缺省取址运算符: `Empty* operator&() { ... }`
6. 缺省取址运算符 const: `const Empty\* operator&() const     { ... }`

 

- 这些函数只有在用到的时候，才会生成
- 这些函数全是public且inline的
- 如果你显式的声明了这些函数中的任何一个函数，那么编译器将不再生成默认的函数



#### 深copy和浅copy的区别

浅拷贝：只对指针进行copy，copy后两个指针指向同一内存（默认拷贝构造函数是浅拷贝）

深拷贝：不仅对指针进行拷贝，而且会对指针所指向的内存进行拷贝，拷贝后两个不同的指针指向两块不同的内存



#### C++ STL

**Traits特性**

特性萃取特性，简单说就是提取“被传进对象”对应的返回类型，让同一个接口实现对应的功能；

STL中拥有大量的容器，不能针对每个容器重写一份算法，这样算法设计会过多依赖容器；为了让STL的算法和容器分离的，两者通过迭代器链接，因此迭代器是算法和容器的桥梁。算法的实现并不知道自己被传进来什么。萃取器相当于在接口和实现之间加一层封装，来隐藏一些细节并协助调用合适的方法，这需要一些技巧（例如，偏特化）;

- 在面对不同的输入类时，能找到合适的返回型别
- 型别对应有不同的实现函数的时候，能起到一个提取型别然后分流的作用

Why？---参数推导机制

Template参数推导机制无法应对函数返回值，Traits技法采用内嵌型别来实现获取迭代器型别这一功能需求；

But？---内嵌型别

并非所有迭代器都是class type，比如原声指针，不是class type就不能声明内嵌型别；

So？---模板偏特化Partial Specialization

针对任何template参数更进一步的条件限制所设计出来的一个特化版本；



**STL中map四种插入**

```c++
map<int, string> m;
// 1. pair
m.insert(pair<int, string>(1, "aaaa"));

// 2. make_pair
m.insert(make_pair<int, string>(2, "bbbb"));

// 3. value_type
m.insert(map<int, string>::value_type(3, "cccc"));

// 4. [] 允许键重复，当重复时，会覆盖掉之间的键值对，否则插入
m[4] = "dddd";

// 前三种方法当出现重复键时，编译器会报错
```



#### vector和list

**vector底层存储机制实现/自动增长机制**

vector就是一个动态数组，里面有一个指针指向一片连续的内存空间，**当空间不够装下数据时，会自动申请另一片更大的空间**，一般是增加当前容量的50%或100%，（开辟新空间），然后把原来的数据拷贝过去（移动数据），接着释放原来的那片空间（销毁旧空间）（当释放或者删除里面的数据时，其存储空间不释放，仅仅是清空了里面的数据。）

支持随机存储(vector迭代器是普通指针 random access iterator, 如果定义vector<int>::itertator it, it的类型就是it*) 

数据结构: 线性连续空间;

**为什么vector的插入操作可能会导致与其相关的指针、引用以及迭代器失效**

当前容量不够时，插入会造成空间的申请和数据移动

**vector中begin和end函数返回的是什么？**

begin返回第一个元素的迭代器，end返回最后一个元素后面位置的迭代器。前闭后开[)

**list自带排序函数的排序原理**

因为list的迭代器是Bidirectional Iterator, 而STL算法中的sort要求是Random access itrator, 所以不能使用；自带排序原理类似merge

将前2个元素合并，再将后2个元素合并，然后合并这2个子序列成为4个元素序列的子序列，重复这一过程，得到8个，16个，，，，子序列，最后得到的就是排序后的序列。

**vector与list区别**

**数据结构的区别**

-  vector与数组类似，拥有一段连续的内存空间，并且起始地址不变。便于随机访问，时间复杂度为O(1)，但因为内存空间是连续的，所以插入和删除操作时间复杂度为O(n) (会造成内存块的拷贝)，此外，当数组内存空间不足，会采取扩容，通过重新申请一块更大的内存空间进行内存拷贝。
- list底层是由双向链表实现的，因此内存空间不连续的。根据链表的实现原理，list查询效率较低，不支持随机访问，时间复杂度为O（n），但插入和删除效率较高为O(1)。只需要在插入的地方更改指针的指向即可，不用移动数据。

**迭代器支持不同**

异：vector中，iterator支持 ”+“、”+=“，”<"等操作。而list中则不支持

同：vector<int>::iterator和list<int>::iterator都重载了 “++ ”操作

**什么情况下用vector，什么情况下用list**

vector可以随机存储元素（即可以通过公式直接计算出元素地址，而不需要挨个查找），但在非尾部插入删除数据时，效率很低，适合对象简单，对象数量变化不大，随机访问频繁。 

list不支持随机存储，适用于对象大，对象数量变化频繁，插入和删除频繁。



#### vector、list、map、deque用erase（it）后，迭代器的变化

- vector和deque是序列式容器，其内存分别是连续空间和分段连续空间，删除迭代器it后，其后面的迭代器都失效了，此时it及其后面的迭代器会自动加1，使it指向被删除元素的下一个元素。     
- list删除迭代器it时，其后面的迭代器都不会失效，将前面和后面连接起来即可。     
- map也是只能使当前删除的迭代器失效，其后面的迭代器依然有效。



#### deque & vector区别

deque动态地以分段连续空间组合而成，随时可以增加一段新的连续空间并链接起来。不提供空间保留功能。

- deque允许在常数时间对头部进行元素的插入或删除(vector是单开口的连续线性空间，deque是双开口的)
- deque没有容量capacity的概念，因为他是动态的以连续分段空间组合而成
- deque的迭代器并不是普通的指针，vector是；



#### 为什么要尽可能选择使用vector而非deque？

1. deque的迭代器比vector迭代器复杂很多。
2. 对deque排序，为了提高效率，可先将deque复制到一个vector上排序，然后再复制回deque。



#### map底层机制，查找复杂度，能不能边遍历边插入

- map以RB-TREE（红黑树）为底层机制。RB-TREE是一种平衡二叉搜索树，自动排序效果不错。 
- 通过map的迭代器不能修改其键值，只能修改其实值。所以map的迭代器既不是const也不是mutable。
- 查找复杂度，红黑树查找复杂度O(logN) 
- 不可以，map不像vector，**它在容器进行erase操作后不会返回后一个元素的迭代器**，不能遍历的往后插删。



#### hashtable如何避免地址冲突

- 线性探测linear probing：先用hash function计算某个元素的插入位置，如果该位置的空间已被占用，则继续往下寻找H+1,     H+2…，知道找到一个可用空间为止。 其删除采用惰性删除：只标记删除记号，实际删除操作等到表格重新整理时再进行。 

- 二次探测quadratic probing：如果计算出的位置为H且被占用，则依次尝试H+1^2，H+2^2等（解决线性探测中主集团问题）。     

- 开链separate chaining（拉链都行）：每一个表格元素中维护一个list，hash function为我们分配一个list，然后在那个list执行插入、删除等操作。（这个是现在STL解决冲突的方法和著名非关系型redis数据库解决hash冲突采用的一种方法）



#### hashtable与平衡二叉搜索树区别

- 平衡二叉搜索树的“对数平均时间”的表现是建立在“输入数据有足够随机性的基础性”上
- hashtable的插入删除搜寻等也具有“常数平均时间”的表现，这种表现是以统计为基础，不依赖输入元素的随机性



 

#### hashtable，hashmap,     hash_set，hash_map，set，map区别

hashmap hash_map hash_set都是以hashtable为底层结构；

map，set是以红黑树（rb tree）为底层结构

- hash_set以hashtable为底层，不具有排序功能，能快速查找。其键值就是实值。
- set以RB-TREE为底层，具有排序功能。

- hash_map（unordered_map）以hashtable为底层，没有自动排序功能，能快速查找（O（1）），每一个元素同时拥有一个实值和键值。
- map以RB-TREE为底层，具有排序功能，操作时间复杂度为O(logn)。



#### set

- set不允许存在重复元素
- set迭代器不能改变元素值，因为set的元素值就是其键值，关系到排序规则，因此set底层的RB-tree的为constant     iterator
- set插入或删除元素后，迭代器不会失效(erase的那个元素除外)



#### hash_map&map

构造函数：hash_map需要hash function和等于函数，而map需要比较函数（大于或小于）。 

存储结构：hash_map以hashtable为底层，而map以RB-TREE为底层。

总的说来，hash_map查找速度比map快，而且查找速度基本和数据量大小无关，属于常数级别。而map的查找速度是logn级别。但不一定常数就比log小，而且hash_map还有hash function耗时。 

如果考虑效率，特别当元素达到一定数量级时，用hash_map。 

考虑内存，或者元素数量较少时，用map。



#### map&set

**为何map和set的插入删除效率比其他序列容器高。** 

因为不需要内存拷贝和内存移动 

**为何map和set每次Insert之后，以前保存的iterator不会失效？** 

因为插入操作只是结点指针换来换去，结点内存没有改变。而iterator就像指向结点的指针，内存没变，指向内存的指针也不会变。 

**当数据元素增多时（从10000到20000），map的set的查找速度会怎样变化？** 

RB-TREE用二分查找法，时间复杂度为logn，所以从10000增到20000时，查找次数从log10000=14次到log20000=15次，多了1次而已。





#### unordered_map

- 不支持pair作为key，需要自己定义

- 查找某个元素是否存在方法：

- - m.find(p)      == m.end()；
  - m.count(p)      == 1；(count只有1 和0两种结果),因为map不允许key重复
  - 切记不能使用m[p] == 0，因为默认值是0



#### STL底层结构数据结构实现

1. vector: 数组，支持快速随机访问 
2. list: 双向链表，支持快速增删 
3. deque:     一个中央控制器和多个缓冲区，支持首尾（中间不能）快速增删，支持随机访问 
4. stack:     底层用deque或者list实现，不用vector的原因是扩容耗时 
5. queue:     底层用deque或者list实现，不用vector的原因是扩容耗时 
6. priority_queue:     一般以vector为底层容器，heap为处理规则来管理底层容器实现 
7. set: 红黑树 有序，不重复 
8. multiset: 红黑树     有序，可重复 
9. map: 红黑树 有序，不重复 
10. multimap: 红黑树     有序，可重复 
11. hash_set :     hashtable 无序，不重复 
12. hash_map: hashtable ，无序，不重复 
13. hashtable: 底层结构为vector



#### remove与erase区别

stl remove并不会删除元素，只会覆盖要删除的元素，返回一个逻辑新终点；（以实际值覆盖被移除元素）

但是，对于容器的成员函数remove，**效率是要高于算法的**，因此，**尽量使用容器成员函数**，而不是算法中实现，比如，list，应当使用list.remove；因为链表本身是具有O(1)时间进行元素的删除和插入操作的，而算法remove是需要O(n)时间的。

erase真正删除元素（关联容器和无序容器都适用这个来删除元素）

set.erase(value)

一行删除

`list.erase(remove(list.begin(), list.end(), value), list.end());`















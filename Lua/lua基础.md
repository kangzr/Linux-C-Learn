A language that doesn't affect the way you think about programming, is not worth knowing

-- Alan Perlis



底层特性很少，可复用度很高

#### 字符串

生成一个新字符串的步骤



全局哈希表管理字符串，字符串比较开销很小，又能省内存，缺点需要维护哈希表，字符串只可读不可改，全局唯一

Compare高效，Insert低效，字符串拼接(..)要用table.cancat



预设不回收的字符串



#### table

**复用度非常高**

数据容器，函数(Env)，元表(metastable)，模块(module)，注册表(Regitry)，其它...

k-v式访问接口

内部双容器算法

插入算法：

两种key：数字key及哈希key

插入算法：更新key对应值

数组域+哈希域选择

数组扩容：

1. 容量至少为最近2的n次方
2. 保证一半的元素可以放在数组里面



删除元素会触发表重构吗？

不会，而是改表结构。

更好的删除性能，代价是表只会越来越大。（构造新的表copy过去）

空间上不浪费，性能更好，（更加注重性能，因此删除是不会触发表重构）



遍历过程中允许删元素吗？

（**python中不可以**）lua中是可以的，因为删元素不会触发重构行为。加元素也是可以的（迭代器也不会失效）

不过尽量避免这样的设计。



怎么样构造table更高效？

1. 预分配数组空间

   `local t = {true, true, true} 甚至 local t = {nil, nil, nil}`，减少表重构

   `int lua_creatable(lua_State *L, int narr, int nrec);`

2. 顺序插入元素

   t[1] = 1, t[2] = 2, t[3] = 3 (不推荐)

   table.insert(t, v); table.remove(t, pos) (推荐)

next(), paris()乱序，why？

#tbl用法  （元素全部不为nil，其长度才是真实长度，因此最好不要用#tab来取长度，应该使用table.maxn）

#{1,nil,1nil} == 1



#### 元表（metatable）

重载

元表自身也可以有元表，可嵌套也可复用

![image-20210125171046451](C:\Users\kangzhongrun\AppData\Roaming\Typora\typora-user-images\image-20210125171046451.png)

可以用实现class机制

#### 函数env及作用域控制

lua函数可视范围，env是以table形式存在

![image-20210125171614357](C:\Users\kangzhongrun\AppData\Roaming\Typora\typora-user-images\image-20210125171614357.png)

是不是local变量，不是就去env表里去找，找不到则报错，lua没有全局的概念；

local范围，env范围；

不加local的变量，就进入函数的env表中

脚本泄漏和引擎泄漏



#### 函数

所有代码的执行单元都是函数，



#### closure & upvalue

闭包+上值

词法定界（lexical scoping）：



《Programming in Lua》

《The Implementation of Lua5.0》



#### lua垃圾回收算法

标记-清除（Mark-sweep）算法

双白色标记算法

Lua 5.2 分代标记算法



#### 协程coroutine



#### lua嵌入式及扩展



#### 错误处理机制及安全控制

Lua无try catch

底层实现 setjmp/longjmp



#### 模块管理机制增强

引入空模块拆循环引用；

![image-20210126085439661](C:\Users\kangzhongrun\AppData\Roaming\Typora\typora-user-images\image-20210126085439661.png)

![image-20210126085627528](C:\Users\kangzhongrun\AppData\Roaming\Typora\typora-user-images\image-20210126085627528.png)

```lua
-- comm_rule.lua
-- 模块初始化逻辑

function SystemStartup(self)
end

function __init__(self)
end

function __update__(self)
end

function __destroy__(self)
end


```

#### 代码热更新，游戏开发必备

comm_rule.lua

import.lua

#### GC策略自定制

gc是一个扫描算法，数量越大，gc越慢，优先考虑减对象数量（捷径）

Lua obj数量与gc效率

#### Coroutines

提高开发效率，更易维护性，降低开发难度；



corotuine前面检查的状态，coroutine之后需要再check（callback也需要，也就是在扣钱之前都需要check）



---



#### table

Lua中有一个全局的表_G，我们声明的任何变量、函数、table等都会放到`_G`这个表中，并用变量明或函数名进行索引。例如，声明一个函数

```lua
function test()
    print("test _G")
end
```

可以使用print(_G.test)得到输出：function: 005F862，说明这个函数被添加`_G`表中，对于其它变量也是同样

易错点：table提供了两个方法来获得数组大小：#y_table和table.getn(y_table)，这个方法只能求得数组的大小，Lua数组是指索引从1开始，并连续递增的，否则将出现错误，比如y_table = {idx=1}，得到结果都将是0



#### Lua灵魂之metatable和元方法

两个整数可以相加，但我们不能直接将两个表进行相加，可以使用metatable来实现，类似重载

```lua
mt = {}
test = {}
setmetatable(test, mt)
print (mt, getmetatable(test))

mt.__add = functtion(a, b)
	print (table.concat(a, ",").." ,"..table.concat(b, ','))
end
a = {1, 2, 3}
b = {4, 5, 6, 7}
setmetatable(a, mt)
setmetatable(b, mt)
c = a + b
-- 输出结果为1,2,3,4,5,6,7.
--[[a + b 是如何找到__add函数的？
Lua虚拟机在执行a+b时会去检查a和b之中是否有一个变量设置了元表，如果有一个变量设置了，
则会查找该元表中是否有一个__add函数，有则调用，只设置一个就可
]]
```

在元表中可定义`__index`和`__newindex`的行为，前者用于查询，后者用于修改

```lua
-- 局限性：只有当key在表中为nil时才能正确工作？？？否则将调用不到定义的操作
mt.__index = function(t, key)
end
mt.__newindex = function(t, key, v)
end
```



#### 面向对象编程

可以利用表的特性来是实现面向对象编程。Lua的世界中，一切皆对象，下面声明一个基类Base，Base本身是一个table

```lua
-- 定义Base类
Base = {base = 10, tv = 20}
Base.new = function(self, init)
    local obj = init or {}
    setmetatable(obj, self)
    self.__index = self
    return obj
end

-- o是类Base的一个对象，这里用到了__index是table的一种用法
o = Base:new()
print (o.tv)

-- 使用Base定义一个子类，子类也是对象，并继承了父类的所有东西
Child = Base:new()
Child.new = function(self, init)
    o = Base:new(init)
    o.chi = "child class"
    return o
end

local ch = Child:new()
print (ch.tv, ch.chi)

```



##### c/c++代码绑定到lua

##### 对象生命周期管理

引擎对象和脚本对象不要混合使用

##### Lua虚拟机改造

Coroutines支持跨C栈调用

luajit 2.0  （内核32位，寻找空间4G，真正使用的不能超过2G，加上各种限制1点几个G，如果服务器地址超过了，虚拟机必跪掉）

##### 脚本内存泄漏检测

泄漏到函数env表里去了。（把env变成只读？）

##### 多lua_State融合

外挂式编辑器（提高开发效率）

性能分担（利用多核，起多个VM分担cpu）

共享内存版C结构数据

##### luajit

数量级的提升

多进程，共享内存数据（userdata）



---

#### 坑总结

### require和module的坑

```lua
-- 第一种写法
-- 1. 模块定义文件a.lua
module(..., package.seeall)
function f() end

-- 2. 使用模块文件main.lua
local a = require "mod.a"
a.f()

-- 第二种写法
-- 1. 模块定义文件a.lua
module("a", package.seeall)
function f() end

-- 2. 使用模块文件main.lua
require "mod.a"
a.f()

-- 第三种写法（坑）
-- 1. 模块定义文件a.lua
module("a", package.seeall)
function f() end

-- 2. 使用模块文件main.lua
local a = require "mod.a" -- 返回a为true

```

源码中loadlib.c的ll_require()和ll_module()流程如下：

1. require先看看package.loaded["mod.a"]是否已经存在，如果有就直接返回了，这里没有，下一步。
2. 使用预先定义的loader列表里面的loader依次去尝试加载文件，这里会通过loader_Lua()把a.lua成功加载进来，然后通过lua_call执行a.lua的代码。
3. 在执行module("a", package.seeall)的时候，看看package.loaded["a"]是否已经存在了，如果不存在，就创建一个，package.loaded["a"]就是模块对应的table了。然后把当前函数执行环境设置为这个table。
4. a.lua执行完后，require看看有没有返回值，如果有，就直接设置到package.loaded["mod.a"]。通常都没有，就看看package.loaded["mod.a"]有没有值，如果有，就不管了。如果没有（这里是没有的，因为module设置的是package.loaded["a"]），那么require就设置package.loaded["mod.a"]为true。
5. 最后require返回package.loaded["mod.a"]

上面的问题其实就在于调用module和require时的模块名是不一样的，在这种情况下，require返回的就不是模块对应的table了，而是true。



##### index从1开始

##### 数组共享，新数组赋值会改变源数据

```lua
table1 = {[1] = "src"}
table2 = table1
table2[1] = "new"  -- table[1] == "new" 源数据被污染（如果两边数据不是同时修改）

-- 如何不溅射源数据呢？逐个数据赋值
function table.copy(tbl, ret)
    for k, v in pairs(tbl) do
        ret[k] = v
    end
end

local table2 = {}
table.copy(table1, table2)

```

##### 函数内嵌函数，不加local会有少量内存泄漏

```lua
function FuncA()
    function SubFunc()
    end
end
-- 子函数SubFunc没加local不会被内存回收
```

##### 排序比较函数，使用小于等于有bug

```lua
--  将输出 1,3,6,3,4而不是希望的结果1,3,3,4,6 这跟lua实现排序算法二分法有关，比较函数使用大于号或者小于号则没有这个问题
Tbl = {1, 3, 4, 3, 6}
function _Comp(a, b)
    return a <= b
end
table.sort(Tbl, _Comp)
for k, v in ipairs(Tbl) do
    print(v)
en
```

##### 在table的循环中，不要对table进行添加和删除操作（可以操作）

---

在函数的入口和出口插入统计代码来收集程序执行时的统计数据

嵌入到lua虚拟机中的kprofile模块，负责在lua函数的入口和出口出插入探测点，得到原始的函数调用数据；kdig进程，负责分析原始数据，得到统计结果



---

####  lua函数

1) 学习函数定义、参数传递、调用方式及返回值规则，思考如何在lua中使用具名参数（Named Argument）;
2) 需要深入理解闭包函数（closure)，建议投入充足时间确保可以深入理解，之后思考总结一些闭包的应用场景;
3) 需要深入理解upvalue，包括其生成、存储、生命周期、作用域等相关概念，编写代码测试之，并思考总结其应用场景；
4) upvalue与closure是lua非常独特的特性，可以有延展出很多有趣实用的编程技巧，需要非常重视；【自选】
5) 理解何为尾部调用，并在lua中编写代码测试，建议先通过递归让lua解释器抛出一次栈溢出错误，再将其改为尾部调用消除错误;
6) 理解lua中环境(Environments )这个概念，需要理解其对于函数存在的意义和呈现的形式（setfenv，getfenv）；【自选】
7) 理解lua程序中_G是什么，其内部包含哪些数据；【自选】



####  模块系统

1) 重点学习理解module、require两个模块关键字功能及含义；
2) 了解lua模块加载机制，学会如何自定义lua加载路径及加载器；
3) 编写自己的测试用模块，并且实现模块的热更新接口，即调用此接口会立即重新加载硬盘上修改过的模块代码并即时生效；
4) 总结在lua中模块是指什么，底层是如何实现，其存在价值是什么，有无可替代方案；【自选】

  lua内建库

1) 建议重点学习掌握math、string、table三个基本库的使用规则方法，例如string.gmatch、string.gsub；
2) 了解io、os库用法功能；
3) debug库列为进阶学习，编写测试代码获取指定调用栈的局部变量；【自选】



####  元表（metatable)

1) 了解元表的概念、作用方式及用途；
2) 了学习列出元表针对哪些数据类型定制哪些行为；
3) 自己动手实现特殊元表，思考并实现带默认值的table，即无需填入数据此table即包含默认数据；
4) 思考并实现只读table，即构建好后此table不允许写入任何数据；
5) 思考Weak Tables有和应用价值；【自选】



####  内建GC机制

1) 学习了解lua的gc策略及控制方式；
2) 利用collectgarbage("count"),编写测试脚本，调整gc参数观察lua内存占用情况并总结；【自选】
3) 为测试数据自定义元表中的__gc方法，观察数据被回收的时间点是否符合预期；【自选】



####  编译调试

1) 理解对lua而言编译意味着什么，理解虚拟机、字节码、脚本源码的含义及之间的关系；
2) 可以清晰给出loadstring、loadfile、dofile四个内建api的区别；
3) 学习使用pcall、xpcall，并定义自己的错误处理函数输出调用栈，进阶可以试输出每层栈的局部变量；
4) 试用string.dump及loadstring，研究实现和作用原理；



####  协同程序(coroutine)

1) 理解何为协作式多线程以及与抢占式多线程的区别；
2) 练习使用协程，思考列出其可能的应用场景；



####  lua嵌入与扩展

1) 理解何为lua宿主程序的概念，列出lua与C\C++的不同；
2) 学习如何在宿主程序（例如C语言）与lua虚拟机中传递数据及相互调用，需要深刻理解lua专为此门设计的数据栈概念（virtual stack）；
3) 编写测试代码，在C中访问脚本中定义的数据，例如如何遍历脚本中一个多层结构的table数据；
4) 学习如何在C中如何引用住脚本中的对象，控制其垃圾回收周期；
5) 编写测试代码在C中调用脚本中自定义的函数，测试各种参数类型的传递。建议时刻关注数据栈平衡，慎防栈内数据泄露；
6) 按照文档实现自己的lua_CFunction函数，将其注册到脚本中测试使用；
7) 测绘在C中，给自定义的lua_CFunction添加upvalue，以及如何访问提取其他函数中的upvalue;
8) 结合脚本中的错误处理技巧，在C中以lua c api的方式尝试重建并测试它们。



#### 自定义数据

1) 掌握如何在C中自定义新的数据类型并注册给脚本使用；
2) 思考userdata与light userdata的区别于应用场景；









---

```c
// stdarg.h

// va_list表示可变参数列表类型
typedef char *va_list;

// 获取可变参数列表的第一个参数的地址, list是类型为va_list的指针，param1是可变参数最左边的参数
#define va_start(list,param1)   ( list = (va_list)&param1+ sizeof(param1) )

// 获取可变参数的当前参数，返回指定类型并将指针指向下一参数（mode参数描述了当前参数的类型）
// va_arg用于获取当前所指的可变参数并将并将指针移向下一可变参数
#define va_arg(list,mode)   ( (mode *) ( list += sizeof(mode) ) )[-1]

// 清空va_list可变参数列表
#define va_end(list) ( list = (va_list)0 )
```
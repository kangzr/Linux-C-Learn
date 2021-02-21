pitfalls

0 is treated as true

parameters are filled with nil if not provided



```
augroup filetype_lua
        autocmd!
        autocmd FileType lua setlocal iskeyword+=:
augroup END
```





##### Lua内存管理lmem.h

内存管理方法最终调用luaM_realloc_管理所有的内存分配/释放；根据Lua_state初始化时设置的g->frealloc分配空间

```c
// luaM_realloc_ : 用于分配和释放内存（外部接口 luaM_free, luaM_new）

// luaM_growaux_: 用于管理变长数组（外部接口：luaM_grow）
```



##### Lua数据类型

<img src="D:\MyGit\Linux-C-Learn\pic\lua_object.png" alt="lua_object" style="zoom:75%;" />

Lua数据接口：Number(int, float)，nil, boolen, string, table, fucntion, lightuserdata, thread

##### String

Lua的字符串分为短字符和长字符，其中短字符的具体数据存放在G中Stirng表中

<img src="D:\MyGit\Linux-C-Learn\pic\lua_string.png" alt="lua_string" style="zoom:75%;" />



##### Table

![lua_string](D:\MyGit\Linux-C-Learn\pic\lua_table.png)

##### 1. maiposition(const Table *t, const TValue *key)

计算每个key的hash值作为index，获取Node节点

长字符串：惰性计算（lstring的luaS_hash函数）

短字符串：直接值用TString自带hash查找

Float：尝试lvm中的luaV_tointerger函数将其转换为int，否则在hash中查找

int：直接取余

##### 2. hash表

luaH_get，luaH_getstr，getgeneric，luaH_set，rehash(重建table结构)，findindex

![lua_func](D:\MyGit\Linux-C-Learn\pic\lua_func.png)



#### closure

(1)`luaD_pcall`和`luaD_call`用于调用函数。`luaD_pcall`基于`luaD_call`对异常做保护, `luaD_call`使用`luaV_execute`执行指令集.

![lua_closure](D:\MyGit\Linux-C-Learn\pic\lua_closure.png)





1. lua采用**增量标记-清除回收器**

- 标记：每次执行GC时，先以若干根节点(**mainthread主线程(协程), 注册表(registry), 全局元表(metatable), 上次GC循环中剩余的finalize中的对象**)开始，逐个把直接或间接和它们相关的节点都做上标记
- 清除：当标记完成后，遍历整个对象链表(global_State的allgc链表)，把被标记为需要删除的节点一一删除即可。
- Lua中的GC实现为**增量标记-清除回收器**, 每次调用luaC_step都会执行一次增加回收.



2. lua用白、灰、黑三色来标记一个对象的可回收状态

- 白色(可回收状态): 如果该对象未被GC标记过, 则此时白色代表当前对象为待访问状态。举例：新创建的对象的初始状态就应该被设定为白色，因为该对象还没有被GC标记到，所以保持初始状态颜色不变，仍然为白色。如果该对象在GC标记阶段结束后，仍然为白色则此时白色代表当前对象为可回收状态。但其实本质上白色的设定就是为了标识可回收。
- 灰色(中间状态): 当前对象为待标记状态。举例：当前对象已经被GC访问过，但是该对象引用的其他对象还没有被标记。
- 黑色(不可回收状态): 当前对象为已标记状态。举例：当前对象已经被GC访问过，并且对象引用的其他对象也被标记了。



[参考](https://blog.gotocoding.com/archives/1024)



Lua源码目录结构

1. lua入口函数和配置文件

   lua.c/h  

   luac.c调用解析Lua源码生成二进制文件

   luaconf.h

2. Lua变量的定义

   lstate.h: state objects

   lobject.h: tagged valued and object representation

3. 内存管理和内置变量实现

   lmem.c: 内存管理

   lstring.c: string interning

   ltable.c: 使用数组+hash表来构成table，Node（=key+value）表示hash节点，key使用链表方式处理相同的hash值

   ltm.c: metamethod handling

4. Lua闭包和VM相关

   func.c/h

   ldo.c: calls, stacks, exceptions, coroutines, tough read

   lvm.c: scroll down to luaV_execute, the main interpreter loop

5. GC相关

   lgc.c/h: incremental garbage collector，使用三色标记算法完成GC算法

6. Lua解析相关

   lparser.c: recursive descent parser, targetting a register-based VM. start from chunk() and work your way through.

   llex.c/h: the expression parser

   lopcodes.h/c: bytecode instruction format and opcode definitions. easy.

   lcode.c/.h: the code generator

7. 其它

   - linit.c
   - lapi.c: Check how the API is implemented internally.
   - ldebug.c/h: abstract interpretation is used to find object names for tracebacks.
   - ldump.c
   - lundump.c/h

8. 相关库：lualib，llimits















#### 栈的维护

void lua_settop  (lua_State *L, int index);//设置栈的top为index，（index>=0时）等同于设置栈的大小为index

void lua_pushvalue (lua_State *L, int index);//push一个index位置的copy在栈顶

void lua_remove  (lua_State *L, int index);//移除index位置的元素

void lua_insert  (lua_State *L, int index);//把栈顶元素插入到指定index位置（栈的大小还是没变，等价于把栈顶元素和指定的index位置元素互换）



##### Push C Value 到栈上

这些函数接收C的值value，并转换为对应的Lua值value，然后push到lua_State栈中



void lua_pushnumber  (lua_State *L, double n);
void lua_pushlstring  (lua_State *L, const char *s, size_t len);
void lua_pushstring  (lua_State *L, const char *s);
void lua_pushusertag  (lua_State *L, void *u, int tag);
void lua_pushnil    (lua_State *L);
void lua_pushcfunction (lua_State *L, lua_CFunction f);//C函数都链接在L->rootcl双向链表中，Lua函数都链接在L->rootproto双向链表中

 

int lua_newtag (lua_State *L); 

void lua_settag (lua_State *L, int tag);//改变栈顶元素的tag





C中调用Lua中定义的函数

- push 调用的函数到Lua栈中；
- push调用的参数到栈中（first arg first push）；
- 使用下面的API函数调用：int lua_call (lua_State *L, int nargs, int nresults);
- 调用后，函数和参数从Lua栈中排pop出来，函数调用结果被放入Lua栈中（first result first push）。



##### 向栈中压入元素

Lua中调用C函数时，C中会把值封装成Lua类型的值压入栈中，在压入栈时，会在Lua虚拟机中生存一个副本，比如lua_pushstring()会向栈中压入由s指向的以'\0'结尾的字符串, 在C中调用这个函数后，可以任意释放或者由s指向的字符串。Lua不会持有指向外部字符串的指针，也不会持有指向任何其它外部对象的指针。

总之，一旦C中值被压入栈中，Lua会生成相应的接口（Lua中实现的数据类型）并管理这个值，从此不再依赖于原来的C值。



##### 获取栈中的元素

通常以lua_to*开头，比如lua_tonumber, lua_tostring... 在C函数中，可以调用这些接口获取从Lua中传递给C函数的参数。

lua_isnumber, 检查是否为数字类型。



lua_call(lua_State *L, int nargs, int nresutls)

调用栈中的函数。



luaL_newstate()：初始化Lua虚拟机返回一个指向lua_State类型的指针。几乎所有的API第一个参数类型都是lua_State*

```c
// lstate.h
struct lua_State {
    CommonHeader;
    ...
    CallInfo *ci;		//保存当前正在调用的函数的运行状态
    ...
}
```

调用栈实质上用一个双向链表来实现，链表中的每个节点是用一个CallInfo的结构体来实现，保存正在调用的函数的运行状态。

```c
typedef struct CallInfo {
    StkId func;  // 指向被调用的函数在栈中的位置
    StkId top;  // 指向被调用的函数可以使用栈空间最大的位置
    struct CallInfo *previous, *next;  // 指向调用链表的前一个节点和下一个节点
}CallInfo;
```





Python虚拟机是典型的stack based，lua是register based

print(debug.traceback())







##### 闭包

```c
typedef union Closure {
    CClosure c;  // c闭包
    LClosure l;  // Lua闭包
} Closure;

// Lua闭包
typedef struct LClosure {
    ClosureHeader;
    struct Proto *p;
    UpVal *upvals[1];
} LClosure;

// Proto
typedef struct Proto {
    CommonHeader;
    struct Proto **p;
    int *lineinfo;
    Upvaldesc *upvalues;
    ...
};
```





#### string

```c
typedef union TString {
    L_Umaxalign dummy;
    struct {
        CommonHeader;
        lu_byte extra;
        unsigned int hash;
        size_t len;
    }tsv;
} TString;
```

所有短字符串都会被存放在一个全局的hash表中。与table的hash表不一样，stringtable采用的是开散列。当stringtable中的元素个数大于stringtable的大小时，进行rehash操作。短字符串在进行内部化时，首先会查找stringtable中有没有这个元素，如果有， 直接返回，否则创建一个返回，这样就保证了相同的短字符串同时只会存在唯一一份。



##### table

```c

```




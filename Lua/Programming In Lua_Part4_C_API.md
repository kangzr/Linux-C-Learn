#### 前言

《Programming In Lua》Part IV The C API



#### 1. An Overview of the C API

Lua为嵌入式的和可扩展的语言，相应的C和Lua之间有两种交互方式，第一种在C中调用Lua的库，第二种Lua中调用C的库。应用代码（Application Code）和库代码（Lib Code）同Lua交流使用相同的API，就是所谓的C_API，C_API是一组能和Lua进行交互的函数、常量和类型。Lua独立的解析器(lua.c)为应用代码的实现，标准库(lmathlih.c等)提供了库代码的实现，可以借鉴。



#####  A First Example

一个基本的独立的Lua解释器

```c
#include <stdio.h>
#include <string.h>
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

int main (void) {
    char buff[256];
    int error;
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);  // 打开标准库
    while (fgets(buff, sizeof(buff), stdin) != NULL) {
        // luaL_loadstring, lua_pcall调用成功会返回0
        error = luaL_loadstring(L, buff) || lua_pcall(L, 0, 0, 0);
        if (error) {
            fprintf(stderr, "%s\n", lua_tostring(L, -1));
            lua_pop(L, 1);
        }
    }
    lua_close(L);
    return 0;
}
```

`lua.h`：申明Lua提供的基本函数，比如创建一个新的Lua环境，调用Lua函数，读写环境中的全局变量，注册被Lua调用的新函数等等。所有申明在`lua.h`中的函数，都有一个前缀`lua_`，比如`lua_pcall`

`laxulib.h`：申明了由`auxiliary library`提供的函数，其中所有函数都有前缀`luaL_`，比如`luaL_loadstring`。其不能访问Lua内部的代码，而是基于`lua.h`库封装出来的更高级的库。

Lua库没有定义c全局变量，其所有的状态都保存在动态结构`lua_State`中，通过`luaL_newstate`可以创建一个新的Lua状态。`lualib.h`中定义了打开标准库的函数`luaL_openlibs`



##### The Stack（虚拟栈）

Lua和C之间通过虚拟栈进行沟通，几乎所有的API调用都会操作栈上的值，Lua和C所有的数据交换都在虚拟栈上进行，也可使用栈来保存一些中间结果。虚拟栈遵循后入先出形式。

对于元素入栈，每一种Lua类型都有一个入栈API，如下：

```c
void lua_pushnil (lua_State *L);
void lua_pushboolean (lua_State *L, int bool);
void lua_pushnumber (lua_State *L, lua_Number n);  // for doubles  float in lua
void lua_pushinteger (lua_State *L, lua_Integer n);  // for integer
void lua_pushlstring (lua_State *L, const char *s, size_t len);  // a pointer to char + length
void lua_pushstring (lua_State *L, const char *s);  // zero-terminated strings
```

还有C函数（`lua_pushcfunction`）和userdata入栈的API后续再讨论。

虚拟栈默认有20个位置（`lua.h`中`LUA_MINSTACK`定义），对于大多数情况是够用的，对于一些特殊情况，需要使用`lua_checkstack`来检查是否有足够的空间，如果空间不够，会增加栈的空间，如下

```C
// lua.h
int lua_checkstack (lua_State *L, int sz);

// auxiliary laxulib.h
// 如果出错，会抛出错误信息
void luaL_checkstakc(lua_State *L, int sz, const char *msg)
```



第一个入栈元素索引值为1，第二个为2... 也可以用-1来索引栈顶的元素，-2来索引倒数第二个元素。比如使用`lua_tostring(L, -1)`以字符串形式返回栈顶元素。API提供了一组函数还检查栈中的元素类型`lua_is*`其中*代表lua中的类型，比如`lua_isnil`, `lua_isnumber`等。所有函数原型为：

```c
int lua_is* (lua_State *L, int index)
```

实际上，`lua_is*`不仅仅检查栈中元素是否为该类型，可以转换为对应类型的也可。



从栈中获取元素，可以使用`lua_to*`这组函数，如下：

```c
int lua_toboolean(lua_State *L, int index);
const char *lua_tostring(lua_State *L, int index, size_t *len);
lua_State *lua_tothread(lua_State *L, int index);
lua_State *lua_tonumber(lua_State *L, int index);
lua_State *lua_tointeger(lua_State *L, int index);
```

通过实现一个将栈上所有的内容dump打印出来的例子来说明这些接口的使用：

```c
// Dumping the Stack
static void stackDump (lua_State *L) {
    int i;
    int top = lua_gettop(L);  // 栈中元素个数
    for (i = 1; i <= top; i++) {
        int t = lua_type(L, i);   // 获取i位置元素类型
        switch (t) {
            case LUA_TSTRING: {
                printf("'%s'", lua_tostring(L, i));
                break;
            }
            case LUA_TBOOLEAN: {
                printf(lua_toboolean(L, i) ? "true" : "false");
                break;
            }
            case LUA_TNUMBER: {  // 这里可用lua_isinteger来区分整数和浮点型
                printf("%g", lua_tonumber(L, i));
                break;
            }
            default: {
                printf("%s", lua_typename(L, t));
                break;
            }
        }
        printf("  ");
    }
    printf("\n");
}
```



其它一些栈的操作

```c
// gettop返回栈上元素个数
int lua_gettop (lua_State *L);
// settop设置栈的大小(栈上元素个数)
// 如果index大于当前size，可用nil补全，如果小于则丢弃多余元素，为0则清空栈
// #define lua_pop(L,n) lua_settop(L, -(n) - 1)
void lua_settop (lua_State *L, int index);
// 将给定index位置的元素压入栈顶
void lua_pushvalue (lua_State *L, index);
// 从给定位置开始向栈顶移动n个位置(n<0则反向)
// lua_remove元素就是基于rotate实现，如下：
// #define lua_remove(L,idx) (lua_rotate(L, (idx), -1), lua_pop(L, 1))
void lua_rotate (lua_State *L, int index, int n);
// 将index位置元素移除
void lua_remove (lua_State *L, int index);
// 将栈顶元素插入给定位置，这个位置以上的元素向上移动
void lua_insert (lua_State *L, int index);
// 用栈顶元素pop，替换给定位置元素
void lua_replace (lua_State *L, int index);
// 将fromidx位置的元素移动到toindx位置
void lua_copy (lua_State *L, int fromidx, int toindx);
```

还是用一个例子来说明栈操作相关接口的使用：

```c
#include <stdio.h>
#include "lua.h"
#include "lauxlib.h"
stack void stackDump (lua_State *L){
    // 之前的Dumping The Stack
}
int main(void) {
    lua_State *L = luaL_newstate();
    lua_pushboolean(L, 1);
    lua_pushnumber(L, 10);
    lua_pushnil(L);
    lua_pushstring(L, "hello");
    stackDump(L);  // print: true 10 nil "hello"
    lua_pushvalue(L, -4); stackDump(L); // print: true 10 nil "hello" true
    lua_replace(L, 3); stackDump(L); // print: true 10 true "hello"
    lua_settop(L, 6); stackDump(L); // print: true 10 true "hello" nil nil
    lua_rotate(L, 3, 1); stackDump(L); // print: true 10 nil true "hello" nil
    lua_remove(L, -3); stackDump(L); // print: true 10 nil "hello" nil
    lua_settop(L, -5); stackDump(L); // print: true
    lua_close(L);
    return 0;
}
```



##### Error handling（异常处理）

一些操作可以抛出错误：访问全局变量触发`__index`元方法抛出错误，分配内存操作最终触发垃圾回收，并通过`finalizers`抛出错误，总而言之，大量的Lua API都能抛出错误。因为C语言没有提供异常处理，面对这种困难，Lua是使用C中的`setjmp`来实现异常处理机制的。

在应用代码（c代码）中通过`lua_pcall`调用C函数的方式来实现安全调用和进行异常处理，如下：

```c
int secure_foo (lua_State *L) {
    lua_pushcfunction(L, foo);  // 将foo压入栈
    return (lua_pcall(L, 0, 0, 0) == 0);  // lua_pcall调用栈上foo
}
```

为Lua写的库函数通过`pcall`来处理异常。



##### Memory Allocation（内存分配）

Lua提供了默认的内存分配机制`malloc-realloc-free`，基于C标准库，如下：

```c
void *l_alloc (void *ud, void *ptr, size_t osize, size_t nsize) {
    (void)ud; void(osize);
    if (nsize == 0) {
        free(ptr);
        return NULL;
    }
    else
        return realloc(ptr, nsize);
}
```

我们也可以自己定制内存分配机制，`lua_newstate`提供了相应的参数

```c
lua_State *lua_newsate (lua_Alloc f, void *ud);

// lua_Alloc形式
typedef void * (*lua_Alloc) (void *ud, void *ptr, size_t osize, size_t nsize);
```

也可通过`lua_getallocf`和`lua_setallocf`来获取和设置内存分配器

```c
lua_Alloc lua_getallocf(lua_State *L, void **ud);
void lua_setallocf(lua_State *L, lua_Alloc f, void *ud);
```

##### Exercises

```c
// 写一个库允许脚本限制Lua虚拟机使用的内存大小，通过setlimit来进行限制
void *allocator(void *ud, void *ptr, size_t osize, size_t nsize)
{
    if (nsize == 0){
        free(ptr);
        return NULL;
    }
    else{
        int* limit = (int*)ud;  // 取出限制大小
        if (limit != NULL && *limit < nsize) {
            printf("fail, out of memory!\n");
            return NULL;
        }
        return realloc(ptr, nsize);
    }
}
void setlimit(lua_State* L, int* memsize) {
    lua_setallocf(L, allocator, memsize);  // memsize放在userdata中？
}
```



#### 2. Extending Your Application

##### The Basics

使用Lua配置文件来定义窗口大小，并解析获得相关参数，如下：

```c
// getting user information from a configuration file
int getglobint (lua_State *L, const char *var) {
    int isnum, result;
    lua_getglobal(L, var);  // 把var对应的值压入栈顶
    result = (int)lua_tointegerx(L, -1, &isnum);  // 把栈顶元素转换成Integer
    if (!isnum)  // isnum用来判断是否成功
        error(L, "'%s' should be a number\n", var);
    lua_pop(L, 1);  // 将结果pop
    return result;
}

void load (lua_State *L, const char *fname, int *w, int *h) {
    if (luaL_loadfile(L, fname) || lua_pcall(L, 0, 0, 0))
        error(L, "cannot run config. file: %s", lua_tostring(L, -1));
    *w = getglobint(L, "width");
    *h = getglobint(L, "height");
}
```

对于table的操作可以使用`lua_settable`和`lua_gettable`。还有对应的更便捷的接口`lua_setfield`和`lua_getfield`



##### Calling Lua Functions （C中调用Lua函数）

Lua的一个优点是其配置文件可以定义函数并被应用程序调用。如下在C中调用Lua函数

```c
// 调用Lua定义的函数f
double f (lua_State *L, double x, double y) {
    int isnum;
    double z;
    // 将函数和参数压入栈
    lua_getglobal(L, "f");
    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    // 调用f，2个参数，1个返回值，（会把函数和参数从栈中pop出来）
    if (lua_pcall(L, 2, 1, 0) != LUA_OK)
        error(L, "error running function'f': %s", lua_tostring(L, -1));
    z = lua_tonumberx(L, -1, &isnum);
    if (!isnum)
        error(L, "function'f' should return a number");
   	lua_pop(L, 1);
    return z;
}
```

**lua_pcall**

原型：`int lua_pcall(lua_State *L, int nargs, int nresults, int msgh)`

功能：在保护模式下调用一个函数

如果没有抛错的话，`lua_pcall`同`lua_call`，如果有错误发生，`lua_pcall`会捕获错误，并将错误信息压入栈，并返回一个错误代码。`lua_pcall`会将函数和参数从栈上pop掉。



##### A Generic Call Function（通用函数调用）

可使用`stdarg`标准库来封装对lua函数的调用，如下：

`call_va(L, "f", "dd>d", x, y, &z)`

其中`dd>d`表示2个double类型参数，返回1个double类型的结果。d代表double，i代表integer，s调用string。`>`用来分隔参数和结果，如果不需要返回结果则这个>可有可无。

一个通用的函数调用如下：

```c
#include <stdarg.h>
void error(lua_State *L, const char *fmt,...)
{
    va_list argp;
    va_start(argp, fmt);
    vfprintf(stderr, fmt, argp);
    va_end(argp);
    lua_close(L);
    printf("\nEnter any key to exit");
    getchar();
    exit(EXIT_FAILURE);
}

void call_va (lua_State *L, const char *func, const char *sig, ...) {
    va_list vl;
    int narg, nres;
    va_start(vl, sig);
    // 函数入栈
    lua_getglobal(L, func);
    // 参数入栈
    for (narg = 0; *sig; narg++) {
        luaL_checkstack(L, 1, "too many args");
        switch (*sig++) {
            case 'd':
                lua_pushnumber(L, va_arg(vl, double));
                break;
            case 'i':
                lua_pushinteger(L, va_arg(vl, int));
                break;
            case 's':
                lua_pushstring(L, va_arg(vl, char*));
                break;
            case '>':
                goto endargs;
            case 'b':
                lua_pushboolean(L, va_arg(vl, int));
                break;
            default:
                error(L, "invalid option (%c)", *(sig - 1));
        }
    }
    endargs:
    nres = strlen(sig);  // 返回结果个数
    if (lua_pcall(L, narg, nres, 0) != 0)
        error(L, "error calling '%s': %s", func, lua_tostring(L, -1));
    nres = -nres;  // 第一个结果的栈索引
    while (*sig) {
        switch (*sig++) {
            case 'd': {
                int isnum;
                double n = lua_tonumber(L, nres, &isnum);
                if (!isnum)
                    error(L, "wrong result");
                *va_arg(vl, double*) = n;
                break;
            }
            case 'i':
                {
                    int isnum;
                    int n = lua_tointegerx(L, nres, &isnum);
                    if(!isnum)
                      error(L, "wrong result type");
                    *va_arg(vl, int*) = n;
                    break;
                }
            case 's':
                {
                    const char *s = lua_tostring(L, nres);
                    if(s == NULL)
                      error(L, "wrong result type");
                    *va_arg(vl, const char **) = s;
                    break;
                }
            case 'b':
                {
                    if(lua_isboolean(L, nres))
                      error(L, "wrong result type");
                    bool b = lua_toboolean(L,nres);
                    *va_arg(vl, bool*) = b;
                    break;
                }
            default:
                error(L, "invalid option (%c)", *(sig - 1));
            }
        }
        nres++;
    }
    va_end(vl);
}
```



##### Excercise

```lua
-- 1 分析以下配置方式的优缺点
-- config1
ws_1 = "https://weather.com/"
ws_2 = "http://www.nmc.cn/"
ws_3 = "http://data.cma.cn/"
--优点:配置简单快速 适合数量少的
--缺点:拓展性很差，无法做额外的规则配置，而且在数量众多的情况下会产生大量的全局变量，降低了访问效率;且容易导致重名。

-- config2
ws_table = {
    ws_1 = "https://weather.com/",
    ws_2 = "http://www.nmc.cn/",
    ws_3 = "http://data.cma.cn/",
}

--优点:易于配置，可拓展性，可通过表的特性来对配置进行一些修改（比如排序之类的）。可以制定一些特殊的映射规则。
--缺点:数量众多的情况下会占用较大内存。

-- config3
function get_ws_url(ws_str)
    if ws_str == "ws_1" then
        return "https://weather.com/"
    elseif ws_str == "ws_2" then
        return "http://www.nmc.cn/"
    elseif ws_str == "ws_3" then
        return "http://data.cma.cn/"
    end
end
--优点:占用内存小
--缺点:存在字符串的比较开销 访问速度没有前两种高。
```



#### 3. Calling C from Lua

Lua中可以调用C函数，但并不能调用所有的C函数，C函数必须遵循一定规则来获取参数和返回结果，此外，还需注册C函数，将其地址传给Lua。Lua调C和C调Lua使用同样的虚拟栈，C函数可以将从栈中获取参数并将结果压入栈。

##### C Functions

任何注册给Lua的函数都有相同的原型

```c
// lua.h
typedef int (*lua_CFunction) (lua_State *L);
```

以下为sin函数实现：

```c
static int l_sin (lua_State *L) {
    double d = luaL_checknumber(L, 1);  // 检查参数类型
    lua_pushnumber(L, sin(d));  // 将结果压入栈
    // 返回结果数量，C不需要自己清空栈，Lua会自动清空并保存结果
    return 1;
}
```

在Lua脚本中调用这个函数前，需要进行注册：

```c
 // 获得c函数的指针，并创建一个函数类型，并将其压入栈
lua_pushcfunction(L, l_sin);
// 将该函数命名为mysin
lua_setglobal(L, "mysin");
```

以下为一个更为复杂例子，编写一个函数返回给定目录下所有的内容

```c
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include "lua.h"
#include "lauxlib.h"

static int l_dir (lua_State *L) {
    DIR *dir;
    struct dirent *entry;
    int i;
    const char *path = luaL_checkstring(L, 1);  // 检查string类型
    dir = opendir(path);
    if (dir == NULL) {
        lua_pushnil(L);
        lua_pushstring(L, sterror(errno));
        return 2;
    }
    // 创建一个table并将其压入栈
    lua_newtable(L);
    i = 1;
    while ((entry = readdir(dir)) != NULL) {
        lua_pushinteger(L, i++);  // key 入栈
        lua_pushstring(L, entry->d_name);  // value 入栈
        // settable在索引-3位置的table中插入kv，并把key, value出栈，
        lua_settable(L, -3);  // table[i] = entry_name
    }
    closedir(dir);
    return 1;
}
```



##### C Modules

定义一组能被Lua调用的C函数，这个模块需要注册所有的C函数并保存在一个table中，当一个C函数保存在Lua中，Lua可以直接通过函数地址的引用来进行调用。

通常将所有的C函数放在一个数组中

```c
// mylib.c
static const struct luaL_Reg mylib [] = {
    {"dir", l_dir},
    {NULL, NULL}  // 哨兵，表示结束
};

// 创建一个新table，将mylib中的内容放入table中。
int luaopen_mylib (lua_State *L) {
    luaL_newlib(L, mylib);
    return 1;
}
```

接着编译上述库文件，生产对应的so文件，然后将其放入对应的C路径下，就可以在lua脚本中使用了

```lua
local mylib = require "mylib"
```



##### Exercise

```c
// 用C写一个库文件，里面包含下述可供Lua调用的函数
// 1. 写一个接收任意参数的函数summation，计算所有参数的和
static int summation(lua_State* L) {
    lua_Number sum = 0;
    int n = lua_gettop(L);
    for (int i = 1; i <= n; i++)
        sum += luaL_checknumber(L, i);
    lua_pushnumber(L, sum);
    return 1
}
// 2. 实现一个类似table.pack的函数
static int pack(lua_State *L) {
    int n = lua_gettop(L);
    lua_newtable(L);
    lua_insert(L, 1);
    for (int i = n; i >= 1; i--) {
        lua_pushinteger(L, i);
        lua_insert(L, -2);
        lua_settable(L, 1);
    }
}
// 3. 实现一个函数接收任意参数，然后反转输出
static reverse(lua_State* L) {
    int n = lua_gettop(L);
    for(int i = 1;i < n;i++) lua_insert(L, i);
    return n;
}

static const struct luaL_Reg mylib [] = {
    {"summation", summation},
    {"pack", pack},
    {"reverse", reverse},
    {NULL, NULL},
};
LUAMOD int luaopen_mylib(lua_State *L) {
    luaL_newlib(L, mylib);
    return 1;
}
void luaL_openmylib(lua_State* L)
{
    luaL_requiref(L,"mylib",luaopen_mylib,1);
    lua_pop(L, 1); 
}

int main()
{
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    luaL_openmylib(L);
}
```



#### 4. Techniques for Writing C Functions

##### Array Manipulation

我们可以使用`lua_settable`和`lua_gettable`来操作table，此外API还提供了`lua_geti`和`lua_seti`来访问和更新table，

```c
// index表示table在栈上的位置，key为要访问的key
void lua_geti (lua_State *L, int index, int key);
// lua_geti(L, t, key) 等同于
lua_pushnumber(L, key);
lua_gettable(L, t);

void lua_seti (lua_State *L, int index, int key);
// lua_seti(L, t, key) 等同于
lua_pushnumber(L, key);
lua_insert(L, -2);
lua_settable(L, t);
```

以下为一个具体的例子，用C实现一个Map函数，将数组中每一个元素作为参数传入函数f，并将执行结果替换为数组中原来的元素。

```c
// function map in c
int l_map (lua_State *L) {
    int i, n;
    luaL_checktype(L, 1, LUA_TTABLE);  // 第一个参数为table
    luaL_checktype(L, 2, LUA_TFUNCTION);  // 第二个参数为函数
    n = luaL_len(L, 1);  // 获取table中元素个数
    for (i = 1; i <= n; i++) {
        lua_pushvalue(L, 2);  // 将函数f入栈
        lua_geti(L, 1, i);  //将t[i]入栈
        lua_call(L, 1, i);  //调用 f(t[i])
        lua_seti(L, 1, i);  //t[i] = result
    }
}
```



##### String Manipulation

以下为分割字符串例子

```c
// 输入("hi:ho:there", ":") 输出{"hi", "ho", "there"}
static int l_split (lua_State *L) {
    const char *s = luaL_checkstring(L, 1);
    const char *sep = luaL_checkstring(L, 2);
    const char *e;
    int i = 1;
    lua_newtable(L);
    while ((e = strchr(s, *sep)) != NULL) {
        lua_pushlstring(L, s, e - s);
        lua_rawseti(L, -2, i++);
        s = e + 1;  // 跳过分隔符
    }
    lua_pushstirng(L, s);
    lua_rawseti(L, -2, i);
    return 1;
}
```



##### Storing State in C Functions

##### Upvalues

##### Exercise



#### 5. User-Defined Types in C

##### Userdata

##### Metatables

##### Object-Oriented Access

##### Array Access

##### Light Userdata



#### 6. Managing Resources



#### 7. Threads and States


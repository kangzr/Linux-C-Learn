#### table

Lua中唯一的数据结构，动态数组和哈希表的结合体

```c
typedef union TKey {
    struct {
        TValuefields;
        struct Node *next;
    } nk;
    TValue tvk;
}TKey;

typedef struct Node {
    TValue i_val;
    TKey i_key;
} Node;

typedef struct Table {
    CommonHeader;
    lu_byte flags;  // 用于元表的优化，标志相关的元方法是否存在，不存在就不需要查找元表了。
    lu_byte lsizenode;  // 哈希部分大小, 2 ^ S
    struct Table *metatable; // 记录了table的元表
    TValue *array;  // 数组部分
    Node *node;  // 哈希部分
    Node *lastfree;  // 用于hash表 新建元素时的优化。
    GCObject *gclist;
    int sizearray;  // 数组部分的大小
}Table;
```



N个坐标，随机选出M个

版本一：

```lua
POS = {...}  -- 坐标点的配置
function refresh_monster(num)
    --  copy一份坐标点
    local my_pos = {}
    for i,v in ipairs(POS) do
        my_pos[i] = v
    end
    -- 随机选点
    for i = 1, num do
        local rand = math.random(#my_pos)
        local pos = table.remove(my_pos, rand)
        --pos点刷怪
    end
end
```

- table copy的开销
- table数组部分移动元素的开销
- 如何让它更快？

版本二：

```lua
POS = {...}
function refresh_monster_opt(num)
    local left_num = #POS
    for i = 1, num do
        local rand = math.random(left_num)
        local pos = POS[rand]
        -- 刷怪
        -- 把随机过的放到最后
        POS[rand] = POS[left_num]
        POS[left_num] = pos
        left_num = left_num - 1
    end
end
```

N越大，M/N越小，效率差异越大

不考虑刷怪本身的开销前提下，N==1000, M==100时，两者消耗的时间开销大致位10：1

TIPS：避免数组copy和移动

- 如果避免不了copy，也要避免逐个设置这种低效的实现，可以考虑自己实现一个memcpy
- 从算法层面去规避数组移动



如何提高下面这段代码的效率？

```lua
local array = {}
for i = 1, 100000 do
    array[i] = i
end
```

table的rehash（table中新增元素时找不到空闲位置会执行rehash来增长table）

- 计算需要的数组和哈希表长度
- 如果现有空间不足则需要重新分配
- 将元素重新分配到数组和哈希表

当表具有一定大小时，rehash时比较费时的。

因此应当尽可能减少rehash次数：

- 预分配，利用lua_createtable预先分配内存



#### 字符串

```c
typedef union TSting{
    L_Umaxalign dummy;
    struct {
        CommonHeader;
        lu_byte reserved;
        unsigned int hash;
        size_t len;
    }tsv;
}TString;
```



#### 全局字符串哈希表

- 所有的字符串都有且只有唯一一份拷贝在全局字符串哈希表中；
- 开链的哈希表
- 哈希函数：mod表示大小
- 满时翻倍扩容，并重新把字符串插入
- 全局字符串表是可以缩小的



##### 生成一个新的字符串步骤：

- 计算哈希值
- check是否在全局字符串哈希表中
- 分配空间和初始化TString数据结构
- 检查扩展全局字符串哈希表
- 插入到全局哈希表



##### tips 避免生成临时字符串



##### 函数

准确的说都叫闭包closure

```c
typedef union Closure {
    CClosure c;
    LClosure l;
}Closure;

typedef struct CClosure {
    ClosureHeader;
    lua_CFunction f;
    TValue upvalue[1];
} CClosure;

typedef struct LClosure {
    ClosureHeader;
    struct Proto *p;  // 函数原型
    UpVal *upvals[1];  // upvalue数组
}LClosure;

typedef struct Proto {
    ...
    lu_byte numparams;  // 参数个数
    lu_byte is_vararg;  // 是不是变参函数
    lu_byte maxstacksize;  // 最大栈高度
}Proto;
```



##### tips：利用upvalue规避传参

```lua
-- common_data当作upvalue，可减少传参的压栈操作
local function fds(node, common_data)
    for _, sub in pairs(node) do
        dfs(sub, common_data)
    end
end
```



函数调用过程：

- 参数压栈
- 扩展栈
- 更新调用信息
- 初始化栈
- 执行
- 设置返回值



##### tips：避免无意义的函数调用



##### 调用信息CallInfo

```c
typedef struct CallInfo {
    ...
} CallInfo;

lua_state
CallInfo *ci;  // 当前执行函数的调用信息
CallInfo *base_ci;  // 调用信息的数组
```

调用堆栈

获得源文件名

`(const char *)(L->ci->func.value.gc->cl.l.p->source+1)`

获得行号

`L->ci->func.value.gc->cl.l.p->lineinfo[...]`



##### pcall

- setjmp/longjpm
- pcall会在调用前设置返回点
- 调用过程出现异常，则跳转到返回点
- 如果调用过程出现异常，则跳转到返回点



##### Metatable（元表）和Metamethod（元方法）

```c
typedef struct Table {
    CommonHeader;
    lu_byte flags;  // 快速判断有无对应的原方法
    lu_byte lsizenode;
    struct Table *metatable;  // 元表
    
}Table;
```

`__index`元方法

##### luaV_gettable

- 当从当前表中取某个key取不到时
- 检查这个表是否有metatable并且metatable有`__index`元方法
- 如果`__index`元方法时函数就直接调用，如果是其它类型，就对`__index`重复前面的过程

```lua
local default = {x = 3, y = 4}
local funcs = {}
local obj = {}
local get_value = function(tbl, key)
    return funcs[key]
end

function funcs::update_pos(x, y)
    self.x, self.y = self.x + x, self.y + y
end

setmetatable(obj, {__index = get_value})  -- 重载读表操作
setmetatable(funcs, {__index = default})  -- 重载读表操作

obj.update_pos(10, 20); 
-- 1. 首先在obj table中找update_pos, 找到则执行，没找到则检查obj是否有元表
-- 2. 有元表，且__index == get_value, 因此就调用get_value去找update_pos
-- 3. funcs[key], 在funcs中找update_pos方法，执行改方法
-- 4. update_pos中用到了self.x, self.y，而funcs中并没有定义，因此判断funcs是否有元表
-- 5. funcs中有元表则，且__index == default，就去default中找x和y
```



##### 面向对象

利用元表和元方法可以模拟面向对象，把类型和实例分开

```lua
C = {}  -- 定义类型
function C:method()  -- 定义方法
    ...
end

M = {__index = C} -- 定义元表

O = {}  -- 声明实例
setmetatable(O, M) -- 设置元表
O:method()  -- 实例调用类型方法
```

- 利用__index可以通过元表向上传递的特性，可以很容易地实现继承
- 子类可以覆盖掉父类的方法，由此实现多态
- 利用`__newindex`元方法可以实现数据访问的权限控制

代价：

- 避免层次比较深的继承体系，因为有多次查表，效率比较低，C++就查一次虚函数表
- 避免毫无意义的封装，因为lua没有内联来降低函数调用的开销，C++有内联

##### 内存分配

##### GC

标记清除算法：Tri-colour marking GC

##### 分代gc



![image-20210127103649450](C:\Users\kangzhongrun\AppData\Roaming\Typora\typora-user-images\image-20210127103649450.png)



```lua
-- 1
for i = 1, 10000 do
    local r = math.sin(i)
end

-- 2
local sin = math.sin
for i = 1, 10000 do
    local r = sin(i)
end
```



##### memorize缓存函数

- 参数简单，调用过程开销比较大的函数
- 可以cache参数和返回值，下次调用直接查表即可
- 可以通过一个通用的memorize函数搞定
- 注意下cache大小，省事可以设成weak

```lua
function memorize(f)
    local cache = {}
    return function(x)
        local r = mem[x]
        if not r then
            r = f(x)
            mem[x] = f
        end
        return r
    end
end
loadstring = memorize(loadstring)


```



##### 节省内存

Lazy loading

- 只加载运行需要的配置数据
- 配置表的大部分数据在一次运行期间是没用的
- 只加载运行需要的配置数据可以有效节省内存
- 降低gc的开销
- 额外但一次性的io

![image-20210127104339944](C:\Users\kangzhongrun\AppData\Roaming\Typora\typora-user-images\image-20210127104339944.png)





##### 



---

##### lua调用c中定义的函数

`gcc -o c_lua c_lua.c -llua -ldl -lm`

```c
#include <stdio.h>
#include <lua.h>
#include <string.h>
#include <lauxlib.h>
#include <lualib.h>

// 被lua调用的c注册的函数
static int add1(lua_State* ls)
{
	// int lua_isnumber (lua_State *L, int index);
	// 当给定索引的值是一个数字，或是一个可转换为数字的字符串时，返回 1 ，否则返回 0 。
	// 检查栈中的参数是否合法，1表示lua调用时第一个参数(从左到右)，依次类推
	if (!lua_isnumber(ls, 1))
	{
		luaL_error(ls, "incorrect arg 1 to func add1");
	}
	double op1 = lua_tonumber(ls, 1);
	if (!lua_isnumber(ls, 2)){
		luaL_error(ls, "incorrect arg 2 to func add1");
	}
	double op2 = lua_tonumber(ls, 2);
	// 将函数的结果压入栈中，如果有多个返回值，可以在这里多次压入栈中
	lua_pushnumber(ls, op1 + op2);
	// 返回值用于提示该c函数的返回值数量，即压入栈中的返回值数量
	return 1;
}

const char* testfunc = "print(add1(1.0, 2.0))";

int main()
{
	lua_State* ls = luaL_newstate();
	// 打开指定状态机中的所有 Lua 标准库。
	luaL_openlibs(ls);
	// 把 C 函数 f 设到全局变量 name 中	
	lua_register(ls, "add1", add1);
	// 加载并运行指定的字符串
	if (luaL_dostring(ls, testfunc))
		printf("Failed to invoke\n");
	lua_close(ls);
	return 0;
}
```



lua_State虚拟栈作为c和lua间数据交互的介质，c/c++程序中，如果要获得lua的值，只需要调用lua的c api函数，lua就会将指定的值压入栈，同样要将一个值传给lua时，需要先将该值压入栈，然后调用lua的c api函数，lua就会将其从栈中弹出





  **总之，一旦C中值被压入栈中，Lua就会生成相应的结构（实质就是Lua中实现的相应数据类型）并管理（比如自动垃圾回收）这个值，从此不会再依赖于原来的C值。**



##### `__index`：当访问一个表中不存在的键时，会从其元表的`__index`中查找。`__index`可以是一个table，也可以是一个function(table,key)

##### lua中使用setmetatable(table,metatable)设置元表



#### 2.__newindex

##### 该元表在赋值时使用。当对某个table的某个键赋值时，如果该键已经存在，则不会调用`__newindex`元表，否则会调用`__newindex`

```lua
local window = {}
window.__newindex = function (t, key, value)
	print("key===is", key, value)
	window[key] = value
end

local window1 = {}
window1.b = "value2"
print("window=====key", window.b, "window1===key", window1.b)

setmetatable(window1, window)

window1.a = "value22222"
print("window=====key", window.a, "window1===key", window1.a
```



#### rawset

##### 在项目中使用__newindex时，如果在__newindex中给原来的表赋值，则会出现循环调用__newindex最终导致爆栈的结果。因此，需要使用rawset方法。

```
-- Sets the real value of table[index] to value, without invoking the __newindex metamethod. table must be a table, index any value different from nil and NaN, and value any Lua value.
-- This function returns table.
```

##### 从注释可以看出，该方法可以避免调用__newindex，也就避免了递归调用元表的问题





##### 全局变量_G

在Lua脚本层，Lua将所有的全局变量保存在一个常规的table中，这个table被称为全局环境，并将这个table保存在一个全局变量_G中，也就是在脚本中可以用`_G`获取这个全局table，并且`_G._G == _G`，默认情况下，Lua在全局环境`_G`中添加了标准库比如math，pairs等函数

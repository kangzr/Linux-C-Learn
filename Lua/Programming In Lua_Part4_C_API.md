#### 前言

《Programming In Lua》Part IV The C API



#### 1. An Overview of the C API

Lua为嵌入式的和可扩展的语言，相应的C和Lua之间有两种交互方式，第一种在C中调用Lua的库，第二种Lua中调用C的库。应用代码和库代码同Lua交流使用相同的API，就是所谓的C-API，C_API是一组能和Lua进行交互的函数、常量和类型。Lua独立的解析器(lua.c)为应用代码的实现，标准库(lmathlih.c等)提供了库代码的实现，可以借鉴。



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

`laxulib.h`：申明了由`auxiliary library`提供的函数，其中所有函数都有前缀`luaL_`，比如`luaL_loadstring`。其不能访问Lua内部的代码，所有的工作都是基于`lua.h`库。

Lua库没有定义c全局变量，其所有的状态都保存在动态结构`lua_State`中，通过`luaL_newstate`可以创建一个新的Lua状态。`lualib.h`中定义了打开标准库的函数`luaL_openlibs`



##### The Stack

##### Pushing elements

##### Querying elements

##### Other stack operations

##### Error handling in application code

##### Erro handling with the C API

##### Error handling in library code

##### Memory Allocation

##### Exercises





#### 2. Extending Your Application



#### 3. Calling C from Lua



#### 4. Techniques for Writing C Functions



#### 5. User-Defined Types in C



#### 6. Managing Resources



#### 7. Threads and States


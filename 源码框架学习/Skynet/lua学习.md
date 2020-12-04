[Lua](https://moonbingbing.gitbooks.io/openresty-best-practices/content/lua/brief.html)

LuaJIT 就是一个为了再榨出一些速度的尝试，它利用即时编译（Just-in Time）技术把 Lua 代码编译成本地机器码后交由 CPU 直接执行

凭借着 FFI 特性，LuaJIT 2 在那些需要频繁地调用外部 C/C++ 代码的场景，也要比标准 Lua 解释器快很多。

LuaJIT 是采用 C 和汇编语言编写的 Lua 解释器与即时编译器


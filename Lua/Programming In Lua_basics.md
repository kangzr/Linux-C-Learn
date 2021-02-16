#### 前言

《Programming In Lua》Part II

最近看学习skynet框架，skynet中一般采用Lua来实现业务逻辑，也可用Lua来实现一些服务。因此准备学习一下Lua这门脚本语言，挑了《Programming in Lua》这本开始啃了起来，并且啃的英文原版。这里主要是将一些值得mark的知识点总结记录了下来，一来加深理解，而来便于后续温故知新。不得不说这本书真的很赞，入门看它就够了，遇到一些不懂的接口，[查接口文档即可](http://www.lua.org/manual/5.3/manual.html#lua_call)。



Lua作为一门简单(Simplicity)，可扩展(Extensibility)，可移植(Portability)，以及高效(Efficiency)的脚本语言，被应用在各个领域。(Lua是脚本语言中效率最高的)

Lua是一门动态语言：变量没有类型，变量的值才有类型。

Lua是一门解释性语言：生成字节码交由Lua虚拟机进行处理。（Python也是如此）



#### Lua的8种基本类型

- nil：任何全局变量在赋值前默认都是nil，我们可以通过将一个变量赋值为nil来删除它
- boolean：lua中只有false和nil为false，其他的都是true（包括0）
- number(包括整数和浮点数 0 == 0.0)
- string
- userdata（内存块）；其中userdata分为完全用户数据（由lua创建的内存并由lua进行gc）和轻量用户数据（由c/c++创建的内存，由c/c++释放）
- function
- thread
- table（唯一数据结构）

```lua
-- 计算阶乘
local function fact(n)
  if n < 1 then
    print("invalid n")
    return
  end
  
  if n == 0 then
    return 1
  else
    return n * fact(n - 1)
  end
end
```



#### 1. Numbers

包括整数和浮点数，Lua中0 == 0.0（true），1 == 1.0（true），即不断整数还是浮点数，只要值相等，就为true。如果我们需要区分整数和浮点数，可以使用`math.type`接口。

```lua
-- 整数运算会发生回绕(wraps around)
math.maxinteger + math.mininteger = -1
math.mininteger - 1 = math.maxinteger
math.maxinteger + 1 = math.mininteger
```



#### 2. Strings

Lua中字符串是不可变的(immutable)，我们不能改变字符串中的字符，一般会创建一个新的字符串来达到修改的目的，如下：

```lua
-- 将a中的one替换成another
a = "one string"
b = string.gsub(a, "one", "another")
print(a)	-- "one string"
print(b)	-- "another string"
```



##### string.gsub用法总结

`string.gsub(s, pattern, repl[,n])`

拷贝一份字符串s，并将s中出现的所有(或者n次，如果有)pattern替换成repl，返回两个参数，第一个为替换过后的字符串，第二个为替换字符串的次数。repl可为三种类型：string，table，function

- repl为string

  1. 为普通string，直接替换，如上`string.gsub(s, "one", "another")`

  2. %d形式，d可以为0-9，0代表整个匹配的字符串，否则代表第d个匹配的字符串

     ```lua
     x = string.gsub("hello world", "(%w+)", "%1 %1")
     -- x = "hello hello world world" (%w+)匹配单词，%1代表第一个匹配的单词，%1 %1 则输入两份，
     x = string.gsub("hello world from lua", "(%w+)%s*(%w+)", "%2 %1")
     -- x = "world hello lua from"  匹配两个单词，%2代表第二个单词，%1代表第一个，%2 %1 则表示把两个匹配到的单词反序
     ```

- repl为table

  会将匹配到的字符串作为key，根据key从table中取出value，再进行替换

  ```lua
  local t = {name="lua", version="5.3"}
  x = string.gsub("$name-$version.tar.gz", "%$(%w+)", t)
  -- x="lua-5.3.tar.gz" "%$(%w+)"匹配$开头的单词
  ```

- repl为function

  将匹配到字符串作为参数传递给function，并将function执行的返回结果进行替换

  ```lua
  x = string.gsub("home = $HOME, user = $USER", "%$(%w+)", os.getenv)
  -- x="home = /home/kang, /user/kang"
  ```



##### 长字符串表示

两种方式，第一种使用`[=[ ... ]=]`， 第二种使用`\n`进行换行，如下

```lua
-- 1
local s = [=[
<![CDATA[
	Hello world
]]>
]=]

-- 2
local s= "<![CDATA[\n		Hello world\n]]>"
```



##### string拼接(concatenation)

使用`..`进行拼接，拼接不会改变原有字符串（也不能改变，上述提到lua中string是不可变的）。

```lua
a = "hello"
b = a .. "world"
-- a = "hello" b = "hello world"
```

但是这种拼接方式是低效的，如果同时需要拼接多个字符串的话，因为每次拼接就意味着一次内存分配和字符串拷贝（其他脚本语言如python也同样有类似问题）。应当使用更为高效的字符串拼接方式`table.concat`，无论多少字符串，都只需要进行一次内存分配，每个带拼接的字符串也只需要拷贝一次。



##### string标准库

Lua的字符串库提供了丰富的函数，比如`string.len, string.rep, string.reverse, string.lower, string.upper`等，常用的字符串操作应该都可以满足，这里就不展开，在处理字符串的时候可以先看下Lib中是否已经实现了我们想要的功能，避免重复造轮子。



##### Exercise

```lua
-- 1. 编写函数，实现在某个字符串的指定位置插入另外一个字符串

-- 2. 编写函数，移除字符串中的指定部分（由起始位置和长度指定）

-- 3. 判断是不是回文字符串，需要忽略空格和标点符号
```



#### 3. Tables

Table是Lua中唯一的数据结构，但是Table非常灵活，基于Table可以实现其他编程语言中的所有数据结构，比如数组、集合、队列、栈等，甚至可以用Table来实现类。

- 除了nil，Table可以用任何类型作为key。

- 通过将Table中的key对应的value赋值为nil来删除Table中的key

- 可以通过下标`[]`或者`.`来访问Table中的元素，比如`a[x]`和`a.x`，注意这两个代表的含义不同，前者变量x为key，后者字符串"x"作为key。

- Table中第一个元素的下标从1开始

- Table的创建很灵活，主要有两种方式，一种是list-style, 一种是record-style，比如：

  ```lua
  -- 前两种方式更加高效，因为在创建Table时，就已经知道了大小，因此可以预分配数组空间，不需要进行扩容操作
  -- 1 list-style
  days = {"Sun", "Mon", "Tues", "Wed", "Thur", "Fri", "Sat"}
  -- print(days[4]) --> Wed
  
  -- 2 record-style
  a = {x = 10, y = 20}
  
  -- 3
  a = {}; a.x = 10; a.y = 20
  ```

- 使用Table实现列表时，我们把不包含nil的列表称为序列(sequence)，只有序列用`#`来获取Table的长度才是准确的。我们可以利用`#`来向序列末尾插入一个元素，`a[#a + 1] = v`;如果不是序列，我们可以通过遍历table的方式计算table长度。



##### Table的遍历

三种遍历方式如下：

```lua
-- 对于所有key-value形式，可使用pairs进行遍历, 这种遍历方式不保序
t = {10, print, x = 12, k = "hi"}
for k, v in pairs(t) do
  print(k, v)
end

-- 对于列表，可使用ipairs进行遍历，这种方式是保序的
t = {10, print, 12, "hi"}
for k, v in ipairs(t) do
  print(k, v)
end

-- 对于序列，可以借助#tab进行遍历，这种当然也保序
t = {10, print, 12, "hi"}
for k = 1, #t do
  print(k, t[k])
end
```



##### 如何安全高效的访问嵌套Table？

```lua
-- 1 成功拿到zipcode方法需要6次table的访问
zip = company and company.director and
				company.director.address and
					company.director.address.zipcode

-- 2 只需要三次table的访问
E = {}
zip = (((company or E).director or E).address or E).zipcode
```



##### TODO

深入学习Table，需要搞懂以下问题

- Table底层数据结构的实现（数组和哈希表），以及如何扩容，进而懂得如何使用table写出高效代码。

- 元表如何来使用，`__index`与`__newindex`，`rawget`与`rawset`

```lua
-- 元表 可以实现c++中的类，虚表；只有table和userdata对象有独自的元表
setmetable(tab, {
    __index = function (t, k)
      print("no exist")
      return
    end,
    __newindex = function (t, k, v)
      rawset(t, k, v)  -- 不触发元表
      print("set", k, v)
    end,
    __len = function(t)
      local i = 0
      for _, _ in pairs(i) do
        i = i + 1
      end
      return i
    end,
 })
local signal = tab["signal"]
  
```



##### Excercise

```lua
-- 1 下列代码输出什么？（可帮助我们理解t.a和t[a]问题）
sunday = "monday"
monday = "sunday"
t = {sunday = "monday", [sunday] = monday}
-- 翻译一下：t = {["sunday"] = "monday", ["monday"] = "sunday"}
print(t.sunday, t[sunday], t[t.sunday])
-- 输入：monday sunday sunday


-- 2 编写函数，测试table是否为有效序列

-- 3 table标准库提供了函数table.concat用于连接表中字符串；请实现自己的concat对比和table.concat的性能

```



#### 4. Functions

Lua中到处都是函数，甚至`main.lua`这种文件中的代码都可以看做是一个匿名函数。



##### 可变参数函数

可以使用三个点`...`来表示可变参数

```lua
-- 1 直接使用...来遍历参数
local function add (...)
  local s = 0
  for _, v in ipairs(...) do
    s = s + v
  end
  return s
end

-- 2 可以使用select来遍历可变参数
local function add (...)
  local s = 0
  for i = 1, select("#", ...) do
    s = s + select(i, ...)
  end
  return s
end

-- 3 可使用table.pack来把...变成一个新的table, 以下例子检测参数中是否有空值
local function nonils (...)
  local arg = table.pack(...)
  for i = 1, arg.n do
    if arg[i] == nil then return false end
  end
  return true
end

```



##### 尾调用

尾调用不会保存调用函数的信息在栈上，比如：

```lua
local function f (x)
  x = x + 1
  return g(x)  -- 进入g函数的调用，此时f函数的信息已经不在栈上
end

-- 基于这个特性，递归调用永远都不会爆栈
local function foo (n)
  if n > 0 then return foo(n - 1) end
end
```



##### Exercise

```lua
-- 1 编写函数，参数为数组，打印数组的所有元素
local function printAllEle (t)
  if type(t) ~= "table" then
    return
  end
  print(table.unpack(t))
end

-- 2. 编写函数，接收可变参数，返回除第一个元素之外其他所有值
local function returnWithoutFirst(first, ...)
  return ...
end

-- 3. 编写函数，接收可变参数，返回除最后元素外其他所有值
local function returnWithoutLast(...)
  local t = table.pack(...)
  table.remove(t)
  return table.unpack(t)
end
```



#### 5. The External World

主要介绍IO模块，以及其他一些外部库，了解`io.read`, `io.write`, `io.input`, `io.output`, `io.popen`,  `os.execute`等一些接口的用途。

io的默认输入为`stdin`，默认输出为`stdout`，通过使用`io.input`和`io.output`可以设置其输入输出方式。



```lua
io.read (···)
-- 等同于 io.input():read(···)，及如果通过io.input设置了输入文件，io.read读取该文件内容；io.write同

io.input ([file])
-- 如果传入file，会打开对应的文件，并将该文件句柄设置为默认输入文件（否则默认为标准输入stdin）;io.output同

io.popen(prog [, mode])
-- 在一个分开的进程中执行程序prog并发安徽一个文件句柄，可以通过这个句柄读取/写入数据。（ 此方法某些平台不能使用；）

os.execute([command])
-- 等同于ISO C函数system, command为shell可执行的指令，比如ls
```





##### Exercise

```lua
-- 1
-- 请编写一个程序，该程序读取一个文本文件然后将每行的内容按照字母表顺序排序后重写该文件。
-- 如果调用时不带参数，则从标准输入读取并向标准输出写入；
-- 如果调用时传入一个文件名作为参数，则从该文件中读取并向标准输出写入；
-- 如果调用时传入两个文件名作为参数，则从第一个文件读取并将结果写入第二个文件中
-- 使得当指定的输出文件已经存在时，要求用户确认


-- 2 使用函数os.execute和io.popen，分别编写用于创建目录、删除目录和输出目录内容的函数。
-- PS: os.execute 执行后返回错误码，而os.popen执行后返回file对象


-- 3. 你能否使用函数os.execute来改变Lua脚本的当前目录？为什么？

```



#### 6. Filling Some Gaps

这一部分主要介绍一些Lua基础部分的更多的细节，比如控制语句，while，for，repeat，until，continue，goto等，

Lua中所有的变量默认都是全局的，除非申明为local，因此非必要最好使用local关键字，避免全局名字冲突，并且访问局部变量也比访问全局变量更快(可通过将一个全局变量赋值给一个局部变量来加快其访问速度)，且局部变量离开其作用域后可被gc.

`for`语句中的变量默认是局部的，只在for循环中有效。



##### Exercise

```lua
-- 1. 无条件循环的四种实现方式

-- 2. 使用goto语句和尾调用两种方式实现maze game
```




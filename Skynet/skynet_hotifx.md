线程间通信，使用管道；

比如work线程和socket线程间的通信

网络编程：单线程读，多线程写

skynet哪些地方需要向epoll进行注册？123 `sp_add就需要绑定通过void*ud`

- connectfd
- clientfd worker
- listenfd clientfd=accept()
- pipe读端，worker线程往管道写端写数据，socket线程在管道读端读数据

main.lua就是一个actor

客户端连接skynet：`new_fd`

listenfd与actor进行绑定



Redis epoll中通过event.fd进行绑定





skynet 业务是由lua来开发，与底层沟通以及计算密集的都需要用c

提升：lua调用c，c怎么调用lua，才能看懂所有的skynet代码；

目的：openresty（nginx+lua，将lua嵌入到nginx中，是的开发更加简单），nginx开发（filter，handler，upstream）



lua

- 动态语言

  变量没有类型，值才有类型

- 解释性语言

  编译性语言：c/c++/go 生成与当前硬件相关的汇编代码

  解释性语言：生成字节码，交由虚拟机处理（lua，python，java），不需要考虑硬件环境，自动释放内存

- lua的类型

  nil，boolean，number（整数和浮点数 0 == 0.0）， string（跟std::string类似），

  function（可以不写名字，支持外包），

  userdata（内存块，用来粘合c/c++）

  	1. 完全用户数据，由lua创建内存，由lua gc
   	2. 轻量用户数据，由c/c++创建的内存，由c/c++自己释放

  thread，coroutine，

  table：lua中唯一的数据结构

```lua
-- lua中数组从1开始
local tab = {}
tab[#tab+1] = 1
tab[#tab+2] = 2
tab[#tab+3] = 3
table.insert(tab, 4)
for i=1, #tab do
  print(tab[i])
end

-- 迭代器函数
for _, v in ipairs(tab) do
  print(v)
end

-- 可以用作hash表
tab["hello"] = "world"
for k, v in pairs(tab) do
  print(k, v)
end

tab["dofunc"] = function()
  print("do function")
end

-- 
local tab = {
  init = function ()
    print("call init")
  end,
  create = function ()
    print("call create")
  end,
}
local cmd = "init"
tab[cmd]()

-- 元表 可以实现c++中的类，虚表
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

lua 5.3中文文档














#### 前言

《Programming In Lua》Part II

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







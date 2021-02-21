#### Ch5 Numbers

Tables are the main (in fact, the only) data structuring mechanism in Lua, and a powerful one.

For Lua, this expression means "index the table math using the string"sin" as the key"

A table in lua is essentially an associative array. A table is an array that accepts not only numbers as indices, but also strings or any other value of the language(except nil).

we can assign nil to a table field to delete it.



Due to the way that Lua implements tables, the order that elements appear in a traversal is undefined.

For lists, we can use the ipairs iterator:

```lua
t = {10, print, 12, "hi"}
for k, v in ipairs(t) do
    print(k, v)
end
```

In this case, Lua trivially ensures the order.

##### Safe Navigation

```lua
-- inefficient, it performs six table accesses in a successful access.
zip = company and company.director and 
		company.director.address and
			company.drector.address.zipcode

-- minimum required number of table accesses(three)
E = {}
zip = (((company or E).director or E).address or E).zipcode
```

#### Function

Lua offers the function table.pack to receives any number of arguments and returns a new table with all its arguments (just like {...}), but this table has also an extra field "n", with the total number of argumens.







#### Ch20 Metatables and Metamethods

the access trigger the interpreter to look for an `__index` metamethod：if there is no such method, as usually happens, then the access results in nil; otherwise, the metamethod will provide the result



When we want to access a table without invoking its `__index` metamethod, we use the function `rawget`. The call rawget(t, i) dose a raw assess to table t, that is, a primitive access without considering metatables. Doing a raw access will not speed up our code, but sometimes we need it





There is a raw function that allows us to bypass the metamethod: the call rawset(t, k, v) dose the equiivalent to t[k] = v without invoking any metamethod



The combined use of the `__index` and `__newindex` metamethods allows serveral powerful constructs in Lua,





---



### Table of Contents

#### 1. The Basics

- Numbers
- Strings
- Tables
- Functions
- The External World
- Filling Some Gaps

#### 2. The Real Programming

- Closures
- Pattern Matching
- Interclude: Most Frequent Words
- Bits and Bytes
- Data Structures
- Data Files and Serialization
- Compilation, Execution, and Errors
- Modules and Packages

#### 3. Lua-isms(主义)

- Iterators and The Generic for
- Interlude: Markov Chain Algorithm
- Metatables and Metamethods
- Object-Oriented Programming
- The Environment
- Garbage
- Conroutines
- **Relection**
- Interlude Multithreading with Coroutines

#### 4. The C API

- An Overview of the C API
- Extending Your Application
- Calling C from Lua
- Techniques for Writing C Functions
- User-Defined Types in C
- Managing Resoures
- Threads and States













---



```lua
local default = { x=3, y=4 }
local funcs = {}
local obj = {}
local get_value = function(tbl,key)
    return funcs[key]
end
function funcs:update_pos(x,y)
    self.x,self.y = self.x + x, self.y + y
end
setmetable(obj, {__index = get_value})
setmetable(funcs, {__index = default})
```
















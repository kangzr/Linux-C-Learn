##### PROJECT

定义工程名称，并可指定工程支持的语言（可忽略，默认支持所有语言）；这个指令隐士定义了两个cmake变量：`<projectname>_BINARY_DIR`以及`<projectname>_SOURCE_DIR`

```cmake
PROJECT(projectname)
```

##### SET

可用来显示的定义变量，

```cmake
SET(SRC_LIST main.c t1.c t2.c)
```

##### MESSAGE

用于向终端输出用户定义的信息，包括三种类型：1. SEND_ERROR，产生错误，生成过程被跳过；2. STATUS，输出前缀为1的信息；3. FATAL_ERROR：立即终止所有cmake过程；

```cmake
MESSAGE(STATUS "This is BINARY dir " ${HELLO_BINARY_DIR})
```

##### ADD_EXECUTABLE

生成一个文件名为`hello`的可执行文件。`${}`用来引用变量

```cmake
# 原型
ADD_EXCUTABLE(hello ${SRC_LIST})

# 例如
ADD_EXCUTABLE(hello main.c)
```

##### ADD_SUBDIRECTORY

向当前工程添加存放源文件的子目录，EXCLUDE_FROM_ALL参数是将这个目录从编译过程中排除；

```cmake
ADD_SUBDIRECTORY(source_dir)
```

##### ADD_LIBRARY

```cmake
ADD_LIBRARY(libname [SHARED|STATIC|MODULE [EXCLUDE_FROM_ALL] source1 source2 ... sourceN])
# SHARED 动态库 STATIC 静态库 MODULE 再使用dyld的系统有效
```

##### SET_TARGET_PROPERTIES

用来设置输出的名称，生成名字相同的静态库和动态库，还可用于控制版本号

```cmake
SET_TARGET_PROPERTIES(target1 target2 ...
PROPERTIES prop1 value1
prop2 value2...)

# 例如
SET_TARGET_PROPERTIES(hello_static PROPERTIES OUTPUT_NAME "hello")
GET_TARGET_PROPERTY(OUTPUT_VALUE hello_static OUTPUT_NAME)
```

##### INCLUDE_DIRECTORIES

用来向工程添加多个特定的头文件搜索路径，路径之间用空格分割，如果路径中包含空格，可用双引号将它括起来，

```cmake
INCLUDE_DIRECTORIES([AFTER|BEFORE] [SYSTEM] dir1 dir2 ...)
```

##### LINK_DIRECTORIES

添加非标准的共享库搜索路径

##### TARGET_LINK_LIBRARIES

用来为target添加需要链接的共享库

```cmake
TARGET_LINK_LIBRARIES(target library1 library2)
```

##### FIND_

```cmake
# VAR变量表示找到的库全路径，包括文件名
FIND_LIBRARY(<VAR> name1 path1 path2 ...)
# VAR变量代表找到的文件全路径，包含文件名
FIND_FILE(<VAR> name1 path1 path2 ...)
# VAR变量代表包含这个文件的路径
FIND_PATH(<VAR> name1 path1 path2 ...)
# VAR变量代表包含这个程序的全路径
FIND_PROGRAM(<VAR> name1 path1 path2)

```




















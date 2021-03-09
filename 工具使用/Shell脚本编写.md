> Shell编程相关总结

##### sh与bash

`/bin/sh`与`/bin/bash`大体没区别，但仍存在不同标准。如果申明为`/bin/sh`，则应使用任何`POSIX`没有规定的特性（如`let`等命令，但`/bin/bash`可以）。通过`man sh`查看，会发现，有的系统(如ubuntu)其`sh`指向`dash`，而有的系统(如debian)其`sh`指向`bash`。

```shell
# 表明用/bin/sh来执行此脚本
#!/bin/sh

# 表明使用/bin/bash来执行此脚本
#!/bin/bash
```



##### 访问变量(Accessing Values)

Shell中通过前缀`$`访问变量的值



##### 变量类型

```shell
1.使用变量
echo ${var}  $var  # 最好加{}

2.只读变量
readonly var

3.删除变量
unset $var  # 无法删除只读变量

4.字符串
定义 str="run" str='run' str=run
拼接 str="hello, "$name" !"  str="hello, ${name} !"
获取字符串长度 echo ${#str}
提取子字符串 echo ${str:1:4}

5.Shell数组
定义 (空格隔开)
array=(var1 var2 ... varn)

array[0]=var1
array[1]=var2
array[n]varn
读取数组
varn=${array[n]}
${array[@]}  # 获取数组所有元素
# 获取数组的长度
${#array[*]}
${#array[#]}

6.注释
单行注释 ： #
多行注释 ：:<<!  !(!可以为任意字符)
```





##### 特殊变量

| Variable |                         Description                          |
| :------: | :----------------------------------------------------------: |
|    $0    |              The filename of the current script              |
|    $n    |                 the position of an argument                  |
|    $#    |        The number of arguments supplied to a script.         |
|    $*    |             All the arguments are double quoted.             |
|    $@    |       All the arguments are individually double quoted       |
|    $?    | The exit status of the last command executed or function return. |
|    $$    |                             PID                              |
|    $!    |      The process number of the last background command.      |
|  $HOME   |                        /home/username                        |



```shell
$(cd `dirname $0`;pwd)

# dirname $0, 取当前执行的脚本文件的父目录

cd `dirname $0`
# 此命令只在脚本中有意义，返回这个脚本文件放置的目录，根据这个目录来定位所要运行程序的相对位置

# pwd, 显示当前目录

source filename
# 读取脚本里面的语句依次在当前shell里执行，脚本里所新建、改变变量的语句都会保存在当前shell里面

eval cmdLine
# 对cmdLine进行扫描，第一遍扫描后，如果cmdLine是个普通命令，则执行此命令，如果cmdLine中含有变量的间接引用，则保证间接引用的意义



```



##### 数组

```shell
#!/bin/sh

NAME[0] = "Hello"
NAME[1] = "World"
echo "First Index: ${NAME[0]}"
echo "Second Index: ${NAME[1]}"

echo "First Method : ${NAME[*]}"
echo "Second Method : ${NAME[@]}"
```



##### 操作符(Operators)

| Operator |                         Description                          |
| :------: | :----------------------------------------------------------: |
|    -o    |                              OR                              |
|    -a    |                             AND                              |
|    !     |                             NOT                              |
|   -eq    |                           equal(=)                           |
|   -ne    |                        not equal(!=)                         |
|   -gt    |                       greater than(>)                        |
|   -lt    |                         less than(<)                         |
|   -ge    |                  greater than or equal(>=)                   |
|   -le    |                    less than or equal(<=)                    |
|    -n    |         检测字符串长度是否不为 0，不为 0 返回 true。         |
|    -z    |            检测字符串长度是否为0，为0返回 true。             |
|    -e    |     检测文件（包括目录）是否存在，如果是，则返回 true。      |
|    -d    |          检测文件是否是目录，如果是，则返回 true。           |
|    -f    | 检测文件是否是普通文件（既不是目录，也不是设备文件），如果是，则返回 true。 |
|    -L    |                         link symbol                          |
|    -r    |           检测文件是否可读，如果是，则返回 true。            |
|    -w    |           检测文件是否可写，如果是，则返回 true。            |
|    -x    |          检测文件是否可执行，如果是，则返回 true。           |
|   -nt    |                    file1 newer than file2                    |
|   -ot    |                    file1 older than file2                    |

```shell
echo -n "Something"
# -n option lets echo avoid printing a new line character（不空行）
echo -e "Something"
# -e enables the interpretation of escape '\n'（echo空行，默认为-e）
```



##### 匹配模式

```shell
${var%pattern}  # 右边
${var%%pattern} # 右边
${var#pattern} # 左边
${var##pattern} # 左边

${var%pattern, var%%pattern} # 右边开始匹配
${var#pattern, var##pattern} # 左边开始匹配
${%, #} # 最短匹配
${%%, ##} # 最长匹配
```



##### 函数

```shell
#!/bin/bash

# 格式
[ function ] funcname [()]
{
    action;
    [return int;]
}

# 函数声明
foo () {
    echo "Hello World $1 $2"
}

# 调用 参数可有可无
foo arg1 ...


# ex:
#!/bin/sh

# 1. 函数定义 无参数
Hello () {
   echo "Hello World"
}
Hello # 调用

#2 函数定义有两个参数
#!/bin/sh
Hello () {
   echo "Hello World $1 $2"
}
Hello Zara Ali # 带参数函数调用

#3 函数带参数和返回值
#!/bin/sh
Hello () {
   echo "Hello World $1 $2"
   return 10
}
Hello Zara Ali  # 调用

# Capture value returnd by last command
ret=$?
echo "Return value is $ret"
```



##### 参数传递

```shell
# 执行Shell脚本，向脚本传递参数，就获取参数格式: $n n代表第几个参数
# $0执行文件名

# $* 与 $@ 区别：
# 相同点：都是引用所有参数。
# 不同点：只有在双引号中体现出来。
# 假设在脚本运行时写了三个参数 1、2、3，，则 " * " 等价于 "1 2 3"（传递了一个参数），而 "@" 等价于 "1" "2" "3"（传递了三个参数）。
```





##### 流程控制

```shell
1. if else
if condition
then
    commands
fi

# 终端命令提示符：if [ $(ps -ef | grep -c "ssh") -gt 1 ]; then echo "true"; fi

if condition1
then
    command
elif condition2
then
    command
else
    command
fi

2. for 循环
for var in item1 item2 ... itemN
do
    commands
done

for var in item1 item2 ... itemN; do command1; command2… done;

3. while 语句
while condition
do 
    command
done


if [[ $NAME -ne "hello" ]]; then
	echo "come here"
else
	echo "here"
fi

```



##### getopts

 可以获取用户在命令下的参数，然后根据参数进行不同的提示或者不同的执行

```shell
while getopts "ocwnrm" arg; do
	case $arg in
		o)
			...
			;;
	    c)
	    	...
	    	;;
	    ...
	    ?)
	    	echo "unknown arg"
	    	exit -1
	    	;;
	 esca
done
```



##### Example

cp文件脚本

```shell
# 复制文件 cp
if [ "$#" -ne 2]
then
    echo "Usage: mycp from to"
    exit 1
fi
from="$1"
to="$2"
if [ -e "$to" ]
then
    echo "$to already exist; overwrite (yes/no)?"
    read answer
    if [ "$answer" != yes ]
    then
        echo "Copy not performed"
        exit 0
    fi
fi
cp $from $to
```












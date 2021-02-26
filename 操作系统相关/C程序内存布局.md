![memory_layout_of_c](..\pic\memory_layout_of_c.png)

从低地址到高地址依次为：代码段，初始化数据段，未初始化数据段，堆，栈，内核空间

##### 代码段（Text）

代码段存放编译好程序的机器码，代码段一般是共享的，因此只有一份拷贝可以被频繁的执行多次，比如文本编辑器，C编译器，shell终端等，代码段为只读段。

##### 初始化数据段（initialized data）

存放所有申明为`global`, `static`, `constant`,`extern`，且被初始化过的变量，这部分可分为细分为只读区域和可读可写区域。

##### 未初始化数据段（uninitialized data BSS）

BSS段存放`global`和`static`变量，且没有被显示初始化或者初始化为0的变量。

##### 堆（Heap）

堆从低地址向高地址生长，用于动态内存分配，使用malloc，realloc等函数在堆上分配内存，堆由所有共享库和动态加载的模块共享。

##### 栈（stack）

栈从高地址向低地址生长，存放局部变量，每次函数调用时，其返回的地址，以及调用者的上下文环境（各种寄存器）也存在stack。

如果栈指针和堆指针相遇时内存就耗尽了（exhausted）。

```c
#include <stdio.h>
char c[] = "hello world";   /*global变量，存于初始化数据段，可读可写区域*/
const char s[] = "how are you";  /*const变量，存于初始化数据段，只读区域*/
char h;  /*未初始化变量，存于BSS段*/
int main() {
    static j;  /*未初始化静态变量，存于bss段*/ 
    static int i = 11;  /*static变量，可读可写区域*/
    char *p = (char*)malloc(sizeof(*p));  /*malloc 堆上分配内存*/
    return 0;
}
```


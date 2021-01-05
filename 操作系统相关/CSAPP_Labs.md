[官网](http://csapp.cs.cmu.edu/3e/labs.html)

[参考code](https://github.com/Exely/CSAPP-Labs)

#### 1. Data Lab

理解位运算，补码，浮点数

i) 位运算

```c
/*
 * bitXor - x^y using only ~ and &
 * Example: bitXor(4, 5) = 1
 * Legal ops: ~ &
 * Max ops: 14
 * Rating: 1
 */
/*德摩根定律 对偶律 用非运算~ 和 或运算| 实现和运算&*/
int bitAnd(int x, int y) {
    return ~(~x | ~y);
}


/* 
 * getByte - Extract byte n from word x
 *   Bytes numbered from 0 (LSB) to 3 (MSB)
 *   Examples: getByte(0x12345678,1) = 0x56
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 6
 *   Rating: 2
 */
/*
 * 从x中取出第n个字节(i=0,1,2,3), 如0x12345678 i=0对应0x78 i=3对应0x12，将对应字节右移到最低位，然后用mask提取
 * n << 3 表示 n * 8， 一个字节为8位，因此要取第k个字节就右移k*8位即可
 */
int getByte(int x, int n){
    int mas=0xff;
    return (x>>(n<<3))&mask;
}


/* 
 * logicalShift - shift x to the right by n, using a logical shift
 *   Can assume that 0 <= n <= 31
 *   Examples: logicalShift(0x87654321,4) = 0x08765432
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 3 
 */
/*逻辑右移：c默认算术右移，构造一个高位扩展位为0，其它位全为1的情况*/
int logicalShift(int x, int n) {
    int mask=((0x1<<(32+~n))+~0)|(0x1<<(32+~n));
    return (x>>n)&mask;
}

/* 
 * bang - Compute !x without using !
 *   Examples: bang(3) = 0, bang(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
/* 对 0 运算就得到 1,对非 0 就得到 0；也就是如果 x 的位中含有 1 就返回 0 ,
 * 这里运用移位后取或将 x 中的位一步步「折叠」 到了第一位上，然后判断第一位就可以了，这种「折叠」的方法很有趣
 */
int bang(int x) {
  x=(x>>16)|x;
  x=(x>>8)|x;
  x=(x>>4)|x;
  x=(x>>2)|x;
  x=(x>>1)|x;
  return ~x&0x1;
}
```

ii) 补码

```c
/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
    return ~x+1;
}


/* 
 * isPositive - return 1 if x > 0, return 0 otherwise 
 *   Example: isPositive(-1) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 8
 *   Rating: 3
 */
int isPositive(int x) {
    return !(!(x))&!((x>>31)&(0x1));
}
```



#### 2. Bomb Lab

理解汇编，学习gdb调试

![os_register](..\pic\os_register.png)

[参考这里](https://earthaa.github.io/2020/01/12/CSAPP-Bomblab/)

#### 3. Attack Lab

第2，3个需要熟悉汇编

#### 4. Cache Lab
















记录一下骚气的位运算

```c
// 1. ALIGNMENT 4  取size的下一个4的倍数
int align_4(int size){
    size = (size + 3) & ~3;
    return size;
}

// 2. 判断是否是2的幂
int is_pow_of_2(uint32_t x) {
    // 因为2^n 都有一个特点就是最高位时1，其它位都是0，且只有2^n才会满足这种情况
    //  2^n - 1 则首位为0，其它位均位1，因此(2^n) & (2^n - 1) == 0
    // return True for x == 0
    return !(x & (x-1));
    // return !x && !(x & (x - 1))
}
// 3. 写一个2的幂
uint32_t next_pow_of_2(uint32_t x) {
    if ( is_pow_of_2(x) )
        return x;
    // 找下一个1000
    x |= x>>1;
    x |= x>>2;
    x |= x>>4;
    x |= x>>8;
    x |= x>>16;
    return x + 1;
}
```


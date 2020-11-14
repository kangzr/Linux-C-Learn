### KMP算法

用于处理在一个字符串中寻找子串(pattern)，首先要根据pattern构建next数组；

时间复杂度：O(texlen + ptnlen)

回溯：取决于pattern公共前缀与后缀有多少（aba --> a/ab  a/ba ---> 1）

```c
void make_next(const char *pattern, int *next){
  int q, k, m = strlen(pattern);
  for(q = 1, k = 0; q < m; q++){
    while(k > 0 && pattern[q] != pattern[k])
      k = next[k - 1];  // 记录回溯位置
     
    if (pattern[q] == pattern[k]) k++;

    next[q] = k;
  }
}

int kmp(const char *text, const char *pattern, int *next){
  int n = strlen(text), m = strlen(pattern);
  make_next(pattern, next);
  int i, q;
  for(i = 0, q = 0; i < n; i++){
    while(q > 0 && pattern[q] != text[i]){
      q = next[q - 1];   // 不相等则回溯
    }
    if(pattern[q] == text[i]) q++;  // 相等
    if(q == m){  // 匹配成功
      break;
    }
  }
  return i - q + 1;
}
```


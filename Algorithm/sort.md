### 一. 排序

#### 1. 冒泡排序

**基本思想**：从后往前比较，把较小者往前移，像气泡一样冒上来

**性质**：属于内排序，稳定排序

**时间复杂度**：平均-O(n^2)；最好-O(n)；最坏O(n^2)；

**空间复杂度**：O(1)

```c
void bubble_sort(int *data, int length){
  int i = 0, j = 0, flag = 1;
  for (i = 0; i < length - 2 && flag; i++){
    flag = 0;
    for (j = length - 2; j >= 0; j--){
      if (data[j+1] < data[j]){
        swap(data, j, j+1);
        flag = 1;
      }
    }
  }
}
```

#### 2. 简单选择排序

**基本思想**：每次从索引i开始遍历，获取最小值的索引值，并跟i进行交换

**性质**：属于内排序，不稳定排序

**时间复杂度**：平均/最好/最坏都是O(n^2)

**空间复杂度**：O(1)

```c
void select_sort(int *data, int length){
  int i = 0, j = 0, min = 0;
  for(i = 0; i < length - 1; i++){
    min = i;
    for(j = i + 1; j < length; j++){
      if(data[j] < data[min]){
        min = j;
      }
    }
    if (min != i){
      swap(data, min, i);
    }
  }
}
```

#### 3. 直接插入排序

**基本思想**：将一个数插入一个有序数组的合适位置，得到一个新的有序数组

**性质**：内排序，稳定

**时间复杂度**：平均-O(n^2), 最好-O(n), 最坏-O(n^2)

**空间复杂度**：O(1)

```c
void insert_sort(int *data, int length){
  int i = 0, j = 0, temp - 0;
  for(i = 1; i < length; i++){
    if(data[i] < data[i-1]){
      temp = data[i];
      for(j = i - 1; data[j] > temp; j--){
        data[j+1] = data[j];
      }
      data[j+1] = temp;
    }
  }
}
```



#### 4. 希尔排序

**基本思想**：通过增量方法构建一个基本有序序列，逐渐减少增量直至为0；(直接插入排序升级版)

**性质**：属于插入类排序；属于内排序；属于不稳定排序

**时间复杂度**：平均-O(n^1.3) , 最好O(n), 最坏O(n^2),

**空间复杂度**:  O(1)

```c
void shell_sort(int *data, int length){
  int gap = 0, i = 0, j = 0, temp = 0;
  for(gap = length / 2; gap > 0; gap /= 2){
    for(i = gap; i < length; i++){
      temp = data[i];
      for(j = i - gap; j >= 0 && data[j] > temp; j-=gap){
        data[j+gap] = data[j];
      }
      data[j+gap] = temp;  // 把temp插入至以gap为增量的有序序列中的合适位置
    }
  }
}
```

#### 5. 堆排序

（实现时间轮定时器，一个定时器，实现若干个定时任务）

```c
void heap_adjust(int *data, int s, int m){
 int j, temp = data[s];
  for(j = s*2; j <= m; j*=2){
    if (j < m && data[j] < data[j+1])
      ++j;
    if(temp >= data[j])
      break;
    data[s] = data[j];
    s = j;
  }
  data[s] = temp;
}
  
void heap_sort(int *data, int length){
  int i = 0;
  for(i = (len - 1) / 2; i>= 0; i--)
    heap_adjust(data, i, length - 1);
  for (i = len - 1; i > 0; i--){
    swap(data, 0, i);
    heap_adjust(data, 0, i-1);
  }
}
```



#### 6. 归并排序

**基本思想**：分治法，先把数组分成两个或者一个元素，排序后以此合并

**性质**：属于外排序(需要借助外部空间)；稳定排序

**时间复杂度**：平均/最好/最坏都是O(nlogn);

**空间复杂度**：O(n)；需要借助n大小的temp数组;

```c
//合 其中 [start, mid] 和 [mid + 1, end]分别为有序的
void merge_conquer(int *data, int *temp, int start, int mid, int end){
  int i = start, j = mid + 1, k = start;
  while (i <= mid && j <= end){
    if(data[i] < data[j]){
      temp[k++] = data[i++];
    }else{
      temp[k++] = data[j++];
    }
  }
  while (i <= mid){
    temp[k++] = data[i++];
  }
  while (j <= end){
    temp[k++] = data[j++];
  }
  memcpy(data + left, temp + left, sizeof(int) * (end - start + 1));
}

// 分
void merge_divide(int *data, int *temp, int start, int end){
  int mid;
  if(start < end){
    mid = start + (end - start) / 2;
    merge_divide(data, temp, start, mid);
    merge_divide(data, temp, mid + 1, end);
    merge_conquer(data, temp, start, mid, end);
  }
}

void merge_sort(int *data, int length){
  int *temp = (int *)malloc(sizeof(int) * length);
  merge_divide(data, temp, 0, length - 1);
  free(temp);
}
```

#### 7. 快速排序

**基本思想**：选取一个关键字pivot，通过一趟排序把数组分成两部分，左侧部分小于pivot，右侧部分大于pivot；再分别对左右两部分进行排序；

**性质**：属于交换类排序(冒泡排序升级)，内排序(不需要额外分配内存)，不稳定

**时间复杂度**：平均/最好 - O(nlogn)；最坏 - O(n^2), 当pivot为最值时;

**空间复杂度**：O(logn), 递归栈深度

```c
void swap(int *data, int i, int j){
  data[i] = data[i] + data[j] - (data[j] = data[i]);
}
void quick_sort_core(int *data, int start, int end){
  if(start >= end){
    return;
  }
  int low = start, high = end;
  int mid = start + (end - start) / 2;
  if (data[low] > data[high]){
    swap(data, low, high);
  }
  if (data[mid] > data[high]){
    swap(data, mid, high);
  }
  if (data[mid] > data[low]){
    swap(data, mid, low);
  }
  int pivot = data[low];

  while(low < high){
    while(low < high && data[high] >= pivot){
      high--;
    }
    data[low] = data[high];
    while(low < high && data[low] <= pivot){
      low++;
    }
    data[high] = data[low];
  }
  data[low] = pivot;
  quick_sort_core(data, start, low - 1);
  quick_sort_core(data, low + 1, end);
}
void quick_sort(int *data, int length){
  quick_sort_core(data, 0, length - 1);
}
```



1. 桶排序
2. 计数排序
3. 基数排序



### 二. KMP算法

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



### 三. 栈和队列

**栈的实现**

```c
#define ALLOC_SIZE 512
typedef int KEY_TYPE;

typdef struct _stack {
  KEY_TYPE *base;
  int top;
  int stack_size;
}stack;

void init_stack(stack *s){
  s->base = (KEY_TYPE*)calloc(ALLOC_SIZE, sizeof(KEY_TYPE));
  assert(s->base);
  s->top = 0;
  s->stack_size = ALLOC_SIZE;
}

void destroy_stack(stack *s){
  assert(s);
  free(s->base);
  s->base = NULL;
  s->stack_size = 0;
  s->top = 0;
}

void push_stack(stack *s, KEY_TYPE data){
  assert(s);
  if (s->top >= s->stack_size){
    s->base = realloc(s->base, (s->stack_size + ALLOC_SIZE) * sizeof(KEY_TYPE));
    assert(s->base);
    s->stack_size += ALLOC_SIZE;
  }
  s->base[s->top] = data;
  s->top++;
}

void pop_stack(stack *s, KEY_TYPE *data){
  assert(s);
  *data = s->base[--s->top];
}

int empty_stack(stack *s){
  return s->top == 0 ? 0 : 1;
}

int size_stack(stack *s){
  return s->top;
}
```

**队列的实现**

```c
#define ALLOC_SIZE 512

typedef int KEY_TYPE;

typedef struct _queue_node {
  struct _queue_node *next;
  KEY_TYPE = key;
} queue_node;

typedef struct _queue {
  queue_node *front;
  queue_node *rear;
  int queue_size;
}queue;

void init_queue(queue *q){
  q->front = q->rear = NULL;
  q->queue_size = 0;
}

void destroy_queue(queue *q){
  queue_node *iter;
  queue_node *next;
  iter = q->front;
  while(iter){
    next = iter->next;
    free(iter);
    iter = next;
  }
}

void push_queue(queue *q, KEY_TYPE key){
  assert(q);
  queue_node *node = (queue_node*)calloc(1, sizeof(queue_node));
  assert(node);
  node->key = key;
  node->next = NULL;
  if(q->rear) {
    q->rear->next = node;
    q->rear = node;
  }else{
    q->front = q->rear = node;
  }
  q->queue_size++;
}

void pop_queue(queue *q, KEY_TYPE *key) {
  assert(q);
  assert(q->front != NULL);
  if(q->front == q->rear){
    *key = q->front->key;
    free(q->front);
    q->front = q->rear = NULL;
  }else{
    queue_node *node = q->front->next;
    *key = q->front->key;
    free(q->front);
    q->front = node
  }
  q->queue_size--;
}

int empty_queue(queue *q){
  assert(q);
  return q->rear == NULL ? 0 : 1;
}

int size_queue(queue *q){
  assert(q);
  return q->queue_size;
}
```

samba/mac虚拟机


















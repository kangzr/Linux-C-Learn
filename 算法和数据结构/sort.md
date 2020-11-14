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



#### 8. 计数排序

**算法思想**：入是有一个小范围内的整数构成的（比如年龄等），利用额外的数组去记录元素应该排列的位置（局限性比较大，数组范围不能太大，且只能为整数）

计数排序是典型的不是基于比较的排序算法，基于比较的排序算法最少也要O(nlogn)，有没有可能创造线性时间的排序算法呢？那就是不急于比较的排序算法

使用额外数组q，q的大小为待排序数组中最大元素+1，该数组的含义是：每个位置的值为该位置索引对应的元素的个数（额外数组索引值==待排序数组元素值），根据数组q来将元素排到正确位置。

算法步骤：

- 找出待排序数组中最大和最小元素
- 统计数组中每个元素出现的次数，存入额外数组q中（索引值等于元素位置）
- 对所有的计数累加，从额外数组的第一个元素开始，以此往后累加
- 反向填充目标数组，将每个元素i放在新数组的q[i]项，并q[i]--

**时间复杂度**：O(n)

**空间复杂度**：额外空间O(n)    (O(n + k)，n为原数组长度，k为数据范围)

**性质**：稳定排序，外排序

```c
void count_sort_core(int *data, int len, int max) {
    int *s = (int *)malloc(sizeof(int) * len);
    int *cnts = (int *)malloc(sizeof(int) * max);
    memset(cnts, 0, sizeof(int) * max);
    
    int i;
    for (i = 0; i < max; i ++)
        cnts[data[i]]++;
    
    for (i = 1; i < max; i ++)
        cnts[i] = cnts[i] + cnts[i - 1];
    
    for (i = 0; i < len; i ++) {
        s[cnts[data[i]] - 1] = data[i];
        cnts[data[i]] --;
    }
    
    memcpy(data, s, sizeof(int) * len);
    free(s);
    free(cnts);
}

void count_sort(int *data, int len) {
    if (!data || len < 1) return;
    int max = data[0];
    int i;
    for (i = 1; i < len; i ++){
        if (data[i] > max)
            max = data[i];
    }
    count_sort_core(data, len , max + 1);
}
```



#### 9. 桶排序

算法步骤；

- 找到数组最大值和最小值，然后根据桶数量，计算每个桶中的数据范围

- 遍历原始数据，找到该数据对应的桶序列，然后将数据放入对应的桶中

- 当向同一个序列的桶中第二次插入数据时，判断桶中已存在的数字与新插入的数字的大小，按从左到右，从小打大的顺序插入。一般通过链表来存放桶中数据。

- 全部数据装桶完毕后，按序列，从小到大合并所有非空的桶(如0,1,2,3,4桶)



假设有n个数字（最大值与最小值之差），有m个桶，如果数字是平均分布的，则每个桶里面平均有n/m个数字。如果对每个桶中的数字采用快速排序，那么整个算法的复杂度是：O(n + m * n/m*log(n/m)) = O(n + nlogn – nlogm)  （当m接近n时，接近O(n)）  (以上复杂度的计算是基于输入的n个数字是平均分布这个假设的)



时间复杂度：O(M * (N / M) * log(N/M)) + O(N) = o(N*(log(N/M) + 1))；当m接近n时，时间复杂度为O(n)

空间复杂度：创建M个桶的额外空间，以及N个元素的额外空间，因此为O(N+M);

特性：稳定排序，外排序



适用范围：用排序主要适用于均匀分布的数字数组，在这种情况下能够达到最大效率

缺点：

- 首先是空间复杂度比较高，需要的额外开销大。排序有两个数组的空间开销，一个存放待排序数组，一个就是所谓的桶，比如待排序值是从0到m-1，那就需要m个桶，这个桶数组就要至少m个空间。

- 其次待排序的元素都要在一定的范围内等等。

例1：一年的全国高考考生人数为500 万，分数使用标准分，最低100 ，最高900 ，没有小数，你把这500 万元素的数组排个序

创建801(900-100)个桶(n)。将每个考生的分数丢进f(score)=score-100的桶中。这个过程从头到尾遍历一遍数据只需要500W次。然后根据桶号大小依次将桶中数值输出，即可以得到一个有序的序列

局限性：桶排序对数据的条件有特殊要求，如果上面的分数不是从100-900，而是从0-2亿，那么分配2亿个桶显然是不可能的。所以桶排序有其局限性，适合元素值集合并不大的情况

```c
#define BUCKET_NUM 10
#define BUCKET_CAP 100
#define MAX_NUM 10

void bucket_sort(int *data, int len){
    int **p = (int **)malloc(sizeof(int *) * BUCKET_NUM);
    int i, j, temp, flag;
    for (i = 0; i < BUCKET_NUM; i ++)
        *(p + i) = (int *)malloc(sizeof(int) * BUCKET_NUM);
    int cnts[BUCKET_NUM] = {0};
    for (i = 0; i < len; i ++) {
        temp = data[i];
        flag = temp / (MAX_NUM / BUCKET_NUM);
        p[flag][cnts[flag]] = temp;
        j = cnts[flag]++;
        for(;j > 0 && temp < p[flag][j-1]; j --) 
            p[flag][j] = p[flag][j-1];
        p[flag][j] = temp;
    }
    
    int k = 0;
    for (i = 0; i < BUCKET_NUM; i++){
        for (j = 0; j < cnts[i]; j++) {
            data[k++] = p[i][j];
        }
    }
    for(i = 0; i < BUCKET_NUM; i++){
        free(*(p + i));
    }
    free(p);
}

void init_data(int *data, int len){
    int i;
    for (i = 0; i < len; i ++) {
        data[i] = rand() % MAX_NUM;
    }
}

int main() {
    int len = 50;
    int data[len];
    init_data(data, len);
    bucket_sort(data, len);
    int i;
    for(i = 0; i < len; i ++){
        printf("%d ", data[i]);
    }
    return 0;
}
```



#### 10. 基数排序

基数排序(Radix Sort)是桶排序的扩展，它的基本思想是：将整数按位数切割成不同的数字，然后按每个位数分别比较。
具体做法是：将所有待比较数值统一为同样的数位长度，数位较短的数前面补零。然后，从最低位开始，依次进行一次排序。这样从最低位排序一直到最高位排序完成以后, 数列就变成一个有序序列。

![radix_sort](..\pic\radix_sort.png)














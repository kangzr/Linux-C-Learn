排序，KMP算法，队列，栈



samba windows与linux虚拟机映射

mac虚拟机



10大排序（全部手写，手写快排）

1. 冒泡
2. 选择
3. 插入
4. 希尔排序：以gap进行分组排序;

```c
void shell_short(int *data, int length) {
    int gap = 0, i = 0, temp = 0, j = 0;
    for(gap = length / 2; gap > 0; gap /= 2) {
        for(i = gap; i < length; i++) {
            temp = data[i];
            for(j = i - gap; j >= 0 && temp < data[j]; j -= gap) {
                data[j+gap] = data[j];
            }
            data[j+gap] = temp;
        }
    }
}

```

5. 归并排序：分治法

```c
void merge(int *data, int *temp, int start, int middle, int end){
    int i = start, j = middle + 1, k = start;
    while (i <= middle && j <= end){
        if(data[i] < data[j])
            temp[k++] = data[i++];
        else
            temp[k++] = data[j++];
    }
    while (i <= middle){
        temp[k++] = data[i++];
    }
    while (j <= end){
        temp[k++] = data[j++];
    }
    for(i = start; i <= end; i++){
        data[i] = temp[i];
    }
}


void merge_sort(int *data, int *temp, int start, int end) {
    int middle = 0;
    if (start > end){
        middle = start + (end - start) / 2;
        merge_sort(data, temp, start, middle);
        merge_sort(data, temp, middle+1, end);
        merge(data, temp, start, middle, end);
    }
}
```

6. 快速排序



```c
void quick_core(int *data, int start, int end){
    int low = start, high = end, key = data[start];
    while (low < high){
        while(low < high && key <= data[high]){
            high--;
        }
        data[low] = data[high];
        while(low < high && key >= data[low]){
            low++;
        }
        data[high] = data[low];
    }
    data[low] = key;
    quick_core(data, start, low - 1);
    quick_core(data, low + 1, end);
}

void quick_sort(int *data, int length){
    quick_core(data, 0, length - 1);
}
```





















































6. 堆排序：（实现时间轮定时器，一个定时器，实现若干个定时任务）
7. KMP：字符串样本里识别一个样本； O(texlen + ptnlen); 回溯：取决于pattern公共前缀与后缀有多少（aba --> a/ab  a/ba ---> 1）
8. 桶排序


















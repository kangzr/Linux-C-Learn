查看内存碎片：`cat /proc/buddyinfo`

左边数值越大，可使用的小内存越多，右边值越大，可用的大内存越多。linux的buddy算法会不断从大内存中分割小内存来使用，并不断把小内存拼接成大内存。

（内存不够用时）write需要很久可能触发buddy算法，系统开始小内存拼接

DROP_SLABS 是清理内核slab对象
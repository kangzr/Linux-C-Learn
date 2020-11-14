#include <stdio.h>
#include <unistd.h>
#include <sys/times.h>
#include <sys/types.h>
#include <time.h>

/************************************
 * 缓存
 *
 * 程序运行:
 * 程序计数器PC: 下一条指令(在内存中)的地址
 * 指令寄存器IR: 当前执行的指令的机器码
 * 栈指针: 当前函数栈帧的指针，包含函数参数和局部变量
 * 
 * CPU执行: 
 * 取指：从内存中获取下一条指令，存储在IR中》
 * 译码：CPU中“控制单元”将指令译码，并向CPU其它部分发送新号
 * 执行：收到来自控制单元的信号后会执行合适的计算
 *
 * EXAMPLE:
 * 读写元素，并度量平均时间(缓存性能的度量)
 *
 * // time_avg: 内存访问平均时间，h: 缓存命中率, m: 缺失率, Th: 命中处理时间, Tm: 缺失处理时间
 *
 * time_avg = h * Th + m * Tm
 *
 * Tp = Tm - Th  // 缺失惩罚(处理缓存未命中所需额外时间)
 *
 * time_avg = Th + m * Tp
 *
 * 可以推断出缓存大小:
 * 数组超过缓存大小，缺失率会剧增
 *
 * 可以推断出块的大小:
 * 1. 步长等于块的大小，空间局部性为0，读取一个块只访问一个元素, 缺失惩罚最大
 * 2. 根据平均缺失惩罚,和1推断
 *
 * Q: 缓存：既然这么快，为啥不使用大块缓存？
 * A: 从电子和经济学看，缓存很快是由于小，且离CPU很近，
 *	  可减少由于电容造成的延迟和信号传播。如果做的很大, 就会变的很慢
 *
 * 缓存策略: LRU 最近最少使用
 *
 * 页面调度/换页/PCB/实时调度
 *
 ************************************/
#define CACHE_MIN (32*1024)
#define CACHE_MAX (32*1024*1024)
#define SAMPLE 10

int x[CACHE_MAX];
long clk_tck;

double get_seconds(){
	struct tms rusage;
	times(&rusage);
	return (double) (rusage.tms_utime)/clk_tck;
}

int main(){
	int register i, index, stride, limit, temp;
	int steps, tsteps, csize;
	double sec0, sec;

	clk_tck = sysconf(_SC_CLK_TCK);

	for(csize = CACHE_MIN; csize <= CACHE_MAX; csize=csize*2){
		for(stride=1; stride <= csize/2; stride=stride*2){
			sec = 0;  // 跟踪cpu用于循环的全部时间
			limit = csize - stride + 1;
			steps = 0;
			do{  /*repeat until collect 1 second*/
				sec0 = get_seconds();  /*start timer*/
				for(i=SAMPLE*stride; i!=0; i=i-1)
					for(index=0; index < limit; index=index+stride)
						x[index] = x[index] + 1;
				steps += 1;
				sec += (get_seconds() - sec0);
			}while(sec < 0.1);

			tsteps = 0;
			do{
				sec0 = get_seconds();
				for(i=SAMPLE*stride; i!=0; i=i-1)
					for(index = 0; index < limit; index=index+stride)
						temp += index;
				tsteps += 1;
				sec -= (get_seconds() - sec0);
			}while(tsteps<steps);
			// sec - temp增加的时间，即所访问触发的全部缺失惩罚
			printf("Size: %7ld Stride: %7ld read+write: %4.4lf ns\n",
					csize*sizeof(int), stride*sizeof(int),
					(double) sec*1e9/(steps*SAMPLE*stride*((limit-1)/stride+1)));
		};
	
	}
	return 0;
}

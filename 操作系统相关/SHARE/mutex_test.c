// epoll 通过互斥锁mutex进行加锁  (在插入fd到红黑树时) 锁子树(对节点加锁)spinlock()

#include <stdio.h>
#include <pthread.h>
#include <time.h>

#define TREHAD_NUM 10

pthread_mutex_t mutex;
pthread_spinlock_t spinlock;


void *thread_proc(void *arg){
	int *pcount = (int *) arg;
	int i = 0;

	while(i++ < 100000){
#if 0
		(*pcount)++;
#elif 0
		pthread_mutex_lock(&mutex);
		(*pcount)++;
		pthread_mutex_unlock(&mutex);
#else
		pthread_spin_lock(&spinlock);
		(*pcount)++;
		pthread_spin_unlock(&spinlock);
#endif
		usleep(1);
	}

}

int main() {
	int i = 0;
	pthread_t thread_id[TREHAD_NUM] = {0};
	int count = 0;


	pthread_mutex_init(&mutex, NULL);
	pthread_spin_init(&spinlock, PTHREAD_PROCESS_SHARED);

	for (i = 0; i < TREHAD_NUM; i++) {
		pthread_create(&thread_id[i], NULL, thread_proc, &count);
	}

	for (i = 0; i < 100; i++) {
		printf("count --> %d\n", count);
		sleep(1);
	}
	return 0;
}

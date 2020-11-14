#include <pthread.h>

typedef struct{
	pthread_mutex_t mutex[1];
}Mutex;

Mutex *make_mutex ();
void mutex_lock(Mutex *mutex);
void mutex_unlock(Mutex *mutex);

#include <stdlib.h>
#include <pthread.h>
#include "mutex.h"

Mutex *make_mutex()
{
	Mutex *mutex = (Mutex *) malloc(sizeof(Mutex));
	pthread_mutex_init(mutex->mutex, NULL);
	return mutex;
}

void mutex_lock(Mutex *mutex){
	pthread_mutex_lock(mutex->mutex);
}

void mutex_unlock(Mutex *mutex){
	pthread_mutex_unlock(mutex->mutex);
}

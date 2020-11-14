#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include "utils.h"

#define NUM_CHILDREN 2
#define QUEUE_LENGTH 16

typedef struct {
	int *array;
	int length;
	int next_in;
	int next_out;
	Mutex *mutex;
} Queue;


Queue *make_queue(int length){
	Queue *queue = (Queue *) malloc(sizeof(Queue));
	queue->length = length;
	queue->array = (int *) malloc(length * sizeof(int));
	queue->next_in = 0;
	queue->next_out = 0;
	queue->mutex = make_mutex();
	return queue;
}

int queue_incr(Queue *queue, int i){
	return (i+1) % queue->length;
}

int queue_empty(Queue *queue){
	return (queue->next_in == queue->next_out);
}

int queue_full(Queue *queue){
	int res = (queue_incr(queue, queue->next_in) == queue->next_out);
	return res;
}

void queue_push(Queue *queue, int item){
	mutex_lock(queue->mutex);
	if (queue_full(queue)){
		mutex_unlock(queue->mutex);
		perror_exit("queue is full");
	}

	queue->array[queue->next_in] = item;
	queue->next_in = queue_incr(queue, queue->next_in);
	mutex_unlock(queue->mutex);
}

int queue_pop(Queue *queue){
	mutex_lock(queue->mutex);
	if (queue_empty(queue)){
		mutex_unlock(queue->mutex);
		perror_exit("queue is empty");
	}
	int item = queue->array[queue->next_out];
	queue->next_out = queue_incr(queue, queue->next_out);
	mutex_unlock(queue->mutex);
	return item;
}

typedef struct {
	Queue *queue;
} Shared;

Shared *make_shared(){
	Shared *shared = check_malloc(sizeof(Shared));
	shared->queue = make_queue(QUEUE_LENGTH);
	return shared;
}

pthread_t make_thread(void *(*entry)(void*), Shared *shared){
	int ret;
	pthread_t thread;
	ret = pthread_create(&thread, NULL, entry, (void *)shared);
	if (ret != 0){
		perror_exit("pthread_create failed");
	}
	return thread;
}

void join_thread(pthread_t thread){
	int ret = pthread_join(thread, NULL);
	if (ret == -1){
		perror_exit("pthread_join failed");
	}
}


// PRODUCE_CONSUMER

void *producer_entry(void *arg){
	int i;
	Shared *shared = (Shared *) arg;
	for (i=0; i<QUEUE_LENGTH-1; i++){
		printf("produce item %d\n", i);
		queue_push(shared->queue, i);
	}
	pthread_exit(NULL);
}

void *consumer_entry(void *arg){
	int i;
	int item;
	Shared *shared = (Shared *) arg;
	printf("consume_entiry---");
	for (i=0; i<QUEUE_LENGTH-1; i++){
		item = queue_pop(shared->queue);
		printf("consume item %d\n", item);
	}
	pthread_exit(NULL);
}

void queue_test(){
	int i, item, length=128;

	Queue *queue = make_queue(length);
	assert(queue_empty(queue));
	for (i=0; i<length; i++){
		printf("queue_test: %d\n", i);
		queue_push(queue, i);
	}
	assert(queue_full(queue));
	for(i=0; i<10; i++){
		item = queue_pop(queue);
		assert(i == item);
	}
	assert(!queue_empty(queue));
	assert(!queue_full(queue));
	for (i=0; i<10; i++){
		item = queue_pop(queue);
	}
	assert(item == 19);
}

int main(){
	int i;
	pthread_t child[NUM_CHILDREN];
	Shared *shared = make_shared();
	child[0] = make_thread(producer_entry, shared);
	child[1] = make_thread(consumer_entry, shared);

	for (i=0; i<NUM_CHILDREN; i++){
		join_thread(child[i]);
	}
	return 0;
}

typedef pthread_mutex_t Mutex;
typedef pthread_cond_t Cond;

void perror_exit(char *s);
void *check_malloc(int size);

Mutex *make_mutex();
void mutex_lock(Mutex *mutex);
void mutex_unlock(Mutex *mutex);

Cond *make_cond();
void cond_wait(Cond *cond, Mutex *mutex);
void cond_signal(Cond *cond);

### 栈和队列

#### 栈的实现

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

#### 队列的实现

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



#### 两个队列实现一个栈

#### 两个栈实现一个队列




























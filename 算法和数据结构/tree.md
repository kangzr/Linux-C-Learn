### 一. 红黑树

#### 1. 红黑树性质

- 每个节点是红或者黑
- 根节点是黑色
- 每个叶子节点是黑的
- 如果一个节点是红色的，则它的两个儿子是黑色(红色不能相邻)
- **对每个节点，从该节点到其子孙节点的所有路径上的包含相同数目的黑节点**（黑色节点高度）

设计红黑树的目的，就是解决平衡树的维护起来比较麻烦的问题，红黑树，读取略逊于AVL，维护强于AVL，每次插入和删除的平均旋转次数应该是远小于平衡树。

#### 2. 数据结构定义

```c
#define RED				1
#define BLACK			2
typedef int KEY_TYPE;


typedef struct _rbtree_node {
  unsigned char color;
  struct _rbtree_node *right;
  struct _rbtree_node *left;
  struct _rbtree_node *parent;
  KEY_TYPE key;
  void *value;
}rbtree_node;

typedef struct _rbtree {
  rbtree_node *root;
  rbtree_node *ni;
}rbtree;

// 最小值
rbtree_node *rbtree_min(rbtree *T, rbtree_node *x){
  while (x->left != T->nil){
    x = x->left;
  }
  return x;
}

// 最大值
rbtree_node *rbtree_max(rbtree *T, rbtree_node *x){
  while (x->right != T->nil){
    x = x->right;
  }
  return x;
}
```

#### 3. 红黑树的插入

需要调整的情况有三种

当前z是红色，z的父节点是红色的，z的祖父节点肯定是黑色

- 父节点是祖父节点左子树情况

  - 叔父节点红色；(不需要旋转，只需边色)

  - 叔父节点是黑色：

    - z为右子树，需多一步以z的父节点左旋

    - 变色
    - 以z的祖父节点右旋

- 父节点是祖父节点右子树情况 与上述情况对称

  

```c
// 左旋
void rbtree_left_rotate(rbtree *T, rbtree_node *x){
  rbtree_node *y = x->right;
  // 1 
  x->right = y->left;
  if (y->left != T->nil){
    y->left->parent = x;
  }
  // 2
  y->parent = x->parent;
  if(x->parent = T->nil){
    T->root = y;
  }else if(x == x->parent->left){
    x->parent->left = y;
  }else{
    x->parent->right = y;
	}
  y->left = x;
  x->parent = y;
}

// 右旋，left-->right. right-->left
void rbtree_right_rotate(rbtree *T, rbtree_node *x){
  // 
}

void rbtree_insert_fixup(rbtree *T, rbtree_node *z){
  while (z->parent->color == RED){  // z的父节点为红色才需要调整
    if (z->parent = z->parent->parent->left){  // z的父节点为左子树情况
      rbtree_node *y = z->parent->parent->right;  // 获取z的叔父节点y
      if (y->color == RED){  // y为红色情况,只需变色
        z->parent->color = BLACK;
        y->color = BLACK;
        z->parent->parent->color = RED;
        z = z->parent->parent;  // 再判断其祖父节点是否满足
      } else{  // y为黑色情况
        if (z == z->parent->right){  // z为右子树
          z = z->parent;
          rbtree_left_rotate(T, z);  // 以父节点左旋
        }
        z->parent->color = BLACK;
        z->parent->parent->color = RED;
        rbtree_right_rotate(T, z->parent->parent);  // 以祖父节点右旋
      }
    }else {  // z的父节点为右子树情况  对称过来把相应的left --> right
      rbtree_node *y = z->parent->parent->left;  // 获取z的叔父节点y
      if (y->color == RED){  // y为红色情况,只需变色
        z->parent->color = BLACK;
        y->color = BLACK;
        z->parent->parent->color = RED;
        z = z->parent->parent;  // 再判断其祖父节点是否满足
      } else{  // y为黑色情况
        if (z == z->parent->left){  // z为右子树
          z = z->parent;
          rbtree_right_rotate(T, z);  // 以父节点左旋
        }
        z->parent->color = BLACK;
        z->parent->parent->color = RED;
        rbtree_left_rotate(T, z->parent->parent);  // 以祖父节点右旋
      }
    }
  }
  T->root->color = BLACK;
}

void rbtree_insert(rbtree *T, rbtree_node *z){
  rbtree_node *y = T->nil;
  rbtree_node *x = T->root;
  // 找到待插入节点
  while (x != T->nil){  // 红黑树不为空
    y = x;
    if (z->key < x->key){
      x = x->left;
    }else if (z->key > x->key){
      x = x->right;
    } else{ // 存在
      return ;
    }
  }
	// 插入
  z->parent = y;
  if (y == T->nil){
    T->root = z;
  } else if (z->key < y->key){
    y->left = z;
  }else{
    y->right = z;
  }
 
  z->left = T->nil;
  z->right = T->nil;
  z->color = RED;  // 插入的节点均初始化为红色
   // 插入完成后，调整红黑树
  rbtree_insert_fixup(T, z);
}
```



#### 4. 红黑树的删除

z：覆盖节点(名义上删除z)（被删节点不会真正删除，而是用后继节点的value替换）

y：删除节点（真正删除的节点）

x：轴心节点

前提：z有右子树，后继y有子节点，则只有右子节点，且y是黑色

1. 兄弟节点是红色，（其两个子节点肯定为黑）  ： 右旋操作（以父节点为轴心旋转）
2. 兄弟节点是黑色，要么左子节点是红色：
3. 兄弟节点是黑色，要么右子节点是红色：（1. 先以兄弟为轴心旋转左旋; 2. 再以x为核心右旋）
4. 兄弟节点是黑色，两个子节点也是黑色：（把兄弟节点变红色，）

只有删除(后继)节点y是黑色时，才需要调整红黑树

#### 5. 红黑树搜索

```c
rbtree_node *rbtree_search(rbtree *T, KEY_TYPE key){
  rbtree_node *node = T->node;
  while (node != T->nil){
    if (key < node->key){
      node = node->left;
    }else if (key > node->key){
      node = node->right;
    } else{
      return node;
    }
  }
  return T->nil;
}
```

#### 6. 遍历

分为前中后三种情况

```c
// 中序 节点node被访问两次
void rbtree_traversal(rbtree *T, rbtree_node *node){
  if(node != T->nil){
    rbtree_traversal(T, node->left);
    printf("key: %d, color: %d\n", node->key, node->color);
    rbtree_traversal(T, node->right);
  }
}
```



开源项目中，整理三个用到红黑树的地方，以及如何使用。



### 二. B-树 (B-tree)/B+树

二叉树：内存存不下时，需要存到磁盘里面，每次节点访问都需寻址，1024个节点，就需要访问10层，效率不高。

因此，可以降低层高来减少检索次数；

B树属于多叉树，其所有的叶子节点都在统一高度上。



内存寻址磁盘的最小单位：扇区，一个扇区对应一个或者多个B树节点 



**B树与B+树区别：**

- B+树在B树的基础上，把叶子节点加上了前后指针，所有的值都存在叶子节点，可以方便的进行范围查找，例如数据库加索引查询（`MySQL`就是用这个）；
- B+树天然具备排序功能，所有叶子节点构成了一个有序链表



**B+树与红黑树区别：**

- 红黑树多用在内部排序，即全放内存中，STL的map和set内部实现就是红黑树
- B+树多用于外存，B+为一个对磁盘友好的数据结构

为什么B+对磁盘友好？

- 磁盘读写代价更低

  树的非叶子结点里面没有数据，这样索引比较小，可以放在一个blcok（或者尽可能少的blcok）里面。避免了树形结构不断的向下查找，然后磁盘不停的寻道，读数据。这样的设计，可以**降低io的次数**。

- 查询效率更加稳定（速度完全接近于二分法查找）

  非终结点并不是最终指向文件内容的结点，而只是叶子结点中关键字的索引。所以任何关键字的查找必须走一条从根结点到叶子结点的路。所有关键字查询的路径长度相同，导致每一个数据的查询效率相当。

- 遍历所有的数据更方便

  B+树只要遍历叶子节点就可以实现整棵树的遍历，而其他的树形结构 要中序遍历才可以访问所有的数据。



#### B树实现

```c
typedef int KEY_VALUE:
#define DEGREE		3

typdef struct _btree_node {
  KEY_VALUE *keys;  // 
  struct btree_node **childrens;  // 二位数组
  int num;  // 当前节点有多少个值
  int leaf;  // yes: 1; no: 0
}btree_node;

typedef struct _btree {
  btree_node *root;
  int degree;
}btree;

// 创建节点
btree_node *btree_create_node(int t, int leaf){
  //使用calloc的话，就不用memset了，因为其会自动给其清零
  // calloc == malloc + memset
  btree_node *node = (btree_node*)(malloc(sizeof(btree_node)));
  if (node == NULL){
    printf("malloc failed!!!");
    return NULL;
  }
  memset(node, 0, sizeof(btree_node));
  node->leaf = leaf;
  node->keys = (KEY_VALUE*)malloc((2 * t - 1)sizeof(KEY_VALUE));
  // memset();
  if(node->keys == NULL){
    free(node);
    return NULL;
  }
  node->childrens = (btree_node**)calloc(1, (2*t) * sizeof(btree_node*));
  if (node->childrens == NULL){
    free(node->keys);
    free(node);
    return NULL;
  }
  node->num = 0;
  return node;
}

// 销毁节点
void btree_destroy_node(btree_node *node){
  if (node == NULL) return ;
  if (node->childrens) free(node->childrens);
  if (node->keys) free(node->keys);
  free(node);
}

// api 添加节点;每次添加都是到叶子节点上，添加只有分裂，删除只有合并
// 1. 插入不满的节点； 2. 插入满的节点会产生裂变split
void btree_split_child(btree *T, btree_node *x, int i){
  int degree = T->degree;
  btree_node *left = x->childrens[i];
  // 创建新的node
  btree_node *right = btree_create_node(degree, left->leaf);
  int j = 0;
  for (j = 0; j < degree-1; j++){
    right->keys[j] = left->keys[degree + j];  //
  }
  if (y->leaf == 0){
    for (j = 0; j < degree; j++){
      right->childrens[j] = left[j+degree];
    }
	}
  
  // 
  right->num = degree - 1;
  left->num = degree - 1;
  
  for (j = x->num; j >= i + 1; j--){
    x->childrens[j+1] = x->childrens[j];
  }
  x->childrens[i+1] = right;
  for (j = x->num-1; j >= i; j--){
    x->keys[j+1] = x->keys[j];
  }
  x->keys[i] = left->keys[degree-1];
  x->num++;
}

void btree_insert_nonfull(btree *T, btree_node *x, KEY_VALUE k){
  // 遍历递归的找到叶子节点，再对比key值，找到对应的位置
  int i = x->num - 1;
  if (x->leaf == 1){
    while (i >= 0 && x->keys[i] > k){
      x->keys[i+1] = x->keys[i--];
    }
    x->keys[i+1] = k;
    x->num++;
  } else {  // 不是叶子节点，有两种情况
     while (i >= 0 && x->keys[i] > k) i--;
     if (x->children[i+1]->num == (2 * T->degree - 1)){
       // 裂变
       btree_split_child(T, x, i+1);
       if (k > x->keys[i + 1] i++;)
     }
    btree_insert_nonfull(T, x->childrens[i+1], k);
  }
}

int btree_insert(btree *T, KEY_VALUE key){
  btree_node *root = T->root;
  if (node->num != 2 * T->degree - 1){ // not full
    btree_insert_nonfull(T, node, key);
  } else{  // full
    btree_node *node = btree_create_node(T->degree, 0);
    T->root = node;
    node->childrens[0] = root;
    btree_split_child(T, node, 0);
    int i = 0;
    if (node->keys[0] < key) i++:
    btree_insert_nonfull(T, node->childrens[i], key);
  }
}

// b树删除 
// 1. 删除key，不是叶子节点；key值
//    前面节点大于degree - 1。向left节点借1，
//   后面节点大雨degree-1, 向right节点借1
//   前后节点都==degree-1
// 2. 删除key，为叶子节点。

void btree_delete_key(btree *T, btree_node *node, KEY_VALUE key){
  int idx = 0, i;
  while (idx < node->num && key > node->keys[idx]) idx++;
  if (idx < node->num && key == node->keys[idx]){  // 找到了key
    // 分成4种情况
    if (node->leaf){  // 是叶子节点
      for (i = idx; i < node->num - 1; i++){
        node->keys[i] = node->keys[i-1];
      }
      node->keys[node->num - 1] = 0;
      node->num--;
      if (node->num == 0){ // root
				free(node);
        T->root = NULL;
      }
      return ;
    }else if(node->childrens[idx]->num >= T->degree - 1){ // 
      btree_node *left = node->childrens[idx];
      node->keys[idx] = left->keys[left->num - 1];
      btree_delete_key(T, left, left->keys[left->num - 1]);
      
    }else if(node->childrens[idx + 1]->num >= T->degree - 1){
      
    }else{  // 归并merge
      
    }
  }
}


int btree_delete(btree *T, KEY_VALUE key){
  
}
```
































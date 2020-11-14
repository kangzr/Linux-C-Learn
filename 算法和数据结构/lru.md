LRU算法

如果一个数据在最近一段时间没有被访问到，那么它在将来被访问的可能性也很小。

当限定空间已存满数据时，应该把最久没有被访问到的数据淘汰



```c++
class LRUCache {
    public:
    	LRUCache(int cap) {
            _capacity = cap;
        }
  		int get(int key) {
            const auto& it = _m.find(key);
            if (it == _m.end()) return -1;  // 没有找到
            // 把list中对应的<key, value>元素移动到最前面，时间复杂度为O(1)
            _cache.splice(_cache.begin(), _cache, it->second);
            return it->sencond->second;
        }  
    	
    	void put(int key, int value) {
            const auto& it = _m.find(key);
            if (it != _m.end()) {  // 找到则修改其值，并插入list最前面
                it->second.second = value;
                _cache.splice(_cahce.begin(), _cache, it->second);
                return;
            }
            // 容量满了，则删除最后元素，相应的map也要erase
            if (_cache.size() == _capacity) {
                _m.erase(_cache.back().first);
                _cache.pop_back();
            }
            // 插入list头部和map
            _cache.emplace_front(make_pair(key, value));
            _m[key] = _cache.begin();
        }
    private:
    	int _capacity;
    	list<pair<int, int>> _cache;
    	unordered_map<int, list<pair<int, int>>::iterator> _m;
    	
};
```







upperbound算法：找到大于target的第一个元素

```c++
int search(vector<int> row, int target) {
    int low = 0, high = row.size();
    while (low < high) {
        int mid =  low + (high - low) / 2;
        if (row[mid] > target)
            high = mid;
        else
            low = mid + 1;
    }
    return low;
}
```





Linus删除链表节点骚操作

```c
struct ListNode* deleteNode(struct ListNode* head, int val) {
    struct ListNode** indirect = &head;
    struct ListNode* tmp;
    for(; *indrect; indrect = &((*indrect)->next)) {
        if((*indirect)->val == val) {
            tmp = *indrect;
            *indrect = (*indrect)->next;
            free(tmp);
            break;
        }
    }
    return head;
}
```



常数时间插入删除和获取随机元素， 不允许重复，插入和删除都需要判断元素是否在set中，因此需要使用map来确保时间复杂度。

```c++
class RandomizedSet {
private:
    unordered_map<int, int> m;
    vector<int> vec;
public:
    /** Initialize your data structure here. */
    RandomizedSet() {
    }
    
    /** Inserts a value to the set. Returns true if the set did not already contain the specified element. */
    bool insert(int val) {
        if(m.count(val) == 1)
            return false;
        m[val] = vec.size();
        vec.emplace_back(val);
        return true;
    }
    
    /** Removes a value from the set. Returns true if the set contained the specified element. */
    bool remove(int val) {
        if(!m.count(val))   return false;
        m[vec.back()] = m[val];
        vec[m[val]] = vec.back();
        vec.pop_back();
        m.erase(val);
        return true;
    }
    
    /** Get a random element from the set. */
    int getRandom() {
        return vec[rand() % vec.size()];
    }
};
/**
 * Your RandomizedSet object will be instantiated and called as such:
 * RandomizedSet* obj = new RandomizedSet();
 * bool param_1 = obj->insert(val);
 * bool param_2 = obj->remove(val);
 * int param_3 = obj->getRandom();

 */
```












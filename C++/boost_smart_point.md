#### boost::scoped_ptr



#### static_cast



#### boost::shared_ptr

`boost/shared_ptr.hpp`

can shre ownership. The shared object is not released until the last copy of the shared pointer referencing the object is destroyed;

```cpp
#include <boost/shared_ptr.hpp>
#include <iostream>

int main()
{
  boost::shared_ptr<int> p1{new int{1}};
  std::cout << *p1 << '\n';
  boost::shared_ptr<int> p2{p1};
  p1.reset(new int{2});
  std::cout << *p1.get() << '\n';
  p1.reset();
  // p2 没有被销毁，static_cast<bool>(p2) 为true
  std::cout << std::boolalpha << static_cast<bool>(p2) << '\n';
  p2.reset();
  std::cout << std::boolalpha << static_cast<bool>(p2) << '\n';
  // p2 没有被销毁，static_cast<bool>(p2) 为false
}
```





#### boost::make_shared

`boost/make_shared.hpp`

create a smart pointer of type `boost::shared_ptr` without having calling to de constructor of `boost::shared_ptr` youself.

advantage: the memory for the object that has to be allocated dynamically and the memory fro the reference counter used by the smart point internally can be reserved in one chunk; so is more efficient than calling new to create a dynamically allocated object and calling `new` again in the constructor of `boost::shared_ptr` to allocate memory for the reference counter.

you can use `boost::make_shared()` for arrays, too.



#### boost::weak_ptr














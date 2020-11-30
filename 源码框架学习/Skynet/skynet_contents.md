[参考](https://zhongyiqun.gitbooks.io/skynet/content/12-skynetyuan-ma-mu-lu-jie-gou.html)

### Skynet框架

![skynet_cont](..\pic\skynet_cont.png)

### Skynet目录结构

- 3rd ：第三方库如jemalloc, lua等
- cservice: c写的skynet的服务模块，即service-src编译后的动态库，如gate.so, harbor.so
- service-src：使用c写的skynet服务模块，
- lualib：使用lua写的库，即lualib-src编译后的动态库
- lualib-src：使用c写的封装给lua使用的库
- service：使用lua写的skynet服务模块
- skynet-src：skynet核心代码
- test：lua写的一些测试代码
- examples: 例子
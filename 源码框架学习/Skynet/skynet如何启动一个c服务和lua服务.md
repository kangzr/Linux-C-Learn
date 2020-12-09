[From ](https://manistein.github.io/blog/post/server/skynet/skynet%E6%BA%90%E7%A0%81%E8%B5%8F%E6%9E%90/)

#### skynet如何启动一个c服务

我们写的c服务在编译成so库以后，在某个时段，会被加载到modules列表中。创建c服务的工作，一般在c层进行，一般会调用skynet_context_new接口;

步骤如下：

- 从modules列表中，查找对应的服务模块，如果找到则返回，否则到modules的path中去查找对应的so库，创建一个skynet_module对象（数据结构见上节），将so库加载到内存，并将访问该so库的句柄和skynet_module对象关联（`_try_open`做了这件事），并将so库中的xxx_create，xxx_init，xxx_signal，xxx_release四个函数地址赋值给skynet_module的create、init、signal和release四个函数中，这样这个skynet_module对象，就能调用so库中，对应的四个接口（`_open_sym`做了这件事）。
- 创建一个服务实例即skynet_context对象，他包含一个次级消息队列指针，服务模块指针（skynet_module对象，便于他访问module自定义的create、init、signal和release函数），由服务模块调用create接口创建的数据实例等。
- 将新创建的服务实例（skynet_context对象）注册到全局的服务列表中（handle_storage结构）
- 初始化服务模块（skynet_module创建的数据实例），并在初始化函数中，注册新创建的skynet_context实例的callback函数
- 将该服务实例（skynet_context实例）的次级消息队列，插入到全局消息队列中。

一个c服务模块就被创建出来了，在回调函数被指定以后，其他服务发送给他的消息，会被pop出来，最终传给服务对应的callback函数，最后达到驱动服务的目的。

以创建logger服务为例：

1. 启动skynet节点时，会启动一个logger c服务

   ```c
   // skynet_start.c
   void
   skynet_start(struct skynet_config * config) {
   ...
   struct skynet_context *ctx = skynet_context_new(config->logservice, config->logger);
   if (ctx == NULL) {
       fprintf(stderr, "Can't launch %s service\n", config->logservice);
       exit(1);
   }
   ...
   }
   ```

   

2. 在skynet_module列表中，搜索logger服务模块，如果没找到则在so库的输出路径中，寻找名为logger的so库，找到则将该so库加载到内存中，并将对应的logger_create,logger_init,logger_release函数地址分别赋值给logger模块中的create,init,release函数指针，并加入skynet_module列表中；

3. 创建服务实例，即创建一个skynet_context实例，为了使skynet_context实例拥有访问logger服务内部函数的权限，这里将logger模块指针，赋值给skynet_context实例的mod变量中。

4. 创建一个logger服务的数据实例，调用logger服务的create函数

5. 将新创建的skynet_context对象，注册到skynet_context list中，此时skynet_context list多了一个logger服务实例

6. 初始化logger服务，注册logger服务的callback函数

7. 为logger服务实例创建一个次级消息队列，并将队列插入到全局消息队列中

从上面的例子，我们就完成了一个logger服务的创建了，当logger服务收到消息时，就会调用_logger函数来进行处理，并将日志输出



#### skynet如何启动一个lua服务

每个skynet进程在启动时，都会启动一个lua层的launcher服务，该服务主要负责skynet运作期间，服务的创建工作。我们在lua层创建一个lua层服务时，通常会调用skynet.newservice函数

```lua
-- skynet.lua
function skynet.newservice(name, ...)
    return skynet.call(".launcher", "lua" , "LAUNCH", "snlua", name, ...)
end

-- launcher.lua
local function launch_service(service, ...)
    local param = table.concat({...}, " ")
    local inst = skynet.launch(service, param)
    local response = skynet.response()
    if inst then
        services[inst] = service .. " " .. param
        instance[inst] = response
    else
        response(false)
        return
    end
    return inst
end

function command.LAUNCH(_, service, ...)
    launch_service(service, ...)
    return NORET
end
```

此时会发送消息给launcher服务，告诉launcher服务，要去创建一个snlua的c服务，并且绑定一个lua_State，该lua_State运行名称为name的lua脚本（这个脚本是入口），这里将c服务名称、脚本名称和参数，拼成一个字符串，并下传给c层

```c
-- skynet.manager
function skynet.launch(...)
    local addr = c.command("LAUNCH", table.concat({...}," "))
    if addr then
        return tonumber("0x" .. string.sub(addr , 2))
    end
end

// lua-skynet.c
static int
_command(lua_State *L) {
    struct skynet_context * context = lua_touserdata(L, lua_upvalueindex(1));
    const char * cmd = luaL_checkstring(L,1);
    const char * result;
    const char * parm = NULL;
    if (lua_gettop(L) == 2) {
        parm = luaL_checkstring(L,2);
    }

    result = skynet_command(context, cmd, parm);
    if (result) {
        lua_pushstring(L, result);
        return 1;
    }
    return 0;
}

// skynet_server.c
static const char *
cmd_launch(struct skynet_context * context, const char * param) {
    size_t sz = strlen(param);
    char tmp[sz+1];
    strcpy(tmp,param);
    char * args = tmp;
    char * mod = strsep(&args, " \t\r\n");
    args = strsep(&args, "\r\n");
    struct skynet_context * inst = skynet_context_new(mod,args);
    if (inst == NULL) {
        return NULL;
    } else {
        id_to_hex(context->result, inst->handle);
        return context->result;
    }
}
```




























#### SpaceVim快捷键

```shell
# 搜索
[SPC] j i : 当前文件中搜索函数
[SPC] f f : 当前目录下搜索文件
[SPC] p / : 当前目录下搜索文本
# [SPC] p f : 当前目录下搜索文件 也可用CTRL + P

<leader> f t : 当前目录下搜索函数


# 注释
[SPC] ;  : 进入注释操作模式,(比如 SPC ; 4 j，这个组合键会注释当前行以及下方的 4 行。这个数字即为相对行号，可在左侧看到。)
[SPC] c l : 注释/反注释当前行
[SPC] c p : 注释/反注释当前段落
[SPC] c s : 使用完美格式注释

# 跳转
[SPC] 窗口序号

# 缓冲区
[SPC] b d : 删除当前缓冲区
[SPC] b b : 缓冲区搜索
[SPC] b n : 切换到下一个缓冲区
[SPC] b p : 切换到上一个缓冲区


[SPC] f S : 保存所有文件
[SPC] f y : 复制并显示当前文件的绝对路径


* : 高亮光标变量（:noh 取消）

# lua
# 打开命令行
[SPC] ' 
# 退出命令行exit
# 运行lua脚本
[SPC] l r

```


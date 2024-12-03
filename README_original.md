# TinyHotkeyTool

一个轻量的Windows全局快捷键工具

整个程序一个源文件共400多行代码，实现Windows下的全局快捷键功能，根据配置文件注册热键。程序无界面后台运行，响应全局快捷键执行相应命令，可通过任务管理器查看是否运行。


## 功能说明 ##

配置文件（setting.ini）说明

- 以#开始的一行为一条配置，配置项以|分隔，第二个#为该条配置结束，后面为注释，#####行表示所有配置结束

- 第一项C代表Ctrl，A代表Alt；
- 第二项英文字母代表对应的按键（最多两个）；
- 第三项为要启动的程序及命令行参数；
- 第四项为特殊参数，[clipboard]代表把剪切板文本当作命令行参数，[exit]代表关闭本程序，[uac]代表获取管理员权限
- 修改配置后需重启程序生效

- 配置文件示例

```
#CA|K||[exit]# 		             	Ctrl+Alt+K 关闭本程序
#CA|U||[uac]# 		             	Ctrl+Alt+U 获取管理员权限
#CA|C|C:\Windows\System32\calc.exe#        Ctrl+Alt+C打开计算器
#CA|D|explorer "C:\Users\Yang\Documents\"# Ctrl+Alt+D打开文档文件夹
#C|IP|"C:\_ipconfig.bat"#                  Ctrl+I+P打开脚本_ipconfig.bat
#CA|W|QRCode.exe|[clipboard]#           Ctrl+Alt+W 以剪切板文本为参数打开QRCode.exe二维码生成程序
```

使用说明

- 将编译好的ShortCut.exe文件和配置好的配置文件放到同一个目录下
- 运行ShortCut.exe程序弹出一个对话框询问是否开机启动，选择是或者否程序后台运行监听快捷键
- 当快捷键打开的程序需要管理员权限时，每次启动Windows都会弹出一个确认窗口，可以通过Ctrl+Alt+U快捷键（或者自定义的其他键）获取管理员权限，这样之后再打开需要管理员权限的程序时就不会再弹确认窗口了
- 需要关闭本程序时，按下Ctrl+Alt+K快捷键（或者自定义的其他键）会弹出一个退出确认对话框，选择是退出本程序


## 编译说明 ##

打开VS新建一个VC++空项目，然后将shortcut.cpp添加到项目并编译即可
（Win10系统，VS2008、VS2013、VS2017测试）




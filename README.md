# kindle
基于S5P6818平台+Linux系统, 实现kindle电子阅读系统

# 硬件和操作系统平台以及支持库


# 调试模块(debug)
## 调试选项管理
* 负责各个调试选项的注册(加入调试选项链表), 调试初始化等功能

## netprint: 网络调试选项, 通过socket发送调试信息, 接受调试控制设置
1. 包括两个线程, 网络发送线程用于发送循环队列数据, 当调用NetDbgPrint函数时候
    会唤醒发送线程. 网络接受线程用于接受调试客户端发送的控制指令
2. 调试控制命令包括:
* setclient            : 设置接收打印信息的客户端
* dbglevel=0,1,2...    : 修改打印级别
* stdout=0             : 关闭stdout打印
* stdout=1             : 打开stdout打印
* netprint=0           : 关闭netprint打印
* netprint=1           : 打开netprint打印

## stdout: 标准输出调试选项, 通过终端打印调试信息
1. 仅仅设置StdoutDebugPrint函数即可, printf即可


# 显示模块(display)

## 显示管理
* 负责管理一个显示选项链表, 以及分配显存, 获取/显示显示选项

## fb显示方式
1. 



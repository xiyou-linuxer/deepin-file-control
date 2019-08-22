
# xFileContorl产品说明文档
 * 点击此可直接跳转查看 [操作手册](https://mp.csdn.net/mdeditor#_301)


-  [一、简介](https://mp.csdn.net/mdeditor#_3)

    - - [1.目的](https://mp.csdn.net/mdeditor#1_17)
      - [2. 范围](https://mp.csdn.net/mdeditor#2__21)

- [二、产品概述](https://mp.csdn.net/mdeditor#_27)

    - - [1. 总体框图](https://mp.csdn.net/mdeditor#1__35)

      - - [1.1 示意图](https://mp.csdn.net/mdeditor#11__37)

        - [1.2 效果](https://mp.csdn.net/mdeditor#12__49)

        - [1.3 流程](https://mp.csdn.net/mdeditor#13__63)

        - - [1.3.1 client](https://mp.csdn.net/mdeditor#131_client_65)
          - [1.3.2 server](https://mp.csdn.net/mdeditor#132_server_69)

      - [2.功能摘要](https://mp.csdn.net/mdeditor#2_73)

      - - [2.1 客户端](https://mp.csdn.net/mdeditor#21__75)
        - [2.2 服务端](https://mp.csdn.net/mdeditor#22__85)
- [三、产品特性](https://mp.csdn.net/mdeditor#_93)

    - - [1. 功能展示](https://mp.csdn.net/mdeditor#1__95)

      - [2. 运行环境](https://mp.csdn.net/mdeditor#2__109)

      - - [2.1 硬件环境](https://mp.csdn.net/mdeditor#21__111)

        - [2.2 软件环境](https://mp.csdn.net/mdeditor#22__117)

        - [运行环境：](https://mp.csdn.net/mdeditor#_119)

        - [开发环境：](https://mp.csdn.net/mdeditor#_123)

        - - [客户端：](https://mp.csdn.net/mdeditor#_125)
          - [服务端：](https://mp.csdn.net/mdeditor#_131)

- [四、代码框架](https://mp.csdn.net/mdeditor#_139)

    - - [4.1 hook端框架](https://mp.csdn.net/mdeditor#41_hook_141)

      - [4.2 client端框架](https://mp.csdn.net/mdeditor#42_client_157)

      - - [threadpool](https://mp.csdn.net/mdeditor#threadpool_174)
        - [Monitored_event](https://mp.csdn.net/mdeditor#Monitored_event_187)

      - [4.3 server端框架](https://mp.csdn.net/mdeditor#43_server_204)

      - - [EventLoop](https://mp.csdn.net/mdeditor#EventLoop_216)
        - [Epoll](https://mp.csdn.net/mdeditor#Epoll_227)
        - [Channel](https://mp.csdn.net/mdeditor#Channel_240)
        - [Buffer](https://mp.csdn.net/mdeditor#Buffer_266)
        - [Socket](https://mp.csdn.net/mdeditor#Socket_280)

      - [4.4 数据展示](https://mp.csdn.net/mdeditor#44__293)

      - - [特性描述](https://mp.csdn.net/mdeditor#_295)

      - [4.5 性能及运行需求](https://mp.csdn.net/mdeditor#45__297)

- [五、操作手册](https://mp.csdn.net/mdeditor#_301)

    - - [1.使用平台](https://mp.csdn.net/mdeditor#1_303)
      - [2.服务端使用概述](https://mp.csdn.net/mdeditor#2_307)
      - [3. 客户端使用概述](https://mp.csdn.net/mdeditor#3__331)
      - [4.使用注意事项](https://mp.csdn.net/mdeditor#4_410)

- [六、问题反馈联系方式](https://mp.csdn.net/mdeditor#_424)

- [七、源代码地址](https://mp.csdn.net/mdeditor#_430)



### 一、简介

xFileContorl是一款以MIT协议进行开源的、基于Deepin GNU/Linux下开发，专为Deepin操作系统打造的文件控制器。

xFileContorl 1.0 已经具备文件监视的能力， 用户可以监视指定目录下文件的读取，使读取文件信息受到严格的限制，当打开一个已经被检测到的文件时，该文件内容被修改为“it is a secret”。

xFileContorl客户端工具并不仅仅是监听文件系统事件并上报的功能，而是有权限对文件操作进行过滤、上报甚至拒绝的“保险箱”工具。

目前来看，对于有安全性的需求的、希望使用轻量简约风格客户端的用户，会被此产品吸引并使用。

对于用户而言，xFileContorl 1.0 会有用户门槛等问题。与此同时，主办方向开发团队提供了国产终端硬件开发环境。

依据以上信息，进行了xFileContorl第二期的开发，即为xFileContorl2.0进行国产平台计算机软件移植、增加简易用户引导，优化传输大文件速度控制，支持加密传输等等。


#### 1.目的

此文档的目的是为使用该产品的开发人员提供直观、清晰、有逻辑、有层次的定义各个模块的内容来源和相关的逻辑、背景、优先级。



#### 2. 范围

此文档主要描述xFileContorl产品的功能点，交互逻辑与细节，主要读者为研发人员。



### 二、产品概述

​	本产品面对的用户主要使用文件监视客户端，文件监控的服务端主要由研发人员设置使用。

​	打开xFileContorl后，输入想要检测的目录进行监控后，再去打开此目录下文件 会显示 it's a secret。

​	客户端主要用来监视和记录指定目录下文件的打开和关闭动作（文件系统事件open 和 close），并上报服务端（文件路径、文件句柄、操作方式等）。客户端在配置了指定的监视服务器情况下，可接受服务端下发的文件操作指令。当监视服务器关闭或者网络不通时，客户端拒绝任何操作，不允许任何人打开文件（拒绝监视目录的一切文件操作）。监视服务器不存在或者未配置时，记录相关的操作到日志文件，监视服务器可以查询历史操作日志。


#### 1. 总体框图

##### 1.1 示意图





![sc2](https://img-blog.csdnimg.cn/20190823021925358.jpg?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L2tra2tkZQ==,size_16,color_FFFFFF,t_70)

![sc](https://github.com/okokme/project/blob/master/deepin-file/sc.jpg)



##### 1.2 效果

客户端：

![](https://github.com/okokme/project/blob/master/deepin-file/client1.jpg)


服务端：

![](https://github.com/okokme/project/blob/master/deepin-file/server.jpg)

##### 1.3 流程

###### 1.3.1 client

![client](https://img-blog.csdnimg.cn/20190823021853806.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L2tra2tkZQ==,size_16,color_FFFFFF,t_70)

###### 1.3.2 server

![server](https://img-blog.csdnimg.cn/20190823021832925.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L2tra2tkZQ==,size_16,color_FFFFFF,t_70)

#### 2.功能摘要

##### 2.1 客户端

* 监测本地监测目录的open、close操作，支持本地多进程访问处理；
* 劫持不可预测执行文件对监测目录文件的open、close系统调用操作，并进行内容重写（重写内容为“It is a srcrest!”）；
* 与本地调用open、colse的进程进行通信，其中包括对open、close系统调用的Ring3劫持和操作返回的信息通知；
* 与服务器进行通信，对文件内容进行加密压缩上传备份；
* 与服务器进行通信，对文件内容进行解密解压下载还原；



##### 2.2 服务端

* 接受客户端的连接请求，支持多连接以及多请求；
* 当客户端请求备份并上传文件时，接收文件并进行本地备份处理；
* 当客户端请求取备份文件时，根据Mac地址寻找文件，并发送至客户端；



### 三、产品特性

#### 1. 功能展示

| 功能                                                         | 实现 |
| ------------------------------------------------------------ | ---- |
| server端和client端工具都打开，使用测试工具读取，显示“It is a secret” | 成功 |
| server端和client端工具都打开，使用测试工具读取后，按继续恢复文件内容 | 成功 |
| 当server端关闭后，不允许任何人打开文件                       | 成功 |
| 当client的监视进程不存在时，按照原始的权限来读取文件         | 成功 |
| 当server和client监视正常时，使用其他方式打开，读取，重新，关闭文件时，要符合限制要求 | 成功 |
| 当server端退出后，再次启动后，client应该重新连接             | 成功 |
| 测试指定目录下其他文件，也需要同样的限制                     | 成功 |
| 当client端退出，server正常运行，继续监控其他client端         | 成功 |
| open时压缩、加密从客户端传到服务端，close时从服务端传到客户端解密、解压 | 成功 |

#### 2. 运行环境

##### 2.1 硬件环境

（1）服务器：CPU：Pentium 双核 以上，内存：1G以上；

（2）客户端：CPU：P4以上，内存：256M以上；支持国产龙芯芯片

##### 2.2 软件环境

##### 运行环境：

​	 Deepin Linux x86_64 GNU/Linux

##### 开发环境：

###### 	   客户端：

​	   （1）操作系统：Deepin Linux x86_64 GNU/Linux

​	   （2）编译版本号：gcc version 7.3.0 (Debian 7.3.0-19)

###### 	    服务端：

​	    （1）操作系统：Deepin 15.9.2 Linux x86_64 GNU/Linux

​	    （2）编译版本号：gcc version 7.3.0 (Debian 7.3.0-19)



### 四、代码框架
![](https://img-blog.csdnimg.cn/2019082302144171.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L2tra2tkZQ==,size_16,color_FFFFFF,t_70)

#### 4.1 hook端框架
hook框架主要由四个函数模块和两个枚举类型组成。

| 函数模块                                                     | 功能                                                       |
| ------------------------------------------------------------ | ---------------------------------------------------------- |
| int open(const char *pathname, int flags, ...)               | 劫持本地被监测目录的open调用                               |
| int close(int fd)                                            | 劫持本地被监测目录的close调用                              |
| enum MONITOR_STATE Unix_Socket(enum TYPE_HOOK types, char *pathname) | Unix域套接字创建以及连接                                   |
| enum MONITOR_STATE prase_monitor_package(const char *package) | 接收监测工具进程返回的状态（备份是否成功，取备份是否成功） |
| Socket                                                       | 封装了一些基本的socket系列函数                             |

| 枚举类型                                                     | 含义                                |
| ------------------------------------------------------------ | ----------------------------------- |
| enum TYPE_HOOK{OPEN_CALL=0,CLOSE_CALL}                       | 启动hook函数的系统调用open或者close |
| enum MONITOR_STATE{OPEN_SAVE_OK,<br>CLOSE_GET_OK,<br> OPEN_SAVE_FAILT,<br>CLOSE_GET_FAILT,<br>USOCKET_FAILT,<br>UCONNECT_FAILT,<br>UWRITE_FAILT} | 备份和取备份成功，以及失败返回      |

#### 4.2 client端框架

client框架主要由线程池+事件封装类队列和epoll组成

| 类              | 用途                                     |
| --------------- | ---------------------------------------- |
| Monitored_event | Unix域套接字事件封装，即本地进程事件封装 |

| 类         | 用途                           |
| ---------- | ------------------------------ |
| threadpool | 线程池处理任务队列中的任务事件 |

| epoll         | 事件表                                   | 模式                                                         |
| ------------- | ---------------------------------------- | ------------------------------------------------------------ |
| epoll相关函数 | Unix域的套接字、与远程服务器连接的套接字 | 本地Unix套接字采用ET模式与非EPOLLOENSHOT模式，远程连接的套接字采用ET模式+EPOLLONESHOT模式 |


##### threadpool

| 类模版成员函数          | 功能                                                 |
| ----------------------- | ---------------------------------------------------- |
| threadpool()            | 构造函数，创建线程并且进入线程执行函数               |
| ~threadpool()           | 析构函数                                             |
| bool addjob(T* request) | 往事件队列中添加一个事件                             |
| void run()              | 轮询等待任务队列有事件，并且从队列中拿出事件进行处理 |

| 线程池与监测工具的接口 | 功能                   |
| ---------------------- | ---------------------- |
| request->do_process()  | 调用到事件类的接口函数 |

##### Monitored_event

| 类成员函数                                     | 功能                                                         |
| ---------------------------------------------- | ------------------------------------------------------------ |
| void init(int ed, int i_s, int u_s)            | 被监测事件含参构造并初始化                                   |
| void close_monitored()                         | 关闭连接                                                     |
| void do_process()                              | 分析被监测事件的类型,线程池轮询事件队列的接口                |
| bool u_write()                                 | Unix写到hook.c进程函数                                       |
| bool i_write()                                 | 与远端服务器连接写函数                                       |
| bool u_read()                                  | Unix读取hook.c进程发送包函数                                 |
| bool i_read()                                  | 与远端服务器连接的函数                                       |
| void Monitored_modfd(int epfd, int fd, int ev) | 因为是EPOLLNESHOT,所以每次要修改epoll事件表                  |
| bool get_line(const char *test_buf)            | 获取协议包头的每行并且解析                                   |
| Request_State parse_read_buf()                 | 通过解析Unix套接字的读缓冲区,判断是open调用还是close调用被劫持 |
| void fill_swrite_buf(Request_State state)      | 填写向服务器发送的写缓冲区,根据请求类型进行填写响应包        |
| void fill_uwrite_buf(Request_State state)      | 填写Unix的发送缓冲区,根据请求填写响应包                      |

#### 4.3 server端框架

服务端框架主要由5个类组成。                                                                                                

| 类        | 用途                                                         |
| --------- | ------------------------------------------------------------ |
| EventLoop | 事件循环框架的主体，管理所有注册的Channel                    |
| Epoll     | 封装系统底层的I/O多路复用机制，对外提供事件注册、修改、删除的接口 |
| Channel   | 抽象一个连接，用于管理一个连接的数据接收、发送               |
| Buffer    | 一个可读、可写的缓冲区，通常搭配NIO使用                      |
| Socket    | 封装了一些基本的socket系列函数                               |

##### EventLoop
| 接口                | 功能                                   |
| ------------------- | -------------------------------------- |
| addChannel()        | 向loop中添加一个新的Channel            |
| delChannel()        | 从loop中移除一个Channel                |
| changeEvent()       | 修改某个fd上关注的事件                 |
| search()            | 返回某个fd对应的Channel                |
| fillActiveChannel() | 填充活跃的Channel，由Epoll::wait()调用 |
| run()               | 运行整个事件循环                       |
| quit()              | 退出事件循环                           |

##### Epoll

我们这里Epoll默认采用水平触发模式，即某个发生的事件如果没有被处理，它就会一直通知。
这在读事件中不成问题，但在写事件中，如果稍不注意，就会造成busy loop，因此
我们只在需要的时候才关注写事件，不需要的时候立即停止关注。

| 接口     | 功能                                  |
| -------- | ------------------------------------- |
| add()    | 注册一个新的fd                        |
| change() | 修改fd上关注的事件                    |
| del()    | 移除一个fd                            |
| wait()   | 返回活跃的fd个数，并填充活跃的Channel |

##### Channel
| 接口           | 功能                                               |
| -------------- | -------------------------------------------------- |
| fd()           | 返回管理的fd                                       |
| socket()       | 返回一个Socket实例                                 |
| events()       | 返回关注的事件                                     |
| setRevents()   | 由Epoll::wait()调用，更新对应Channel中已发生的事件 |
| isReading()    | 是否关注了读事件                                   |
| isWriting()    | 是否关注了可写事件                                 |
| enableRead()   | 关注可读事件                                       |
| enableWrite()  | 关注可写事件                                       |
| disableWrite() | 停止关注可写事件                                   |
| changeEvent()  | 用当前修改的事件更新Epoll中的事件                  |
| send()         | 发送数据，用户只需调用一次，我们保证发送完数据     |
| handleEvent()  | 事件多路分发器                                     |
| handleRead()   | 读取数据                                           |
| handleWrite()  | 自动被调用，用来写未发送完的数据                   |
| handleClose()  | 关闭一个连接                                       |
| handleError()  | 处理连接错误                                       |
| handleAccept() | 接收一个新的连接                                   |
| setReadCb()    | 设置读回调                                         |
| setMessageCb() | 设置消息回调                                       |

用户不需要关心写事件，我们会自动调用handleWrite()发送send()未发送完的数据。
当有读事件发生时，我们会调用用户注册的消息回调函数，用户在这里处理业务逻辑。

##### Buffer
| 接口        | 功能                                 |
| ----------- | ------------------------------------ |
| begin()     | 返回指向buffer起始位置的指针         |
| peek()      | 返回指向buffer中数据的起始位置的指针 |
| readable()  | 返回buffer中可读的数据量             |
| writeable() | 返回buffer中剩余可写的空间           |
| append()    | 将数据追加到buffer中                 |
| c_str()     | 返回C风格字符串                      |
| makeSpace() | 由append调用，进行内部腾挪           |
| findCrlf()  | 返回buffer中\r\n第一次出现的位置     |
| retrieve()  | 更新buffer，清除掉已读的数据         |
| readFd()    | 从一个fd中读取数据                   |

##### Socket
| 接口           | 功能                       |
| -------------- | -------------------------- |
| fd()           | 返回管理的fd               |
| setFd()        | 设置对应的fd               |
| setPort()      | 设置调用listen时监听的端口 |
| setNonblock()  | 将管理的fd设置为非阻塞     |
| setReuseAddr() | 开启SO_REUSEADDR选项       |
| setNodelay()   | 开启TCP_NODELAY选项        |
| listen()       | 将对应的fd转变为监听套接字 |
| accept()       | 接受一个新连接             |

[点击查看更多实现细节](https://github.com/xiyou-linuxer/deepin-file-control/wiki)

#### 4.4 数据展示

##### 特性描述

#### 4.5 性能及运行需求

7x24 小时运行，可随时重启，能自动恢复服务

### 五、操作手册

#### 1.使用平台

Linux系统 gcc version 7.3(Debian 7.3.0-19)

#### 2.服务端使用概述

（1）打开终端，输入以下命令 （**若没有git，请先安装git**）：

​	 若没有安装过 git ，执行 `sudo apt-get install git` 

​          `git clone https://github.com/xiyou-linuxer/deepin-file-control.git `

（2）打开目录 /deepin-file-control/src/server:

​          `cd deep-file-control/src/server`

（3）执行脚本文件 build.sh

​          `./build.sh`

（4）执行服务器程序

​          `./a.out`

（5）若上一步的执行可执行文件./a.out 的输出为 LogError：

​	  请查看`deepin-file-control/test/.log/.x.log`日志文件来检查错误

#### 3. 客户端使用概述

（1）打开终端，输入以下命令(若没有git，请先安装git)：

​        `git clone https://github.com/xiyou-linuxer/deepin-file-control.git`

（2）打开目录 deepin-file-control:

​        `cd deep-file-control`

（3）在 /deepin-file-control 中 打开 /etc ，在该目录下填写配置文件file.conf 

​       `cd /etc`

​       `gedit file.conf`

​       `在 file.conf 填写配置文件` 

```
#这个是连接的端口
port:8888
#这个是服务器的地址
addr:127.0.0.1
#这是监测的路径,路径请使用绝对路径
path:/home/username/deepin-file-control/etc
```

（注：服务器地址可以根据自身放置服务器的ip地址而改变 ）      

       `按 Ctrl+s 保存 file.conf 文件`

（4）回到 /deepin-file-control:

​        `cd ..`

（5）执行脚本文件

​       `chmod 777 make.sh`

​       `./make.sh`

（6）在 /deepin-file-control 下进入 /test ，查看当前目录的绝对路径，并复制该绝对路径

​       `cd ./test`

​       `pwd `  

​	复制上一步输出后的结果 

​	（参考绝对路径格式应该为 /home/username/deep-file-control/test 

​	    username 根据实际用户替换 仅供参考）

（7）打开要修改系统配置文件

​       `su root`

​       `vim /etc/profile`

（8）在上一步打开的配置文件 /etc/profile 中添加 “export LD_PRELOAD=此处是(6)中复制的绝对路径/myhook.so”，然后保存退出

​      例如（根据(6)中的例子进行设置 ）：

​      `export LD_PRELOAD=/home/username/deep-file-control/test/myhook.so)`

（9）执行 /etc/profile 的脚本命令

​      `source /etc/profile`

（10）返回 /deep-file-control 下的 /test，启动客户端

​      `cd ..`

​      `cd /test`

​      `./client`

（11）若 上一步提示启动成功，则客户端开启成功

#### 4.使用注意事项

（1）服务器程序的编译需要gcc version 7.3(Debian 7.3.0-19)以上的版本，客户端由于编程运用了epoll模型，编译运行的环境一般为linux系统平台。

（2）设置监测目录的时候，不允许将监测目录设置为程序运行的目录，否则会引起不必要的致命错误。

（3）在配置客户端中添加环境变量的时候，需要添加系统级环境变量。

（4）不能在监测目录使用vim命令，否则vim程序将无法返回，只能结束进程。

（5）若某进程不需要在监测系统下，可以执行unset LD_PRELOAD命令取消环境变量。若不使用该客户端，可以vi /etc/profile，将当时安装时候的export LD_PRELOAD语句删除。



### 六、问题反馈联系方式

[ghorges@xiyoulinux.org](mailto:ghorges@xiyoulinux.org)



### 七、源代码地址

https://github.com/xiyou-linuxer/deepin-file-control



[点击查看更多实现细节](https://github.com/xiyou-linuxer/deepin-file-control/wiki)



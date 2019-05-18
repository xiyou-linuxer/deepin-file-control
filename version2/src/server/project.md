## 服务端框架
服务端框架主要由5个类组成。

|类|用途|
|---|---|
|EventLoop|事件循环框架的主体，管理所有注册的Channel|
|Epoll|封装系统底层的I/O多路复用机制，对外提供事件注册、修改、删除的接口|
|Channel|抽象一个连接，用于管理一个连接的数据接收、发送|
|Buffer|一个可读、可写的缓冲区，通常搭配NIO使用|
|Socket|封装了一些基本的socket系列函数|

#### EventLoop
|接口|功能|
|---|---|
|addChannel()|向loop中添加一个新的Channel|
|delChannel()|从loop中移除一个Channel|
|changeEvent()|修改某个fd上关注的事件|
|search()|返回某个fd对应的Channel|
|fillActiveChannel()|填充活跃的Channel，由Epoll::wait()调用|
|run()|运行整个事件循环|
|quit()|退出事件循环|

#### Epoll

我们这里Epoll默认采用水平触发模式，即某个发生的事件如果没有被处理，它就会一直通知。
这在读事件中不成问题，但在写事件中，如果稍不注意，就会造成busy loop，因此
我们只在需要的时候才关注写事件，不需要的时候立即停止关注。

|接口|功能|
|---|---|
|add()|注册一个新的fd|
|change()|修改fd上关注的事件|
|del()|移除一个fd|
|wait()|返回活跃的fd个数，并填充活跃的Channel|

#### Channel
|接口|功能|
|---|---|
|fd()|返回管理的fd|
|socket()|返回一个Socket实例|
|events()|返回关注的事件|
|setRevents()|由Epoll::wait()调用，更新对应Channel中已发生的事件|
|isReading()|是否关注了读事件|
|isWriting()|是否关注了可写事件|
|enableRead()|关注可读事件|
|enableWrite()|关注可写事件|
|disableWrite()|停止关注可写事件|
|changeEvent()|用当前修改的事件更新Epoll中的事件|
|send()|发送数据，用户只需调用一次，我们保证发送完数据|
|handleEvent()|事件多路分发器|
|handleRead()|读取数据|
|handleWrite()|自动被调用，用来写未发送完的数据|
|handleClose()|关闭一个连接|
|handleError()|处理连接错误|
|handleAccept()|接收一个新的连接|
|setReadCb()|设置读回调|
|setMessageCb()|设置消息回调|

用户不需要关心写事件，我们会自动调用handleWrite()发送send()未发送完的数据。
当有读事件发生时，我们会调用用户注册的消息回调函数，用户在这里处理业务逻辑。

#### Buffer
|接口|功能|
|---|---|
|begin()|返回指向buffer起始位置的指针|
|peek()|返回指向buffer中数据的起始位置的指针|
|readable()|返回buffer中可读的数据量|
|writeable()|返回buffer中剩余可写的空间|
|append()|将数据追加到buffer中|
|c_str()|返回C风格字符串|
|makeSpace()|由append调用，进行内部腾挪|
|findCrlf()|返回buffer中\r\n第一次出现的位置|
|retrieve()|更新buffer，清除掉已读的数据|
|readFd()|从一个fd中读取数据|

#### Socket
|接口|功能|
|---|---|
|fd()|返回管理的fd|
|setFd()|设置对应的fd|
|setPort()|设置调用listen时监听的端口|
|setNonblock()|将管理的fd设置为非阻塞|
|setReuseAddr()|开启SO_REUSEADDR选项|
|setNodelay()|开启TCP_NODELAY选项|
|listen()|将对应的fd转变为监听套接字|
|accept()|接受一个新连接|

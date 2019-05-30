
#pragma once
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <map>
#include<string>
#include<arpa/inet.h>
#include <dlfcn.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include<arpa/inet.h>
#include <net/if.h>
#include <vector>

class Monitored_event{
public:
    /*Unix套接字读缓冲区有三种状态:OPEN请求,CLOSE请求,重复打开同一个文件*/
    enum Request_State{OPEN_SAVE, CLOSE_GET, REPEAT_FILE};
    /*解析有两种状态，一种是解析头部，另一种是还有文件内容*/
    enum Parse_State{HEAD, CONTENT};
public:
    static const int READ_BUF_SIZE = 2048;
    static const int WRITE_BUF_SIZE = 1024;
    static map<string, int> repeat_path;//查看是否是重复文件
    static int SERVER_STATUS;
private:
    static int epfd;//所有被监测的事件共同使用一个epoll注册事件
    static int i_socketfd;//所有被监测的事件共同使用一个远程连接
    int u_socketfd;//Unix套接字
    static int Monitored_number;//所有被监测事件的个数

    Parse_State p_state;//解析头部和内容,状态转移标志
    const char *mac_addr;
    char *line_buf;//读取到的每一行的头指针
    int now_index;//当前解析了多少字节
    long int file_length;//文件的大小
    string file_name;//文件名称
    //static map<string, int> repeat_path;//查看是否是重复文件
public:
    /*由于之后用的是类数组形式,初始化类成员统一在init成员函数中进行*/
    Monitored_event(){}
    ~Monitored_event(){}

public:
    /*被监测事件含参构造并初始化*/
    void init(int ed, int i_s, int u_s);

    /*关闭连接(考虑之中，因为不能关闭网络套接字和Unix套接字)*/
    void close_monitored();

    /*分析被监测事件的类型,线程池轮询事件队列的接口*/
    void do_process();

    /*Unix写到hook.c进程函数*/
    bool u_write(){
        int r=send(u_socketfd,"OPEN_CALL_OK\r\n",strlen("OPEN_CALL_OK"),0);
        rmfd(epfd,u_socketfd);
        repeat_path.erase(file_name);
        Monitored_modfd(epfd, u_socketfd, EPOLLIN);
        Monitored_modfd(epfd, i_socketfd, EPOLLIN);
        return true;
    }

    /*与远端服务器连接写函数*/
    bool i_write();

    /**!!!划重点！由于用的是ET非阻塞模式，所以读取的时候一定要保证读到EAGAIN为止**/
    /*Unix读取hook.c进程发送包函数*/
    bool u_read();
    /*与远端服务器连接的函数*/
    bool i_read(){
        int r=send(u_socketfd,"OPEN_CALL_OK\r\n",strlen("OPEN_CALL_OK"),0);
        //Monitored_modfd(epfd, u_socketfd, EPOLLIN);
        return true;}

private:
    /*unix套接字的读取缓冲区*/
    char unix_read_buf[READ_BUF_SIZE];

    /*TCP套接字读取缓冲区,即服务器应答缓冲区*/
    char server_read_buf[READ_BUF_SIZE];

    /*unix套接字的发送缓冲区*/
    char unix_write_buf[READ_BUF_SIZE];

    /*TCP套接字发送缓冲区*/
    char server_write_buf[READ_BUF_SIZE];

private:
    /*因为是EPOLLNESHOT,所以每次要修改epoll事件表*/
    void Monitored_modfd(int epfd, int fd, int ev);

    /*发送文件函数*/
    int send_n(int fd,char *buffer,int n);

    /*获取每行并且解析*/
    bool get_line(const char *test_buf){return true;}

    /*通过解析Unix套接字的读缓冲区,判断是open调用还是close调用被劫持*/
    Request_State parse_read_buf();

    /*填写向服务器发送的写缓冲区,根据请求类型进行填写响应包*/
    void fill_swrite_buf(Request_State state);
    /*填写Unix的发送缓冲区,根据请求填写响应包*/
    void fill_uwrite_buf(Request_State state);
    //private:

};


int setnonblocking(int fd);
void addfd(int epfd, int fd, bool flag);
void rmfd(int epfd, int fd);
void modfd(int epfd, int fd, int ev);
void tcp_read(int epfd,int i_socketfd);



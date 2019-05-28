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
#include<arpa/inet.h>
#include <dlfcn.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include<arpa/inet.h>
#include <net/if.h>
#include <vector>
#include "sendn.hpp"
#include "threadpool.h"
using namespace std;

static int UNIX;
void rmfd(int epfd, int fd);


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
    
    friend void tcp_read(int epfd,int i_socketfd);
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
    /*接收TCP套接字响应*/
    int recv_n(int fd,char *buffer,int flag,int _size);
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
map<string, int> Monitored_event::repeat_path = map<string, int>();//map初始化
int Monitored_event::epfd=-1;//所有被监测的事件共同使用一个epoll注册事件
int Monitored_event::i_socketfd=-1;//所有被监测的事件共同使用一个远程连接
int Monitored_event:: Monitored_number=0;//所有被监测事件的个数
int Monitored_event::SERVER_STATUS=0;
/*REPEAT 处理*/
void Monitored_event::fill_uwrite_buf(Request_State state)
{
    int t = send(u_socketfd,"OPEN_SAVE_FAILT",strlen("OPEN_SAVE_FAILT"),0);
    if (t < 0) {
        perror("send err:");
        pthread_exit(0);
    }
}

int Monitored_event::send_n(int fd,char *buffer,int n)
{
    int num = 0;
    while ( num < n ) {
        int t = send( fd,&buffer[num],n - num,0 );
        if ( t < 0 ) {
            if( errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK ) {
                cout << "send_n send err" << endl;
                continue;
            }
            else {
                perror("send err in send_n:");
                break;
            }
        }
        else if ( t == 0 ) {
            return -1;
        } else {
            num += t;
        }
    }
    return num;
}



void Monitored_event :: init(int ed, int i_s, int u_s)
{
    epfd = ed;
    i_socketfd = i_s;
    u_socketfd = u_s;
    /*以下包括其他类成员的初始化*/
    ++Monitored_number;
}

void Monitored_event::close_monitored()
{
    /**/
}

/*改变epoll事件类型*/
void Monitored_event:: Monitored_modfd(int epfd, int fd, int ev)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = ev|EPOLLET|EPOLLONESHOT|EPOLLRDHUP;
    epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&event);
}

/*填写向服务器发送的写缓冲区,根据请求类型进行填写响应包*/
void Monitored_event::fill_swrite_buf(Request_State state){
    switch(state)
    {
    case OPEN_SAVE:
    {

        mac_addr=get_mac();
        struct stat buf_stat;
        bzero(&buf_stat,sizeof(buf_stat));
        stat(file_name.c_str(),&buf_stat);
        file_length = buf_stat.st_size;
        sprintf(unix_write_buf,"SAVE %s %s\r\nfilesize: %ld\r\n\r\n",file_name.c_str(),mac_addr,file_length);
        unix_write_buf[strlen(unix_write_buf)+1] = '\0';
        break;
    }
    case CLOSE_GET:
    {
        mac_addr = get_mac();
        sprintf(unix_write_buf,"GET %s %s\r\nfilesize: 0\r\n\r\n", file_name.c_str(), mac_addr);
        unix_write_buf[strlen(unix_write_buf)+1] = '\0';
        break;
    }
    }
    cout << "%%%%:" << unix_write_buf << endl;
}

/*TCP套接字发送*/
bool Monitored_event::i_write()
{
    /*打开动态链接库*/
    static void *handle = NULL;
    static OPEN old_open = NULL;
    static CLOSE old_close = NULL;
    if(!handle)
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_open = (OPEN)dlsym(handle, "open");
        dlclose(handle);
    }
    handle = NULL;
    if(!handle)
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_close = (CLOSE)dlsym(handle, "close");
        dlclose(handle);
    }

    int file_fd = old_open(file_name.c_str(),O_RDWR);
    if (file_fd < 0) {
        perror("open file.conf err:");
        pthread_exit(0);
    }

    cout << "file_length = " << file_length << endl;
    char *send_buffer = (char*)mmap(NULL,file_length,PROT_READ | PROT_WRITE, MAP_SHARED,file_fd, 0);


    //这个是发送文件
    int r = send_n(i_socketfd,send_buffer,file_length);
    if(r < 0) {
        cout << "send_n err" << endl;
        return false;
    }
    munmap(send_buffer,file_length);
    ftruncate(file_fd,strlen("It is a secret"));
    write(file_fd,"It is a secret",strlen("It is a secret"));
    old_close(file_fd);

    cout << "unix_write_buf&&: " << unix_write_buf << endl;
    return true;

}

/*只获取协议包的包头*/
bool Monitored_event :: u_read()
{
    int flag = 1;
    int k=0;
    char ch[1];
    while(read(u_socketfd, ch, 1) > 0)
    {
        unix_read_buf[k] = ch[0];
        if(k>4 && unix_read_buf[k]=='\n' && unix_read_buf[k-1]=='\r' && unix_read_buf[k-2]=='\n' && unix_read_buf[k-3]=='\r')
        {
            flag = 1;
            break;
        }
        k++;
    }
    unix_read_buf[strlen(unix_read_buf)+1] = '\0';
    cout<<"mayicheng: " << unix_read_buf << endl;
    if(flag)
    {
        return true;
    }
    else{
        return false;
    }
}

/*解析读取缓冲区的内容,只解析头部*/
Monitored_event::Request_State Monitored_event::parse_read_buf()
{
    string read_package(unix_read_buf);
    string type;
    /*第一个空格分割*/
    int first = read_package.find(' ');
    
    /*OPEN调用还是CLOSE*/
    type = read_package.substr(0, first);
    /*截取文件的绝对路径，不包括\r\n*/
    file_name = read_package.substr(first+1,read_package.size()-first-3);
    
    /*查看文件是否是重复文件*/
    if(repeat_path.find(file_name) == repeat_path.end())
    {
        repeat_path.insert(pair<string ,int>(file_name, u_socketfd));
        if(type=="OPEN")
        {
            if(Monitored_event::SERVER_STATUS)
            {
                int ret;
                ret = write(u_socketfd,"FORBIDDEN",10);
                assert(ret>0);
            }
            return OPEN_SAVE;
        }
        else if(type=="CLOS")
        {
            return CLOSE_GET;
        }
    }
    else{
        if(type=="CLOS")
            return CLOSE_GET;
    }
    return REPEAT_FILE;
}


/*Unix事件的线程池接口函数*/
void Monitored_event::do_process()
{
    /*解析进程发送的包,OPEN相当于向服务器申请备份,CLOSE属于向服务器申请取备份*/
    Request_State ret = parse_read_buf();
    /*如果为重复open，需要直接给Unix套接字返回信息,不向服务器发送信息*/
    if(ret == REPEAT_FILE)
    {
        /*填写Unix套接字响应请求类型*/
        fill_uwrite_buf(ret);
    }

    /*如果不重复,根据是OPEN请求还是CLOSE请求发送给服务器*/
    else{
        fill_swrite_buf(ret);
        UNIX = u_socketfd;
        int r = send_n(i_socketfd,unix_write_buf,strlen(unix_write_buf));
        Monitored_modfd(epfd, i_socketfd, EPOLLOUT);
    }
}

/*设置为非阻塞模式*/
int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

/*增加epoll事件*/
void addfd(int epfd, int fd, bool flag)
{
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    if(flag)
    {
        ev.events = ev.events | EPOLLONESHOT;
    }
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    setnonblocking(fd);
}
void rmfd(int epfd, int fd)
{
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, 0);
}
/*改变epoll事件*/
void modfd(int epfd, int fd, int ev)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = ev|EPOLLET|EPOLLONESHOT|EPOLLRDHUP;
    epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&event);
}


void tcp_read(int epfd,int i_socketfd)
{
    char read[100];
    bzero(read,sizeof(read));
    if ( recv_n(i_socketfd,read,0,0) < 0)
    {
        cout << "recv_n err:" << endl;

    }
    cout << "recve of server = " << read << endl;


    if ( strncmp("SAVE",read,strlen("SAVE")) == 0 )
    {
        char read1[100];
        bzero(read1,sizeof(read1));
        if ( recv_n(i_socketfd,read1,0,0) < 0)
        {
            cout << "recv_n err:" << endl;

        }
        cout << "recve of server = " << read1 << endl;


        int _space = 0;
        for(int i = 0;i < strlen(read);i++)
        {
            if(read[i] == '\r') {
                read[i] = (char)0;
            }
            if(read[i] == '\n') {
                read[i] = (char)0;
            }
            if(read[i] == ' ') {
                _space = i + 1;
            }
        }
        string read_t(&read[_space]);
        cout << read_t << endl;

        map<string, int>::iterator it = Monitored_event::repeat_path.find(read_t);
        if (it == Monitored_event::repeat_path.end())
        {
            cout << "repeat_path no" << endl;

        }
        Monitored_event::repeat_path.erase(it);
        modfd(epfd, it->second, EPOLLOUT);
    }
    else if ( strncmp("GET",read,strlen("GET")) == 0 ){
        char *b = read;
        cout << "进入GET" << endl;
        int spaces = 0;
        for(spaces = 0;b[spaces] != ' ';spaces++);

        for(int i = 0;i < strlen(b);i++) {
            if (b[i] == '\r' || b[i] == '\n') {
                b[i] = char(0);
            }
        }

        spaces++;
        cout << "close path = " << &b[spaces]  << endl;
        string read_t(&b[spaces]);
        cout << read_t << endl;

        //获取old_open
        static void *handle = NULL;
        static OPEN old_open = NULL;
        static CLOSE old_close = NULL;
        if(!handle)
        {
            handle = dlopen("libc.so.6", RTLD_LAZY);
            old_open = (OPEN)dlsym(handle, "open");
            dlclose(handle);
        }
        handle = NULL;
        if(!handle)
        {
            handle = dlopen("libc.so.6", RTLD_LAZY);
            old_close = (CLOSE)dlsym(handle, "close");
            dlclose(handle);
        }

        int close_fd = old_open(&b[strlen("GET-STATUS") + 1],O_RDWR);
        if (close_fd < 0) {
            perror("710 close_fd err:");
        }

        char line2[200] = { 0 };
        bzero(line2,0);
        recv_n(i_socketfd,line2,0,0);
        int close_filesize = atoi(&line2[strlen("filesize: ")]);
        cout << "文件大小为:" << close_filesize << endl;
        if(close_filesize == 0) {
            ftruncate(close_fd, close_filesize);
            old_close(close_fd);

        }
        ftruncate(close_fd, close_filesize);
        char *send_buffer = (char*)mmap(NULL,close_filesize,PROT_READ | PROT_WRITE, MAP_SHARED,close_fd, 0);

        recv_n(i_socketfd,send_buffer,1,close_filesize);
        munmap(send_buffer,close_filesize);
        old_close(close_fd);

        cout << "old_close ok!" << endl;
        cout << read_t << endl;
        map<string, int>::iterator it = Monitored_event::repeat_path.find(read_t);

        cout << it->second << endl;
        modfd(epfd, it->second, EPOLLOUT);
    }
}

int main()
{
    /*创建线程池指针*/
    threadpool< Monitored_event >* monitored_pool = NULL;
    monitored_pool = new threadpool<Monitored_event>;
    Monitored_event my_monitored_event[1000];
    /*创建Unix套接字,绑定Unix套接字并且建立监听套接字*/
    struct sockaddr_un myserver;
    int u_socketfd;
    u_socketfd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if(u_socketfd < 0)
    {
        cout << "socket is failt!\n";
        return 0;
    }

    get_etcs *GET_ETC = get_etcs::get_init();
    myserver.sun_family = AF_LOCAL;//设置为本地协议
    strcpy(myserver.sun_path, GET_ETC->UNIXSOCKPATH);
    int r = unlink(GET_ETC->UNIXSOCKPATH);
    if(r<0)
    {
        cout << "unlink is wrong!\n";
        return 0;
    }
    bind(u_socketfd,(struct sockaddr*)&myserver,sizeof(myserver));
    int ret = listen(u_socketfd,5);
    if(ret < 0)
    {
        cout << "listen is failt!\n";
        return 0;
    }

    /*创建TCP网络套接字并且绑定TCP网络套接字，并且与服务器建立连接*/
    struct sockaddr_in myclient;
    bzero(&myclient,sizeof(myclient));
    int i_socketfd;
    i_socketfd = socket(AF_INET, SOCK_STREAM, 0);
    setnonblocking(i_socketfd);
    if(i_socketfd < 0 )
    {
        cout << "i_socketfd is failt\n";
        return 0;
    }
    myclient.sin_family = AF_INET;
    cout << GET_ETC->ETC_ADDR << endl;
    myclient.sin_port = htons(GET_ETC->ETC_PORT);
    inet_pton(AF_INET, GET_ETC->ETC_ADDR, (void *)&myclient.sin_addr);
    ret  = connect(i_socketfd,(struct sockaddr*)&myclient,sizeof(myclient));
    if(ret < 0 && errno!=EINPROGRESS)
    {
        cout << "connect is failt\n";
        return 0;
    }
    cout << "TCP请求链接成功\n";
    cout <<"u_socketfd: "<<u_socketfd <<" i_socketfd: " <<i_socketfd<<endl;

    /*创建epoll*/
    int epfd;
    epoll_event events[1000];
    epfd = epoll_create(5);
    assert(epfd != -1);

    /*向epoll中注册Unix本地套接字和网络套接字*/
    addfd(epfd, u_socketfd, false);//进程之间的通信u_socketfd作为服务端,需要处理多个进程连接,不能为EPOLLNESHOT事件
    addfd(epfd, i_socketfd, true);//网络通信作为客户端,只能有一个线程占有，所以为EPOLLNESHOT

    while(true)
    {
        int timeout = -1;
        int number = epoll_wait(epfd, events, 1000, timeout);
        if( (number < 0) && (errno != EINTR) )
        {
            printf("my epoll is failure!\n");
            break;
        }
        for(int i=0; i<number; i++)
        {
            int now_sockfd = events[i].data.fd;

            /*本地进程有新连接*/
            if(now_sockfd == u_socketfd)
            {
                struct sockaddr_un u_client_address;
                socklen_t client_addresslength = sizeof(u_client_address);
                int client_fd = accept(now_sockfd,(struct sockaddr*)&u_client_address, &client_addresslength);
                if(client_fd < 0)
                {
                    printf("errno is %d\n",errno);
                    continue;
                }
                cout << "接受连接成功\n";
                cout << "U_SOCKETFFD: " << client_fd << endl;
                /*向epoll中注册新的进程事件*/
                my_monitored_event[client_fd].init(epfd,i_socketfd,client_fd);
                addfd(epfd, client_fd, false);
            }

            /*本地进程客户端断开连接或者远端服务器进程异常断开*/
            else if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                /*若服务器断开链接，则监测系统不允许打开任何文件*/
                if(now_sockfd == i_socketfd)//此处服务端不可到达,需要让本地被监测的open调用都失败
                {
                    Monitored_event::SERVER_STATUS = 1;
                    rmfd(epfd,i_socketfd);
                    //timeout = 3000;
                    /*重新连接的接口*/
                    cout << "i_socketfd is close\n";
                }
                else{
                    /*出现异常，事件自动关闭套接字*/
                    close(now_sockfd);
                    cout << "出现异常，关闭进程连接\n";
                }
            }

            /*epoll事件表中有u_socketfd读取事件，或者是i_socketfd有读取时间*/
            else if(events[i].events & EPOLLIN)//可以读取
            {
                if(now_sockfd == i_socketfd)//i_socketfd网络套接字可读取,处理服务端返回的信息
                {
                    tcp_read(epfd,i_socketfd);
                    /*char read[100];
                    bzero(read,sizeof(read));
                    if ( recv_n(i_socketfd,read,0,0) < 0)
                    {
                        cout << "recv_n err:" << endl;
                        continue;
                    }
                    cout << "recve of server = " << read << endl;


                    if ( strncmp("SAVE",read,strlen("SAVE")) == 0 )
                    {
                        char read1[100];
                        bzero(read1,sizeof(read1));
                        if ( recv_n(i_socketfd,read1,0,0) < 0)
                        {
                            cout << "recv_n err:" << endl;
                            continue;
                        }
                        cout << "recve of server = " << read1 << endl;


                        int _space = 0;
                        for(int i = 0;i < strlen(read);i++)
                        {
                            if(read[i] == '\r') {
                                read[i] = (char)0;
                            }
                            if(read[i] == '\n') {
                                read[i] = (char)0;
                            }
                            if(read[i] == ' ') {
                                _space = i + 1;
                            }
                        }
                        string read_t(&read[_space]);
                        cout << read_t << endl;

                        map<string, int>::iterator it = Monitored_event::repeat_path.find(read_t);
                        if (it == Monitored_event::repeat_path.end())
                        {
                            cout << "repeat_path no" << endl;
                            continue;
                        }
                        Monitored_event::repeat_path.erase(it);
                        modfd(epfd, it->second, EPOLLOUT);
                    }
                    else if ( strncmp("GET",read,strlen("GET")) == 0 ){
                        char *b = read;
                        cout << "进入GET" << endl;
                        int spaces = 0;
                        for(spaces = 0;b[spaces] != ' ';spaces++);

                        for(int i = 0;i < strlen(b);i++) {
                            if (b[i] == '\r' || b[i] == '\n') {
                                b[i] = char(0);
                            }
                        }

                        spaces++;
                        cout << "close path = " << &b[spaces]  << endl;
                        string read_t(&b[spaces]);
                        cout << read_t << endl;

                        //获取old_open
                        static void *handle = NULL;
                        static OPEN old_open = NULL;
                        static CLOSE old_close = NULL;
                        if(!handle)
                        {
                            handle = dlopen("libc.so.6", RTLD_LAZY);
                            old_open = (OPEN)dlsym(handle, "open");
                            dlclose(handle);
                        }
                        handle = NULL;
                        if(!handle)
                        {
                            handle = dlopen("libc.so.6", RTLD_LAZY);
                            old_close = (CLOSE)dlsym(handle, "close");
                            dlclose(handle);
                        }

                        int close_fd = old_open(&b[strlen("GET-STATUS") + 1],O_RDWR);
                        if (close_fd < 0) {
                            perror("710 close_fd err:");
                        }

                        char line2[200] = { 0 };
                        bzero(line2,0);
                        recv_n(i_socketfd,line2,0,0);
                        int close_filesize = atoi(&line2[strlen("filesize: ")]);
                        cout << "文件大小为:" << close_filesize << endl;
                        if(close_filesize == 0) {
                            ftruncate(close_fd, close_filesize);
                            old_close(close_fd);
                            continue;
                        }
                        ftruncate(close_fd, close_filesize);
                        char *send_buffer = (char*)mmap(NULL,close_filesize,PROT_READ | PROT_WRITE, MAP_SHARED,close_fd, 0);

                        recv_n(i_socketfd,send_buffer,1,close_filesize);
                        munmap(send_buffer,close_filesize);
                        old_close(close_fd);

                        cout << "old_close ok!" << endl;
                        cout << read_t << endl;
                        map<string, int>::iterator it = Monitored_event::repeat_path.find(read_t);

                        cout << it->second << endl;
                        modfd(epfd, it->second, EPOLLOUT);
                    }*/
                }
                /*u_socketfd套接字有可读事件,并且读入将内容读入缓冲区*/
                else{
                    if( my_monitored_event[now_sockfd].u_read() ) //读取成功,加入任务列表
                    {
                        monitored_pool->addjob(my_monitored_event+now_sockfd);
                    }
                }

            }
            /*epoll注册表中，有可写入事件*/
            else if(events[i].events & EPOLLOUT)
            {
                /*向服务器发送请求，备份或者取备份*/
                if(now_sockfd == i_socketfd)
                {
                    cout << "i_socketfd is write\n";
                    /*写入服务器成功*/
                    if(my_monitored_event[UNIX].i_write())
                    {
                        cout << "send is successful! UNIX: : " << UNIX << endl;
                        modfd(epfd, i_socketfd, EPOLLIN);
                    }
                }
                /*向被监控进程发送返回状态*/
                else{
                    /*向被监控进程返回状态成功*/
                    if(my_monitored_event[now_sockfd].u_write())
                    {
                        cout << "检测成功\n";
                    }
                }

            }
        }

    }
    close(epfd);
    return 0;
}


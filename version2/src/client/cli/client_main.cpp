#include"client.hpp"
#include"threadpool.h"
#include"sendn.hpp"
#include"etc.hpp"
using namespace std;


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


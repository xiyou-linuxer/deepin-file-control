/*#include <iostream>
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
#include <vector>*/

#include"client.hpp"

using namespace std;

static int UNIX;


bool Monitored_event::u_write(){
    int r=send(u_socketfd,"OPEN_CALL_OK\r\n",strlen("OPEN_CALL_OK"),0);
    rmfd(epfd,u_socketfd);
    repeat_path.erase(file_name);
    Monitored_modfd(epfd, u_socketfd, EPOLLIN);
    Monitored_modfd(epfd, i_socketfd, EPOLLIN);
    return true;
}

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
int recv_n(int fd,char *buffer,int flag,int _size)
{
    if(flag == 0) {
        while (1) {
            char b[100];
            bzero(b,sizeof(b));
            int n = recv(fd,(void *)b,sizeof(b),MSG_PEEK);
            if ( n < 0) {
                perror("recv err in recv_n:");
            } else if (n == 0) {
                perror("close server in recv_n:");
                return -1;
            } else {
                int i = 0;
                for(i = 0;i < n;i++)
                {
                    if (b[i] == '\n') {
                        break;
                    }
                }
                //如果b[i] != '\n',说明读的少
                if (i == n || b[i] != '\n' || b[i + 1] == 0) {
                    continue;
                }

                if (b[i + 1] != '\r') {      //如果不是\r,说明可以取了
                    return recv_n(fd,buffer,1,i + 1);
                } else if (b[i + 1] == '\r' &&  b[i + 2] == '\n') {
                    return recv_n(fd,buffer,1,i + 3);
                }
                else {
                    continue;
                }
            }
        }
    }
    /* 否则就不需要预读了 */
    else if (flag == 1) {
        int sum = 0;
        while (sum < _size) {
            int m = recv(fd,&buffer[sum],_size - sum,0);
            if(m < 0) {
                continue;
            } else if(m == 0) {
                perror("server close:");
                return -1;
            } else {
                sum += m;
            }
        }
        cout << "buffer = " << buffer << "    _size = " << _size << endl;
        return 0;
    }
    /* 如果flag错误 */
    else {
        cout << "flags err:" << endl;
        return -1;
    }
}



char* get_mac()
{
    char *c = new char[200];

    struct   ifreq   ifreq;
    int   sock;

    if((sock=socket(AF_INET,SOCK_STREAM,0)) <0)
    {
        perror( "socket ");
        return NULL;
    }
    const char *etho = "wlp2s0";
    strcpy(ifreq.ifr_name,etho);
    if(ioctl(sock,SIOCGIFHWADDR,&ifreq) <0)
    {
        perror( "ioctl ");
        return NULL;
    }
    sprintf(c, "%02x:%02x:%02x:%02x:%02x:%02x\n ",
            (unsigned   char)ifreq.ifr_hwaddr.sa_data[0],
            (unsigned   char)ifreq.ifr_hwaddr.sa_data[1],
            (unsigned   char)ifreq.ifr_hwaddr.sa_data[2],
            (unsigned   char)ifreq.ifr_hwaddr.sa_data[3],
            (unsigned   char)ifreq.ifr_hwaddr.sa_data[4],
            (unsigned   char)ifreq.ifr_hwaddr.sa_data[5]);
    return c;
}

char get_etcs::PATH[200];
char get_etcs::UNIXSOCKPATH[200];
int get_etcs::ETC_PORT = 0;
char get_etcs::ETC_ADDR[200];

get_etcs* get_etcs::instance = new get_etcs();

get_etcs::get_etcs(const get_etcs&){

}


get_etcs& get_etcs::operator=(const get_etcs&){

}


get_etcs::get_etcs()
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

    //打开配置文件
    int fd = old_open(FILE_PATH,O_RDWR);
    if (fd < 0) {
        perror("open file.conf err:");
        exit(0);
    }

    //读取配置文件
    char c[200];
    bzero(c,sizeof(c));
    read(fd,c,sizeof(c));

    const int j = strlen(c);

    for (int i = 0;i < j;i++) {
        if(c[i] == '\n')
            c[i] = 0;
    }
    for (int i = 0;i < j - 1;i++) {
        if (strncmp(&c[i],"path:",5) == 0) {
            strcpy(PATH,&c[i + 5]);
            continue;
        }
        if (strncmp(&c[i],"unix:",5) == 0) {
            strcpy(UNIXSOCKPATH,&c[i + 5]);
            continue;
        }
        if (strncmp(&c[i],"addr:",5) == 0) {
            strcpy(ETC_ADDR,&c[i + 5]);
            continue;
        }
        if (strncmp(&c[i],"port:",5) == 0) {
            ETC_PORT = atoi(&c[i + 5]);
            continue;
        }
    }
    //关闭配置文件
    int t = old_close(fd);
    if(t < 0)
    {
        perror("close t err:");
        exit(0);
    }
}


get_etcs* get_etcs::get_init()
{

    if (instance == NULL)
    {
        pthread_mutex_lock(&mutex_class);
        if (instance == NULL)
        {
            instance = new get_etcs();
        }
        pthread_mutex_unlock(&mutex_class);
    }
    return instance;
}



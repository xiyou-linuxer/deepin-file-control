#include"client.hpp"
#include"sendn.hpp"
/*REPEAT 处理*/

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

map<string, int> Monitored_event::repeat_path = map<string, int>();//map初始化
int Monitored_event::epfd=-1;//所有被监测的事件共同使用一个epoll注册事件
int Monitored_event::i_socketfd=-1;//所有被监测的事件共同使用一个远程连接
int Monitored_event:: Monitored_number=0;//所有被监测事件的个数
int Monitored_event::SERVER_STATUS=0;//是否与服务器断开连接标志

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

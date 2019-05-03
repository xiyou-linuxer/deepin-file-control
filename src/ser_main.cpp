#include <iostream>
#include <iostream>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

using namespace std;

//发送的是定长包
typedef struct send_file
{
    int n;
    char pathname[100];
    char c[1000];
}send_file;

//这个结构体是用来给线程传参数的时候,給线程的一些信息
struct cli_data
{
    struct in_addr cli_addr;
    int fd;
};

void get_backup(int fd,send_file f,char *file_addr);

ssize_t writen(int fd,const void *ptr,size_t n);

void handle(int t)
{
}
void file_backup(send_file f,char *file_addr,int *fd)
{
    cout << "f.c = " << f.c << endl;
    cout << "f.path = " << f.pathname << endl;
    //这样既把fd存起来了,又少了一次判断,一石二鸟
    if (*fd == 0) {
        //首先建立多级目录! 如果建立过,就不用多次建立了!!!
        string t("../backupfile/");
        t += file_addr;
        t += f.pathname;
        cout << t << endl;

        string::iterator t2 = t.begin() + 3;
        while (t2 <= t.end()) {
            for (;t2 != t.end();t2++) {
                if(*t2 == '/')
                    break;
            }
            if (t2 == t.end())
                break;
            char c[100];
            bzero(c,sizeof(c));
            strncpy(c,t.c_str(),t2 - t.begin());
            cout << c << endl;
            mkdir(c,0777);
            t2++;
        }

        //then打开文件,并写入信息
        *fd = open(t.c_str(),O_RDWR | O_CREAT,0666);
        if (*fd < 0) {
            perror("open err:");
            pthread_exit(0);
        }
    }

    /*
    //这个就是最后发的那个空包,可以用来判断!!
    if (f.n == 0) {
        send(*fd,"1",strlen("1"),0);
    }
    */
    //一直写,直到完成
    write(*fd,f.c,f.n);
}


void *read_file(void *p)
{
    int flag_file = 0;
    //脱离主线程控制
    if (pthread_detach(pthread_self()) < 0) {
        perror("pthread_detach err:");
        pthread_exit(0);
    }

    cli_data *data = (cli_data*)p;
    int fd = data->fd;

    /*
    struct in_addr cli_addr;
    bzero(&cli_addr,sizeof(cli_addr));
    memcpy(&cli_addr, &data->cli_addr, 4);
    */
    //这个是path路径
    char file_path[100];
    bzero(file_path,sizeof(file_path));
    strcpy(file_path,inet_ntoa(data->cli_addr));
    printf("%s\n", inet_ntoa(data->cli_addr));


    int sum_data = 0;
    //这个要保证每次while循环读到一个buf
    while (1) {
        int t = 0;
        send_file buf;
        memset(&buf,0,sizeof(buf));
        while (t != sizeof(buf)) {
            int n = recv(fd,(void *)&buf,sizeof(buf) - t,(int)0);
            if (n == 0) {
                cout << "连接断开" << endl;
                return NULL;
            }
            if (n > 0) {
                t += n;
                continue;
            }
            if (n < 0) {
                if (errno == EINTR)
                    continue;
                cout << "recv err" << endl;
                return NULL;
            }
        }
        //为了和open统一,close只能这么写
        if (buf.n == -777) {
            get_backup(fd,buf,file_path);
            return NULL;
        }

        //后加的,为了文件截断
        sum_data += buf.n;
        if (buf.n == 0) {
            if(flag_file)
                ftruncate(flag_file,sum_data);
        }
        file_backup(buf,file_path,&flag_file);
    }

}
int main(int argc,char **argv)
{
    if(argc != 2)
    {
        printf("格式:运行的文件 端口\n");
        exit(0);
    }
    //signal(SIGINT,handle);
    int sock = socket(AF_INET,SOCK_STREAM,0);
    if (sock < 0) {
        perror("get sockerr:");
        return -1;
    }
    int yes = 1;
    if(setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes)) < 0)
    {
        perror("setsockopt err:");
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;

    int a = atoi(argv[1]);
    short b = a;
    printf("port = %d\n",b);
    addr.sin_port = htons(b);
    addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    if(bind(sock,(struct sockaddr *)&addr,sizeof(addr)) < 0)
    {
        perror("bind err:");
        return -1;
    }

    if(listen(sock,100) < 0)
    {
        perror("listen err:");
        return -1;
    }


    //目前写的服务器这边,是每有一个连接,建立一个线程
    while(1)
    {
        struct sockaddr_in addr;
        socklen_t len  = sizeof(addr);
        bzero(&addr,sizeof(addr));
        int accfd = accept(sock,(struct sockaddr *)&addr,&len);
        if (accfd < 0) {
            perror("accept err:");
            exit(0);
        }
        pthread_t t1;

        cli_data * c_data = new cli_data;
        bzero(c_data,sizeof(cli_data));
        memcpy(&c_data->cli_addr, &addr.sin_addr.s_addr, 4);
        //c_data->cli_addr = addr.sin_addr.s_addr;
        c_data->fd = accfd;
        if (pthread_create(&t1,NULL,read_file,(void *)c_data) < 0) {
            perror("pthread_create err:");
            exit(0);
        }

    }
    std::cout << "Hello world" << std::endl;
    return 0;
}

void get_backup(int fd,send_file f,char *file_addr)
{
    string t("../backupfile/");
    t += file_addr;
    t += f.pathname;
    cout << t << endl;

    int fd_file = open(t.c_str(),O_RDWR,0666);
    if (fd_file < 0) {
        perror("open err:");
        pthread_exit(0);
    }


    cout << "这里说明已经要close文件了" << endl;


    //struct 结构体用来发送file文件
    //定长
    typedef struct send_file
    {
        int n;
        char pathname[100];
        char c[1000];
    }send_file;
    send_file f1;
    bzero(&f1,sizeof(f1));
    printf("read_path = %s\n",f.pathname);
    memcpy(f1.pathname,f.pathname,sizeof(f1.pathname));

    struct stat ttt;
    bzero(&ttt,sizeof(ttt));
    fstat(fd_file,&ttt);
    int ssum = ttt.st_size;
    int rd_sum = 0;

    while (rd_sum <= ssum) {
        //排除为0的情况,而且最后把0包发送了
        if (rd_sum >= ssum) {
            f1.n = 0;
            bzero(f1.c,1000);
            write(fd,&f1,sizeof(send_file));
            break;
        }
        f1.n = 0;
        bzero(f1.c,1000);

        //读文件
        f1.n = read(fd_file,f1.c,1000);
        if (f1.n == 0) {
            write(fd,&f1,sizeof(send_file));
            printf("文件读取完成!");
            break;
        }
        if (f1.n < 0) {
            if (errno == EINTR)
                continue;
            perror("send_mes read err:");
            pthread_exit(0);
        }
        rd_sum += f1.n;
        int t = writen(fd,&f1,sizeof(send_file));
        if(t == 0)
        {
            cout << "客户端关闭" << endl;
            pthread_exit(0);
        }
        if(t < 0)
        {
            if (errno == EINTR)
                continue;
            printf("write err\n");
            pthread_exit(0);
        }
    }
}

ssize_t writen(int fd,const void *ptr,size_t n)
{
    size_t nleft;
    ssize_t nwritten;

    nleft = n;
    while (nleft > 0) {
        if ((nwritten = write(fd,ptr,nleft)) < 0) {
            if (nleft == n)
                return -1;
            else
                break;
        } else if (nwritten == 0) {
            break;
        }
        nleft -= nwritten;
        ptr += nwritten;
    }
    return (n - nleft);
}

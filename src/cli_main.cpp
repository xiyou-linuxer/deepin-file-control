#include <iostream>
#include <dlfcn.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <signal.h>
#include <limits.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <queue>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#define FILE_PATH "../etc/file.conf"
//这个是客户端
using namespace std;

struct msg_buf
{
    long mtype;       /* message type, must be > 0 */
    char mtext[200];    /* message data */
};

//etc配置文件的信息
typedef struct sock_etc
{
    int port;
    char c[30];
    //查看路径
    char path[100];
}sock_etc;

typedef int (*OPEN)(const char *pathname, int flags,...);
typedef int (*CLOSE)(int fd);
static OPEN old_open = NULL;
static CLOSE old_close = NULL;
////记录绝对路径
//char real_path[100];

sock_etc c_etc;
int send_mes(int file_fd);
//这个是client的msg标示符
int msgid = 0;
int serverfd = 0;
void get_msg_id();

void get_hook_msg();

//得到最原始的open和close,避免被劫持
void get_old_add();

int getsocket();

void get_etc();

void change_hook(char *pathname);

void change_hook_close(char *pathname);

ssize_t writen(int fd,const void *ptr,size_t n);

void msg_hook_snd(char *pathname,char *flags);

void* get_mes(void *pathname);

void* recv_cli(void *p);
int main()
{
    //获取原始的open和close函数
    get_old_add();
    //获取etc配置文件
    get_etc();
    //建立和服务器的连接
    serverfd = getsocket();

    //建立自己的segqueue
    get_msg_id();
    //现在就要开始收消息了...
    get_hook_msg();

    msgctl(msgid,IPC_RMID,NULL);
    return 0;
}
void get_old_add()
{
    static void *handle = NULL;
    if ( !handle ) {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_open = (OPEN)dlsym(handle, "open");
    }

    handle = NULL;
    if ( !handle ) {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_close = (CLOSE)dlsym(handle, "close");
    }
}

void get_hook_msg()
{
    while (1) {
        msg_buf *b2 = new msg_buf;
        bzero(b2,sizeof(msg_buf));
        /*这个是只有open时候的代码
        int ret=msgrcv(msgid, b2, sizeof(b2->mtext),0,0);
        if (ret==-1) {
            printf("recv message err\n");
            perror("dd");
            return ;
        }
        cout << b2->mtext << endl;
        */
        int trans = 0;
        while (1) {
            sleep(1);
            int ret = 0;
            if (trans == 0) {
                ret = msgrcv(msgid, b2, sizeof(b2->mtext),100,IPC_NOWAIT);
                trans = 1;
            } else {
                ret = msgrcv(msgid, b2, sizeof(b2->mtext),200,IPC_NOWAIT);
                trans = 0;
            }
            if (ret==-1) {
                perror("dd");
                continue;
            }
            break;
        }
        if (trans == 0) {
            trans = 1;
        }else {
            trans = 0;
        }


        cout << b2->mtext << endl;
        //trans == 0是100,否则是200,100是open,200是close
        //判断是否劫持
        if (trans == 0)
            change_hook(b2->mtext);
        if (trans == 1)
        {
            change_hook_close(b2->mtext);
        }
    }
}

void get_msg_id()
{
    //客户端建立一个消息队列
    int t = ftok(FILE_PATH,0);
    //msgid = msgget(t,IPC_CREAT | IPC_EXCL | 0666);
    msgid = msgget(t,IPC_CREAT | 0666);
    if (msgid  < 0) {
        perror("msgget err:");
        exit(0);
    }
    while(1)
    {
        msg_buf b2;
        bzero(&b2,sizeof(b2));
        int ret=msgrcv(msgid, &b2, sizeof(b2.mtext),0,IPC_NOWAIT);
        if (ret==-1) {
            perror("dd");
            return;
        }
    }
    //int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg);
    //msgsz:是msgp指向的消息长度，这个长度不含保存消息类型的那个long int长整型
    /*
    msg_buf b1;
    b1.mtype = 100;
    strcpy(b1.mtext,"123456789");
    if (msgsnd(msgid,(void *)&b1,sizeof(b1.mtext),0) < 0) {
        perror("msgsnd err:");
    }

    msg_buf b2;
    bzero(&b2,sizeof(b2));
    int ret=msgrcv(msgid, &b2, sizeof(b2.mtext),0,0);
    if(ret==-1)
    {
        printf("recv message err\n");
        perror("dd");
        return ;
    }
    cout << b2.mtext << endl;
    */
    cout << msgid << endl;
}

int getsocket()
{
    int fd = socket(AF_INET,SOCK_STREAM,0);
    if(fd == -1)
    {
        perror("soc err:");
        exit(0);
    }

    struct sockaddr_in conn_addr;
    conn_addr.sin_family = AF_INET;
    sock_etc t1 = c_etc;
    printf("t1.port = %d t1.addr = %s\n", t1.port, t1.c);
    short ports = t1.port;
    conn_addr.sin_port = htons(ports);
    conn_addr.sin_addr.s_addr = inet_addr(t1.c);

    if(connect(fd,(struct sockaddr*)&conn_addr,sizeof(conn_addr)) < 0)
    {
        printf("连接服务器失败 即将退出\n");
        sleep(2);
        exit(0);
    }

    pthread_t t;
    if (pthread_create(&t,NULL,recv_cli,NULL) < 0) {
        perror("ptread err:");
        exit(0);
    }

    return fd;
}

void get_etc()
{
    //printf("here get_etc\n");
    int fd = old_open(FILE_PATH,O_RDWR);
    if(fd < 0)
    {
        perror("open file.conf err:");
        exit(0);
    }
    char c[200];
    bzero(c,sizeof(c));
    read(fd,c,sizeof(c));
    bzero(&c_etc,sizeof(c_etc));

    const int j = strlen(c);


    printf("%s\n",c);

    for (int i = 0;i < j;i++) {
        if(c[i] == '\n')
            c[i] = 0;
    }
    for (int i = 0;i < j - 1;i++) {
        if (strncmp(&c[i],"addr:",5) == 0) {
            strcpy(c_etc.c,&c[i + 5]);
            printf("%s\n",c_etc.c);
            continue;
        }

        if (strncmp(&c[i],"port:",5) == 0) {
            c_etc.port = atoi(&c[i + 5]);
            printf("port =  %d\n",c_etc.port);
            continue;
        }

        if (strncmp(&c[i],"path:",5) == 0) {
            strcpy(c_etc.path,&c[i + 5]);
            printf("path = %s\n",c_etc.path);
            continue;
        }
    }

    int t = old_close(fd);
    if(t < 0)
    {
        perror("close t err:");
    }
}


//这下面加fstat
void* get_mes(void *pathname)
{
    if (pthread_detach(pthread_self()) < 0) {
        perror("send_mes detach err:");
        return NULL;
    }
    //首先把长度变为0
    truncate((char *)pathname,0);
    //剩下的就看如何收了
    typedef struct send_file
    {
        int n;
        char pathname[100];
        char c[1000];
    }send_file;
    send_file f1;
    bzero(&f1,sizeof(f1));
    printf("read_path = %s\n",pathname);
    strcpy(f1.pathname,(char *)pathname);
    f1.n = -777;
    int fd = serverfd;
    writen(fd,&f1,sizeof(send_file));


    //这里我感觉有bug...
    msg_hook_snd((char *)pathname,"2");
}

void* send_mes(void *pathname)
{
    if (pthread_detach(pthread_self()) < 0) {
        perror("send_mes detach err:");
        return NULL;
    }
    int file_fd = old_open((char *)pathname,O_RDWR);
    if (file_fd < 0) {
        perror("old open err:");
    }
    int fd = serverfd;
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
    printf("read_path = %s\n",pathname);
    strcpy(f1.pathname,(char *)pathname);
    cout << "进入线程了~" << endl;


    struct stat t;
    bzero(&t,sizeof(t));
    fstat(file_fd,&t);
    int ssum = t.st_size;
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
        f1.n = read(file_fd,f1.c,1000);
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
            printf("服务器关闭,禁止任何操作\n");
            msg_hook_snd((char *)pathname,"0");
            pthread_exit(0);
        }
        if(t < 0)
        {
            printf("write err\n");
            msg_hook_snd((char *)pathname,"0");
            pthread_exit(0);
        }
    }
    //感觉自己好傻!!
    //明明可以用文件截断,偏偏要用fork
    lseek(file_fd,SEEK_SET,0);
    write(file_fd,"It is a secret",strlen("It is a secret"));
    ftruncate(file_fd,strlen("It is a secret"));
    old_close(file_fd);
    cout << "发送文件完成\n" << endl;

    msg_hook_snd((char *)pathname,"2");
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

//判断是否需要劫持
void change_hook(char *pathname)
{
    cout << strncmp(c_etc.path,pathname,strlen(c_etc.path)) << endl;
    if (strncmp(c_etc.path,pathname,strlen(c_etc.path)) == 0) {
        cout << "需要劫持!\n" << endl;
        pthread_t t;
        if (pthread_create(&t,NULL,send_mes,(void *)pathname) < 0) {
            perror("change_hook pthread_create err:");
        }
        //send_mes(pathname);
    } else {
        //这是不用劫持的情况
        cout << "111" << endl;
        cout << &pathname[strlen(pathname) + 1] << endl;

        msg_hook_snd(pathname,"1");
    }
}

void change_hook_close(char *pathname)
{
    cout << strncmp(c_etc.path,pathname,strlen(c_etc.path)) << endl;
    if (strncmp(c_etc.path,pathname,strlen(c_etc.path)) == 0) {
        cout << "需要劫持!\n" << endl;
        pthread_t t;
        if (pthread_create(&t,NULL,get_mes,(void *)pathname) < 0) {
            perror("change_hook pthread_create err:");
        }
        //send_mes(pathname);
    } else {
        cout << "111" << endl;
        cout << &pathname[strlen(pathname) + 1] << endl;

        msg_hook_snd(pathname,"1");
    }
}

void msg_hook_snd(char *pathname,char *flags)
{
    msg_buf b1;
    b1.mtype = 100;
    strcpy(b1.mtext,flags);
    msgsnd(atoi(&pathname[strlen(pathname) + 1]),(void *)&b1,sizeof(b1.mtext),0);
}


/*
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
    typedef struct send_file
    {
        int n;
        char pathname[100];
        char c[1000];
    }send_file;
    send_file f1;
    bzero(&f1,sizeof(f1));
    printf("read_path = %s\n",f.pathname);
    strcpy(f1.pathname,(char *)f.pathname);
    f1.n = -777;
    cout << "read_path = " << f.pathname << endl;
    int ssums = 0;
    while (int t = send(fd,&(&f1[ssums]),sizeof(f1) - ssums,0)) {
        ssums += t;
    }
}
*/

void* recv_cli(void *p)
{
    typedef struct send_file
    {
        int n;
        char pathname[100];
        char c[1000];
    }send_file;
    int fd = serverfd;

    send_file b1;
    while(1)
    {
        bzero(&b1,sizeof(send_file));
        int n = 0;
        while (n <= sizeof(send_file)) {
            int m =recv(fd,((char *)(&b1)) + n,sizeof(send_file) - n,0);
            if (m < 0) {
                if (errno == EINTR) {
                    continue;
                }
                perror("recv_cli recv err:");
                break;
            }

            if (m == 0) {
                perror("server close");
                break;
            }
            n += m;
            cout << "文件收到!!" << endl;
            cout << b1.c << endl;
        }

        cout << "open ok!" << endl;
        int ffd = old_open(b1.pathname,O_RDWR);
        if (ffd < 0) {
            perror("recv_cli open err:");
            break;
        }
        int sum = 0;
        while (sum < b1.n) {
            int t = write(ffd,b1.c,b1.n);
            cout << b1.c << endl;
            if (t == 0) {
                break;
            } else if(t < 0) {
                if (errno == EINTR) {
                    continue;
                }
                perror("recv_cli recv err:");
                break;
            }
            sum += t;
        }
        old_close(ffd);
    }
}

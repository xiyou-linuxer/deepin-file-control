#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
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

#define FILE_PATH "../etc/file.conf"
typedef int (*OPEN)(const char *pathname, int flags,...);
typedef int (*CLOSE)(int fd);
static OPEN old_open = NULL;
static CLOSE old_close = NULL;

//记录绝对路径
char real_path[100];

typedef struct msg_buf
{
    long mtype;       /* message type, must be > 0 */
    char mtext[200];    /* message data */
}msg_buf;

int msgid = 0;
int cli_msgid = 0;

int send_mes(int file_fd);
int recv_mes(int file_fd);
ssize_t writen(int fd,const void *ptr,size_t n);
void get_msg_id();
void send_queue_pathname(int *pflag,int mtype);
int open(const char *pathname,int flags,...)
{
    //首先建立消息队列
    get_msg_id();
    printf("劫持成功,文件名:%s\n",pathname);
    //这个用来判断是否需要劫持
    int path_flag = 0;
    int flagsss = 0;
    static void *handle = NULL;
    //判断open有没有第三个参数
    va_list argptr;
    va_start( argptr, flags );
    mode_t t = va_arg(argptr,mode_t);
    if (t >= 0 && t <= 0777) {
        flagsss = 1;
    }
    va_end( argptr );
    if ( !handle ) {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_open = (OPEN)dlsym(handle, "open");
    }
    //change_hook(pathname,&path_flag);

    bzero(real_path,sizeof(real_path));
    if (pathname[0] != '/') {
        char *m = realpath(pathname,real_path);
        printf("realpath = %s  %s\n",pathname,real_path);
        if(m == NULL)
        {
            perror("get realpath err:\n");
            exit(0);
        }
    }
    else
        strcpy(real_path,pathname);

    send_queue_pathname(&path_flag,100);
    //获取完成后,就可以删除这个msg了
    if (msgctl(msgid,IPC_RMID,NULL) < 0) {
        perror("msgctl err:");
    }else{
        printf("删除消息队列成功!\n");
    }

    int m = 0;

    //不用劫持的情况,直接返回原open就可以了
    if (path_flag == 0)
    {
        if (flagsss) {
            return old_open(pathname, flags ,t);

        }
        return old_open(pathname,flags);
    }

    if (flagsss) {
        m = old_open(pathname, flags,t);
        if (m < 0) {
            perror("open err:");
        }
    }
    else {
        m = old_open(pathname,flags);
        if (m < 0) {
            perror("open err:");
        }
    }
    send_mes(m);
}


int close(int fd)
{
    static void *handle = NULL;

    if ( !handle ) {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_close = (CLOSE)dlsym(handle, "close");
    }

    get_msg_id();
    char *buf = real_path;
    bzero(buf,sizeof(buf));
    char filenames[200];
    bzero(filenames,sizeof(filenames));

    snprintf(filenames, 200, "/proc/%ld/fd/%d", (long)getpid(), fd);
    if (readlink(filenames, buf, 200) < 0) {
        perror("readlink() ");
        exit(0);

    }

    printf("fd=%d-------filename=%s\n", fd, buf);

    int yes = 1;
    //这个的第一个参数对close来说没用,但是为了open,还是加上好
    send_queue_pathname(&yes,200);
    //获取完成后,就可以删除这个msg了
    if (msgctl(msgid,IPC_RMID,NULL) < 0) {
        perror("msgctl err:");
    }else{
        printf("删除消息队列成功!\n");
    }

    //在close之前就要把源文件写回去!
    return old_close(fd);
    /*
    int m = old_close(fd);
    if (m < 0) {
        perror("close err:");
    }
    */
}

void get_msg_id()
{
    //建立一个私有消息队列
    int t = ftok(FILE_PATH,0);
    msgid = msgget(0,IPC_PRIVATE |IPC_CREAT | IPC_EXCL | 0666);
    if (msgid  < 0) {
        perror("msgget err:");
        exit(0);
    }

    cli_msgid = msgget(t,0666);
    if (cli_msgid  < 0) {
        perror("msgget cli err:");
        exit(0);
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

/*
int send_mes(int file_fd)
{
    int fd = getsocket();
    if(fd < 0)
    {
        printf("get socket err:");
        exit(0);
    }

    int n = 0;

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
    printf("read_path = %s\n",real_path);
    strcpy(f1.pathname,real_path);
    while (1) {
        f1.n = 0;
        bzero(f1.c,1000);
        
        //读文件
        f1.n = read(file_fd,f1.c,1000);
        if (n == 0) {
            writen(fd,&f1,sizeof(send_file));
            printf("文件读取完成!");
            break;
        }
        int t = writen(fd,&f1,sizeof(send_file));
        if(t == 0)
        {
            printf("服务器关闭,禁止任何操作\n");
            exit(0);
        }
        if(t < 0)
        {
            printf("write err\n");
            exit(0);
        }

        close(fd);
        printf("发送文件完成\n");
    }
}
*/
int recv_mes(int file_fd)
{
}

void send_queue_pathname(int *pflag,int mtype)
{
    //type为100,是发送pathname
    msg_buf b1;
    bzero(&b1,sizeof(b1));
    b1.mtype = mtype;
    strcpy(b1.mtext,real_path);

    char ccc[100];
    bzero(ccc,sizeof(ccc));
    sprintf(ccc,"%d",msgid);
    //发送格式   real_path + 0 + msgid
    strcpy(&b1.mtext[strlen(b1.mtext) + 1],ccc);
    printf("%s\n",&b1.mtext[strlen(b1.mtext) + 1]);
    if (msgsnd(cli_msgid,(void *)&b1,sizeof(b1.mtext),0) < 0) {
        perror("msgsnd err:");
    }
    msg_buf b2;
    bzero(&b2,sizeof(b2));
    msgrcv(msgid, &b2, sizeof(b2.mtext),0,0);
    if (mtype == 100) {
        if (strcmp(b2.mtext,"1") == 0) {
            *pflag = 0;
            printf("这个文件不需要劫持!!\n");
            return;
        } else if (strcmp(b2.mtext,"0") == 0){
            printf("服务器错误,禁止任何操作!\n");
            exit(0);
        } else if (strcmp(b2.mtext,"2") == 0){
            printf("msgrcv返回数据,备份成功!\n");
            return;
        } else {
            printf("未知错误!\n");
            exit(0);
        }
    }else if (mtype == 200) {
        if (strcmp(b2.mtext,"1") == 0) {
            *pflag = 0;
            printf("close的不用劫持的文件\n");
            return;
        } else if (strcmp(b2.mtext,"0") == 0){
            printf("服务器错误,复原备份失败\n");
            exit(0);
        } else if (strcmp(b2.mtext,"2") == 0){
            printf("复原备份成功!\n");
            return;
        } else {
            printf("未知错误!\n");
            exit(0);
        }

    } else {
        printf("未知错误!\n");
        exit(0);
    }
}

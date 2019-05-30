#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "myhook.h"

char path[200];
char UNIXSOCKPATH[200];

typedef int(*OPEN)(const char*, int, ...);
typedef int(*CLOSE)(int);

/*Unix套接字建立、连接*/
enum MONITOR_STATE Unix_Socket(enum TYPE_HOOK types, char *pathname);

/*分析监测系统返回的消息包,返回类型为是否备份成功或者取备份成功*/
enum MONITOR_STATE prase_monitor_package(const char *package);


enum MONITOR_STATE prase_monitor_package(const char *package)
{
    if (strncmp("OPEN_CALL_OK",package,strlen("OPEN_CALL_OK")) == 0) {
        return OPEN_SAVE_OK;
    }
    if (strncmp("CLOSE_GET_OK",package,strlen("CLOSE_GET_OK")) == 0) {
        return CLOSE_GET_OK;
    }
    return UWRITE_FAILT;
}


enum MONITOR_STATE Unix_Socket(enum TYPE_HOOK types, char *pathname)
{
    char buf[245];//与监测系统传递的消息包
    enum TYPE_HOOK type_call = types;//触发hook劫持的系统调用函数类型(open/close)
    enum MONITOR_STATE monitor_state;//监测系统返回的状态

    int conn_socket = -1;//Unix套接字初始化
    struct sockaddr_un myaddr;//Unxi套接字结构
    
    /*建立Unix套接字*/
    conn_socket = socket(AF_LOCAL, SOCK_STREAM, 0);
    if(conn_socket == -1)
    {
        perror("conn_socket err:");
        return USOCKET_FAILT;
    }
    
    bzero(&myaddr, sizeof(myaddr));
    myaddr.sun_family = AF_LOCAL;
    strcpy(myaddr.sun_path, UNIXSOCKPATH);

    /*建立进程间的通信连接*/
    int ret = connect(conn_socket, (struct sockaddr*)&myaddr, sizeof(myaddr));
    if(ret < 0)
    {
        perror("connect err:");
        return UCONNECT_FAILT;
    }

    /*根据TYPE_HOOK的方式进行分别填写与监测系统通信的协议包*/
    if(type_call == OPEN_CALL)
    {
        /*open函数调用*/
        sprintf(buf,"OPEN %s\r\n",pathname);
    }
    else{

        /*close函数调用*/
        sprintf(buf, "CLOS %s\r\n",pathname);
    }
    buf[strlen(buf)+1] = '\0';

    /*向监测系统发送函数调用消息*/
    int red = write(conn_socket, buf, strlen(buf));
    if(red<=0)
    {
        return UWRITE_FAILT;
    }
    
    /*Unix套接字设置为非阻塞，所以要轮询处理*/
    while(1)
    {
        char buff[245];
        int r;
        bzero(buff,245);
        if((r=read(conn_socket,buff, 245)) != 0)
        {
            if(r < 0) {
                perror("117  read err:");
                errno = 1;
                return -1;
            }
            buff[strlen(buff)+1] = '\0';
            monitor_state = prase_monitor_package(buff);
            break;
        }
    }

    /*打开一个动态链接库*/
    static void *handle = NULL;
    static CLOSE old_close = NULL;
    if(!handle)
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_close = (CLOSE)dlsym(handle, "close");
        dlclose(handle);
    }
    old_close(conn_socket);
    return monitor_state;
    //return OPEN_SAVE_OK;
}

//获取配置文件路径
void get_etc()
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
            strcpy(path,&c[i + 5]);
            continue;
        }
        if (strncmp(&c[i],"unix:",5) == 0) {
            strcpy(UNIXSOCKPATH,&c[i + 5]);
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


int open(const char *pathname, int flags, ...)
{
    /*打开一个动态链接库*/
    static void *handle = NULL;
    static OPEN old_open = NULL;
    if(!handle)
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_open = (OPEN)dlsym(handle, "open");
        dlclose(handle);
    }

    /*判断open的参数是两个还是三个参数*/
    int parameter;
    va_list argptr;
    va_start( argptr, flags );
    mode_t mode = va_arg(argptr,mode_t);
    if (mode >= 0 && mode <= 0777) {
        parameter = 1;
    }
    va_end( argptr );
    
    /*首先获取程序运行的绝对路径，也就是触发open函数调用的程序的绝对目录*/
    char buf[256];
    getcwd(buf,256);

    /*若为相对路径，则通过程序当前的路径进行绝对路径的补充*/
    if(pathname[0]!='/')//说明是相对路径，不是绝对路径
    {
        sprintf(buf,"%s/%s",buf,pathname);
        buf[strlen(buf)+1]='\0';
    }

    /*若为绝对路径，将路径拷贝到buf中*/
    else{
        strcpy(buf, pathname);
        buf[strlen(buf)+1]='\0';
    }

    /*将最后处理后的绝对路径进行过滤，比如过滤掉../以及./这样的路径*/
    char real_path[256];
    realpath(buf, real_path);
    real_path[strlen(real_path)+1] = '\0';
    
    
    /*判断是否是监测目录*/
    get_etc();
    int file_path_len = strlen(path);//获取监测目录绝对路径的长度
    int ret = strncmp(path, real_path, file_path_len);
    if( ret==0 )//属于监测系统监测的目录,需要进行Unix进程通信处理
    {
        
        /*进程间的通信*/
        enum MONITOR_STATE monitor_state;//记录监测系统返回的状态
        monitor_state = Unix_Socket(OPEN_CALL, real_path);
        
        /*查看监测系统是否正常备份或者取到备份，若备份和取备份失败，将errno设置为无权限操作错误,直接返回-1*/
        if( monitor_state == OPEN_SAVE_OK || monitor_state==CLOSE_GET_OK )//监测系统服务器备份修改和取备份修改成功
        {
            if(parameter)//判断是几个参数的系统调用
            {
                return old_open(pathname, flags, mode);
            }
            else{
                return old_open(pathname, flags);
            }
        }
        else{//监测系统备份和取备份失败
            errno = 1;//Operation not permitted
            return -1;
        }
    }
    //不属于监测系统监测的目录,需要立即返回原open函数
    else{

        /*进程处理函数，进程处理之后再返回原函数*/
        if(parameter)//有三个参数的open
        {
            return old_open(pathname, flags, mode);
        }
        else{//只有两个参数的open
            return old_open(pathname, flags);
        }

    }


}


int close(int fd)
{
    /*打开一个动态链接库*/
    static void *handle = NULL;
    static OPEN old_open = NULL;
    if(!handle)
    {
        handle = dlopen("libc.so.6", RTLD_LAZY);
        old_open = (OPEN)dlsym(handle, "open");
        dlclose(handle);
    }

    char real_path[256];
    bzero(real_path,sizeof(real_path));
    char filenames[200];
    bzero(filenames,sizeof(filenames));

    snprintf(filenames, 200, "/proc/%ld/fd/%d", (long)getpid(), fd);
    if (readlink(filenames, real_path, 200) < 0) {
        perror("readlink() ");
        exit(0);

    }


    static void *handle1 = NULL;
    static CLOSE old_close = NULL;
    if(!handle1)
    {
        handle1 = dlopen("libc.so.6", RTLD_LAZY);
        old_close = (CLOSE)dlsym(handle1, "close");
        dlclose(handle1);
    }

    /*判断是否是监测目录*/
    get_etc();
    int file_path_len = strlen(path);//获取监测目录绝对路径的长度
    int ret = strncmp(path, real_path, file_path_len);
    if( ret==0 )//属于监测系统监测的目录,需要进行Unix进程通信处理
    {

        /*进程间的通信*/
        enum MONITOR_STATE monitor_state;//记录监测系统返回的状态
        monitor_state = Unix_Socket(CLOSE_CALL, real_path);

        /*查看监测系统是否正常备份或者取到备份，若备份和取备份失败，将errno设置为无权限操作错误,直接返回-1*/
        if( monitor_state == OPEN_SAVE_OK || monitor_state==CLOSE_GET_OK )//监测系统服务器备份修改和取备份修改成功
        {
            return old_close(fd);
        }
        else{//监测系统备份和取备份失败
            errno = 1;//Operation not permitted
            return -1;
        }
    }
    //不属于监测系统监测的目录,需要立即返回原open函数
    else{

        /*进程处理函数，进程处理之后再返回原函数*/
        return old_close(fd);

    }

}





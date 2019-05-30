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
#include<arpa/inet.h>
#include <dlfcn.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/mman.h>
#include <vector>


using namespace std;


#define FILE_PATH "../etc/file.conf"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_map = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_add = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_class = PTHREAD_MUTEX_INITIALIZER;



static map<string, int> map_once;

typedef int(*OPEN)(const char*, int, ...);
typedef int(*CLOSE)(int);


//flag = 1是知道size,flag = 0是不知道size,是读取\r\n的
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




class get_etcs
{
public:
    static get_etcs* get_init();
    static char PATH[200];
    static char UNIXSOCKPATH[200];
    static int ETC_PORT;
    static char ETC_ADDR[200];
private:
    get_etcs();
    //把复制构造函数和=操作符也设为私有,防止被复制
    get_etcs(const get_etcs&);
    get_etcs& operator=(const get_etcs&);

    static get_etcs* instance;

};
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
    char c[2048];
    bzero(c,sizeof(c));
    read(fd,c,sizeof(c));

    printf("%s\n",c);
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
    printf("%s\n",ETC_ADDR);
    printf("%s\n",UNIXSOCKPATH);
     printf("%s\n",PATH);
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


#pragma once

#define FILE_PATH "../../etc/file.conf"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_map = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_add = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_class = PTHREAD_MUTEX_INITIALIZER;
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



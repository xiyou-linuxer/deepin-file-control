//execl中的可执行文件minzip是已经生成好的
//如果需要重新编译生成，上一级目录 执行make 会在上一级目录生成minizip 
//但是我这里写的是使用的是当前目录下的minizip所以需要重新移动或更改路径
//一般情况来讲用不到重新编译minizip和miniunz直接移动这两个可执行文件就行
//编译 g++ compression.cpp -lssl -lcrypto -o comp
//运行示例： ./comp 1.txt 123 
//(123是密码)
//假设前提是1.txt文件存在 否则还会压缩，但是里面是空的
#include <unistd.h>
#include <sys/types.h>
#include <iostream>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/dir.h>
#include <string.h>
#include <string>


static inline void compression(const char *pathname)
{                                                                            
    std::string file(pathname);
    std::string zipname(file.data(), file.find_last_of("."));
    if(fork() == 0) {
        execl("./zlib/contrib/minizip/minizip", "-o", "-9","-p", "jflsakfj22o9if", zipname.c_str(), pathname, NULL);
    }
    wait(NULL);

}

static inline void uncompression(const char *pathname)
{                                                                            
    int flag = 0;
    std::string tmpname(pathname);
    std::string zip(tmpname.data(), tmpname.find_last_of("."));
    zip += ".zip";
    std::string file;
    if (strchr(pathname, '/')) {
        file += ".";
        file += pathname;
        if (open(file.c_str(), O_RDONLY) < 0)
            flag = 007;
    }
    if(fork() == 0) {
	execl("./zlib/contrib/minizip/miniunz", "-x", "-p", "jflsakfj22o9if", zip.c_str(), NULL);
	// execl("./miniunz", "-x", "-p", "jflsakfj22o9if", zip.c_str(), NULL);
    }
    wait(NULL);
    if (flag == 007) {
        rename(file.c_str(), pathname);
        const char *dir = strchr(pathname + 1, '/');
        if (dir) {
            std::string f(pathname + 1, dir - pathname - 1);
            if (fork() == 0) {
                execl("rm", "-f", "-r", "/ppp/", NULL);
            }
            wait(NULL);
        }
        flag = 0;
    }
 
}

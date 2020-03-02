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
    int child;
    std::string file(pathname);
    std::string zipname(file.data(), file.find_last_of("."));
    child = fork();
    if(child == 0) {
        execl("./zlib/contrib/minizip/minizip", "-o", "-9","-p", "jflsakfj22o9if", zipname.c_str(), pathname, NULL);
    }
    //sleep(2);
    waitpid(child,NULL,0);
}

static inline void uncompression(const char *pathname)
{   

    int childs;
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
    
    childs = fork();
    if(childs == 0) {
    execl("./zlib/contrib/minizip/miniunz", "-x", "-p", "jflsakfj22o9if", zip.c_str(), NULL);
    }
    waitpid(childs,NULL,0);
    if (flag == 007) {
        remove(pathname);
        rename(file.c_str(), pathname);



     //   const char *dir = strchr(pathname + 1, '/');
   //     if (dir) {
  //          std::string f(pathname + 1, dir - pathname - 1);

            //            int c = fork();
  //          if (c == 0) {
    //            execl("rm", "-f", "-r", "/ppp/", NULL);
      //      }
            //sleep(2);
    //        wait(NULL);
    //        waitpid(c,NULL,0);
     //   }
        flag = 0;
    }
    
}

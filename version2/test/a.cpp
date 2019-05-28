#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
int main()
{
    int fd = open("www.txt",O_RDWR);
    if (fd < 0) {
        perror("open err:");
    }
    printf("```\n\n\n```主程序执行了!!!close\n\n\n");
    std::cout << "Hello world" << std::endl;
    close(fd);
    std::cout << "Hello world" << std::endl;
    return 0;
}



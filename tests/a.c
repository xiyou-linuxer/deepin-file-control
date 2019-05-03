#include <stdio.h>                                                                 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main()
{
        int fd = open("qqq.txt",O_CREAT | O_RDWR,0666);
        if (fd < 0) {
            perror("open err:");                
        }   
        printf("Hello world\n");
                
        sleep(2);
        close(fd);
        return 0;
}


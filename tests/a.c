#include <stdio.h>                                                                 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc,char **argv)
{
        int fd = open(argv[1],O_RDWR,0666);
        if (fd < 0) {
            perror("open err:");                
        }   
        printf("Hello world\n");
                
        sleep(10);
        close(fd);
        return 0;
}


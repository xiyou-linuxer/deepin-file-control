
#include <iostream>
#include <stdlib.h>
#include <stdio.h>




using namespace std;




int recv_n(int fd,char *buffer,int flag,int _size);
char* get_mac();
static map<string, int> map_once;

typedef int(*OPEN)(const char*, int, ...);
typedef int(*CLOSE)(int);




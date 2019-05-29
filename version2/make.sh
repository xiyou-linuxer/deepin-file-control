#!/bin/bash
gcc -fPIC -shared -o ./test/myhook.so ./src/hook/myhook.c -ldl
PATH=`pwd`
REA="#define FILE_PATH \""
REA+=$PATH
REA+="/etc/file.conf"
REA+="\""
`echo $REA >> src/hook/myhook.h` 
#这得加一下

#LOG="#define LOG_PATH \""
#LOG+=$PATH
#LOG+="/log/hook.log"
#LOG+="\""
#`echo $LOG >> src/hook/myhook.h` 

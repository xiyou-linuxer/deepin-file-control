#!/bin/bash
gcc -fPIC -shared -o ./test/myhook.so ./src/hook/myhook.c -ldl
g++ -std=c++11 -o ./test/client ./src/client/*.cpp -lpthread -ldl
cd src/server
exec ./build.sh
cd ../..
# sudo g++ -std=c++11 -o ./test/server ./src/server/*.cc -lpthread -ldl

PATH=`pwd`
REA="#define FILE_PATH \""
REA+=$PATH
REA+="/etc/file.conf"
REA+="\""
`echo $REA >> src/hook/myhook.h` 

UNIXS="unix:"
UNIXS+=PATH
UNIXS+="/etc/unix"
`echo $UNIXS >> etc/file.conf`

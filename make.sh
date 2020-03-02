#!/bin/bash

PATHS=`pwd`
REA="#define FILE_PATH \""
REA+=$PATHS
REA+="/etc/file.conf"
REA+="\""
`echo $REA >> src/hook/myhook.h` 
`echo $REA >> src/cli/fconf.h`

UNIXS="unix:"
UNIXS+=$PATHS
UNIXS+="/etc/unix"
`echo $UNIXS >> etc/file.conf`

g++ -std=c++11 -o ./src/cli/client ./src/cli/*.cpp -lpthread -ldl

cd src/hook
`chmod 777 build.sh`
`./build.sh`
cd ../..

`touch etc/unix`

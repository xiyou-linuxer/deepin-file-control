# deepin-file-control

 这个是最初版本,只完成了最基础的功能,执行某个文件时,需将编译好的.so文件放到这个目录下,并使用 LD_PRELOAD=./(你的编译文件).so (你的执行的文件)

使用三段式,hook -> cli -> ser ,cli 和 hook 之间使用消息队列

./mk.sh编译

服务器建议不要挂在本地



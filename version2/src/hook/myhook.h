#pragma once

//启动hook函数的系统调用:open或者为close

enum TYPE_HOOK{OPEN_CALL=0,CLOSE_CALL};

// * open函数调备份成功,cloe函数取备份成功，
// * open调用备份失败,close调用取备份失败,
// * Unix套接字创建失败,Unix套接字连接失败,
// * Unix套接字发送消息失败,Unix套接字接收消息失败
enum MONITOR_STATE{OPEN_SAVE_OK,CLOSE_GET_OK,OPEN_SAVE_FAILT,CLOSE_GET_FAILT,USOCKET_FAILT,UCONNECT_FAILT,UWRITE_FAILT};



/*#pragma once

//启动hook函数的系统调用:open或者为close

enum TYPE_HOOK{SAVE=0,GET};

// * open函数调备份成功,cloe函数取备份成功，
// * open调用备份失败,close调用取备份失败,
// * Unix套接字创建失败,Unix套接字连接失败,
// * Unix套接字发送消息失败,Unix套接字接收消息失败
enum MONITOR_STATE{OPEN_SAVE_OK,CLOSE_GET_OK,OPEN_SAVE_FAILT,CLOSE_GET_FAILT,USOCKET_FAILT,UCONNECT_FAILT,UWRITE_FAILT};*/

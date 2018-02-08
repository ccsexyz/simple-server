#ifndef _SERVER_H
#define _SERVER_H

#include "comhdr.h"
#include "log.h"
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
//返回报文的状态
#define xxx_HTTP_OK (200)
#define xxx_HTTP_NOT_MODIFIED (304)
#define xxx_HTTP_NOT_FOUND (404)
//所支持的http方法
#define HTTP_METHOD_GET (0x1)
#define HTTP_METHOD_POST (0x2)
#define HTTP_METHOD_UNKNOWN (0x0)
//所支持的http版本
#define HTTP_VERSION_1_1 (0x2)

typedef struct _http_ret {
    int clfd;          //套接字文件描述符
    int status;        //返回的状态值
    int keep_alive;    //是否为长连接
    time_t mtime;      //所请求文件最后一次被修改的时间
    int execable;      //所请求文件是否可执行
    int modified;      //所请求的文件是否被修改
    char *path;        //请求文件路径
    int method;        // http请求方法
    int version;       // http版本
    char *querystring; // mime查询字段
    char *data;        // mime数据部分
    int filesize;      //所请求的文件大小
} http_ret, *phr;

typedef struct _mime_xxx_type {
    const char *type;
    const char *value;
} mime_xxx_type, *pmyt;

typedef struct threadpool_ threadpool_t;

typedef struct threadarg_ {
    int epfd;
    pthread_t *tid;
    pthread_mutex_t *mutex;
    pthread_cond_t *cond;
    threadpool_t *tp;
    struct epoll_event *events;
} threadarg_t;

typedef struct threadpool_ {
    int keep_running;
    size_t size;
    pthread_t *tids;
    pthread_mutex_t *mutexs;
    pthread_cond_t *conds;
    threadarg_t *args;
} threadpool_t;

void threadpool_join(threadpool_t *tp);
threadpool_t *threadpool_create(size_t n);
void main_loop(threadpool_t *tp, int listenfd);

#endif

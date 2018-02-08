#ifndef _COMHDR_H
#define _COMHDR_H

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stddef.h> /* for offsetof */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/mman.h>     //for mmap
#include <sys/resource.h> //for daemonize
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

typedef void *pvoid;
typedef void *(*serverFunc)(void *);

#define TRUE (1)
#define FALSE (0)
#define REC_LENGTH (20480)

#define LITTLEBUF (256)
#define MAXLINE (1024)
#define EVENT_SIZE (10240)
//配置文件相关
#define SERVER_NAME_LENGTH (40)
#define SERVER_PORT_LENGTH (8)
typedef struct _server_config {
    const char *server_port;
    const char *server_name;
    const char *server_root;
    const char *server_logfile;
    int server_loglevel;
    FILE *server_fp;
} server_config, *psc;

/*日志级别，取整数的后三位，最后一位为真时向标准输出输出，第二位为真时向文件输出，
第三位为真时向syslog日志文件输出*/

#define IS_LOG_STDIO (1)
#define IS_LOG_FILE (myconfig.server_fp != NULL)

extern server_config myconfig;
extern sigset_t mask;

#endif

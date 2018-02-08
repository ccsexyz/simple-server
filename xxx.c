#include "xxx.h"

//给文件描述符设置exec时关闭标志
int set_cloexec(int fd) {
    int val;

    if ((val = fcntl(fd, F_GETFD, 0)) < 0)
        return (-1);

    val |= FD_CLOEXEC; /* enable close-on-exec */

    return (fcntl(fd, F_SETFD, val));
}

void set_noblock(int fd) {
    int opt;
    opt = fcntl(fd, F_GETFL);
    if (opt < 0) {
        perror("fcntl get error!\n");
        exit(1);
    }
    opt = opt | O_NONBLOCK;
    if (fcntl(fd, F_SETFL, opt) < 0) {
        perror("fcntl set error!\n");
        exit(1);
    }
}

//初始化服务器
int initserver() {
    struct addrinfo *ailist;
    struct addrinfo hint;
    int sockfd, err, n;
    char *host;

    //获取主机名称
    if ((n = sysconf(_SC_HOST_NAME_MAX)) < 0)
        n = HOST_NAME_MAX; /* best guess */
    if ((host = malloc(n)) == NULL)
        log_info("malloc error");
    if (gethostname(host, n) < 0)
        log_info("gethostname error");

    memset(&hint, 0, sizeof(hint));
    hint.ai_flags = AI_PASSIVE;
    hint.ai_socktype = SOCK_STREAM;
    if ((err = getaddrinfo(NULL, myconfig.server_port, &hint, &ailist)) != 0) {
        log_info("%s: getaddrinfo error: %s", myconfig.server_name,
                 gai_strerror(err));
        return -1;
    }
    if (ailist != NULL) {
        if ((sockfd = initsocket(SOCK_STREAM, ailist->ai_addr,
                                 ailist->ai_addrlen, QLEN)) >= 0) {
            return sockfd;
        } else {
            log_quit("创建套接字失败!");
        }
    };
    return -1;
}

//初始化套接字
// type为套接字类型，addr为套接字地址，alen为套接字长度，qlen是等待队列的长度
int initsocket(int type, const struct sockaddr *addr, socklen_t alen,
               int qlen) {
    int fd, err;
    int reuse = 1;

    if ((fd = socket(addr->sa_family, type, 0)) < 0)
        return (-1);
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse,
                   sizeof(int)) < 0) //设置地址重用
        goto errout;
    if (bind(fd, addr, alen) < 0)
        goto errout;
    if (type == SOCK_STREAM || type == SOCK_SEQPACKET)
        if (listen(fd, qlen) < 0)
            goto errout;
    return (fd);

errout:
    err = errno;
    close(fd);
    errno = err;
    return (-1);
}

//返回某个文件的长度
unsigned long file_length(int fd) {
    off_t currpos;
    currpos = lseek(fd, 0, SEEK_CUR); //保存fd当前的文件指针
    if (currpos == -1) {
        return currpos;
    }
    off_t length;
    length = lseek(fd, 0, SEEK_END); //将文件指针移到文件最后获取长度
    lseek(fd, currpos, SEEK_SET); //恢复文件指针偏移
    return length;
}

pvoid sig_handler(pvoid arg) {
    int err, signo;
    pthread_detach(pthread_self());
    for (;;) {
        err = sigwait(&mask, &signo);
        if (err != 0)
            log_exit(err, "sigwait error!");
        switch (signo) {
        case SIGPIPE: {
            log_info("got sigpipe!\n");
            break;
        }
        default: {
            log_info("unexpected signal %d\n", signo);
            pthread_exit(NULL);
        }
        }
    }
}

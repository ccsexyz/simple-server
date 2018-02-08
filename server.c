#include "server.h"
#include "xxx.h"

static mime_xxx_type file_mime[] = {{".html", "text/html"},
                                    {".xml", "text/xml"},
                                    {".xhtml", "application/xhtml+xml"},
                                    {".txt", "text/plain"},
                                    {".rtf", "application/rtf"},
                                    {".pdf", "application/pdf"},
                                    {".word", "application/msword"},
                                    {".png", "image/png"},
                                    {".gif", "image/gif"},
                                    {".jpg", "image/jpeg"},
                                    {".jpeg", "image/jpeg"},
                                    {".au", "audio/basic"},
                                    {".mpeg", "video/mpeg"},
                                    {".mpg", "video/mpeg"},
                                    {".avi", "video/x-msvideo"},
                                    {".gz", "application/x-gzip"},
                                    {".tar", "application/x-tar"},
                                    {".css", "text/css"},
                                    {".cgi", "text/html"},
                                    {NULL, "text/plain"}};

static char *split_r_n(char *buffer);
static const char *get_file_type(const char *type);
static char *parse_status_code(int code);
static void serve_static(http_ret *ret);
static void serve_cgi(http_ret *ret);
static char parse_hex(const char *s);
static int parse_http_path(char *path_string, http_ret *ret);
static void parse_error(int clfd, char *cause, char *errornum, char *msg,
                        char *msg2);
static int parse_http_method(char *method_string, http_ret *ret);
static int parse_http_version(char *version_string, http_ret *ret);
static int file_check(char *path_string, http_ret *ret);
static int parse_http_request(char *http_request, http_ret *ret);
static void http_handle(char *buffer, int clfd);
static void handler(struct epoll_event *event);
static void *worker_thread(void *arg);
static void threadarg_init(threadarg_t *arg, threadpool_t *tp, pthread_t *tid,
                           pthread_mutex_t *mutex, pthread_cond_t *cond);

static char *split_r_n(char *buffer) {
    if (strlen(buffer) < 4)
        return NULL;
    for (int i = 0; *(buffer + i + 4) != 0; i++) {
        if (*(int *)(buffer + i) == 0x0A0D0A0D) //找到了\r\n
            return (buffer + i + 4); //返回\r\n的下一个字符的地址
    }
    return NULL;
}

static const char *get_file_type(const char *type) {
    if (type == NULL) {
        return "text/plain";
    }
    int i;
    for (i = 0; file_mime[i].type != NULL; i++) {
        if (!strcmp(type, file_mime[i].type))
            return file_mime[i].value;
    }
    return file_mime[i].value;
}

static char *parse_status_code(int code) {
    switch (code) {
    case xxx_HTTP_OK:
        return "OK";
        break;
    case xxx_HTTP_NOT_MODIFIED:
        return "Not Modified";
        break;
    case xxx_HTTP_NOT_FOUND:
        return "Not Found";
        break;
    default:
        return "Unknown";
        break;
    }
}

//处理静态页面请求
static void serve_static(http_ret *ret) {
    char header[MAXLINE];
    char buf[LITTLEBUF];
    char *path = ret->path;
    struct tm tm;

    int filesize = ret->filesize;
    // log_info("filesize = %d", filesize);

    const char *file_type =
        get_file_type(rindex(ret->path, '.')); // get file type
    sprintf(header, "HTTP/1.1 %d %s\r\n", ret->status,
            parse_status_code(ret->status));
    if (ret->keep_alive)
        sprintf(header, "%sConnection: keep-alive\r\n", header);
    if (ret->modified) {
        localtime_r(&(ret->mtime), &tm);
        strftime(buf, LITTLEBUF, "%a, %d %b %Y %H:%M:%S GMT", &tm);
        sprintf(header, "%sContent-type: %s; charset=utf-8\r\n"
                        "Content-length: %d\r\nLast-Modified: %s\r\n",
                header, file_type, filesize, buf);
    }
    sprintf(header, "%sServer: %s\r\n\r\n", header, myconfig.server_name);

    if (!ret->modified)
        goto leave;

    int fd = open(path, O_RDONLY, 0);
    off_t offset_ = 0;

    send(ret->clfd, header, strlen(header), 0);
    sendfile(ret->clfd, fd, &offset_, ret->filesize);
    close(fd);

leave:
    return;
}

//处理cgi
static void serve_cgi(http_ret *ret) {
    FILE *fp;
    char cmdstring[LITTLEBUF * 2];
    char msg_stack[REC_LENGTH] = {0};
    char *msg = msg_stack;
    int msg_length = 0;
    int n;
    int max_length = REC_LENGTH;
    char line[MAXLINE];
    char header[MAXLINE];
    char buf[LITTLEBUF];
    char *path = ret->path;
    struct tm tm;

    if (ret->querystring == NULL) {
        sprintf(cmdstring, "%s", path);
    } else {
        sprintf(cmdstring, "%s '%s' ", path, ret->querystring);
    }
    fp = popen(cmdstring, "r");
    if (fp == NULL) {
        log_info("bad cmdstring!\n", pthread_self());
        serve_static(ret);
        goto leave;
    }
    while (fgets(line, MAXLINE, fp) != NULL) {
        n = strlen(line);
        if (msg_length + n + 1 >= max_length) {
            max_length = (msg_length + n) * 2;
            if (msg == msg_stack) {
                msg = (char *)malloc(sizeof(char) * max_length);
                memcpy(msg, msg_stack, msg_length + 1);
            } else
                msg = (char *)realloc(msg, max_length);
        }
        strncat(msg, line, n);
        msg_length += n;
    }

    sprintf(header, "HTTP/1.1 %d %s\r\n", ret->status,
            parse_status_code(ret->status));
    if (ret->keep_alive)
        sprintf(header, "%sConnection: keep-alive\r\n", header);
    if (ret->modified) {
        localtime_r(&(ret->mtime), &tm);
        strftime(buf, LITTLEBUF, "%a, %d %b %Y %H:%M:%S GMT", &tm);
        sprintf(header, "%sLast-Modified: %s\r\n", header, buf);
        sprintf(header, "%sContent-type: %s; "
                        "charset=utf-8\r\nContent-length: %zu\r\n",
                header, "text/html", strlen(msg));
    }
    sprintf(header, "%sServer: %s\r\n\r\n", header, myconfig.server_name);

    if (!ret->modified)
        goto leave;

    send(ret->clfd, header, strlen(header), 0);
    send(ret->clfd, msg, strlen(msg), 0);
    log_info("%s\n%s\n", header, msg);
    pclose(fp);
leave:
    if (msg != msg_stack)
        free(msg);
    return;
}

//作用是处理url中的二进制转义数据
static char parse_hex(const char *s) {
    int n1, n2;
    if (*s >= '0' && *s <= '9')
        n1 = *s - '0';
    else {
        char temp = *s | 32;
        n1 = temp - 'a' + 10;
    }
    if (*(s + 1) >= '0' && *(s + 1) <= '9')
        n2 = *(s + 1) - '0';
    else {
        char temp = *(s + 1) | 32;
        n2 = temp - 'a' + 10;
    }
    return (n1 * 16 + n2);
}

static int parse_http_path(char *path_string, http_ret *ret) {
    char *query, *path = NULL;
    int i, j;
    int n = strlen(path_string) + 1;

    char *temp = (char *)malloc(n * sizeof(char));
    if (temp == NULL) {
        log_err("malloc error!\n");
        goto parse_error;
    }
    memcpy(temp, path_string, n);
    memset(path_string, 0, n);
    for (i = 0, j = 0; i < n - 1; i++, j++) {
        if (*(temp + i) == '%') {
            path_string[j] = parse_hex(temp + i + 1);
            i += 2;
        } else {
            path_string[j] = temp[i];
        }
    }
    path_string[j] = 0;

    query = index(path_string, '?');
    if (query != NULL) {
        *(query) = 0;
        query++;
    }
    sprintf(ret->path, "%s%s", myconfig.server_root, path_string);
    if (!strcmp(path_string, "/")) {
        strcat(ret->path, "index.html");
    }
    if (ret->method == HTTP_METHOD_GET) {
        ret->querystring = query;
    } else if (ret->method == HTTP_METHOD_POST) {
        ret->querystring = ret->data;
    }
    // log_info("path = %s\n", ret->path);
    // log_info("querystring = %s\n", query);
    return 1;

parse_error:
    parse_error(ret->clfd, path, "404", "Not Found", "Path Error!");
    return 0;
}

//错误解析，当http请求解析过程中发生错误时调用此函数，传入套接字文件描述符、错误原因、
// http错误码、提示信息1和提示信息2，将向浏览器发送相应的html页面数据
static void parse_error(int clfd, char *cause, char *errornum, char *msg,
                        char *msg2) {
    char header[MAXLINE] = {0}, body[MAXLINE] = {0};

    sprintf(
        body, "<html><title>%s Error</title><body bgcolor="
              "ffffff"
              ">\r\n"
              "%s: %s\r\n<p>%s: %s\r\n</p><hr><em>The %s web server</em>\r\n",
        myconfig.server_name, errornum, msg, msg2, cause, myconfig.server_name);

    sprintf(header, "HTTP/1.1 %s %s\r\nServer: %s\r\n"
                    "Content-type: text/html; charset=utf-8\r\n"
                    "Connection: close\r\nContent-length: %d\r\n\r\n",
            errornum, msg, myconfig.server_name, (int)strlen(body));
    send(clfd, header, strlen(header), 0);
    send(clfd, body, strlen(body), 0);
    return;
}

//出错返回0
static int parse_http_method(char *method_string, http_ret *ret) {
    if (!strcmp(method_string, "GET")) {
        ret->method = HTTP_METHOD_GET;
        return HTTP_METHOD_GET;
    } else if (!strcmp(method_string, "POST")) {
        ret->method = HTTP_METHOD_POST;
        return HTTP_METHOD_POST;
    } else {
        parse_error(ret->clfd, ret->path, "404", "Not Found", "Wrong Method!");
        return 0;
    }
}

//出错返回0
static int parse_http_version(char *version_string, http_ret *ret) {
    if (!strcmp(version_string, "HTTP/1.1")) {
        ret->version = 2;
        return 1;
    } else {
        parse_error(ret->clfd, ret->path, "404", "Not Found",
                    "Wrong HTTP Version!");
        return 0;
    }
}

//出错返回0
static int file_check(char *path_string, http_ret *ret) {
    char *path = ret->path;
    struct stat file;

    if (stat(path, &file)) {
        parse_error(ret->clfd, path, "404", "Not Found",
                    "xxx cannot find the file");
        return 0;
    }
    if (!(S_ISREG(file.st_mode)) || !(S_IRUSR & file.st_mode)) {
        parse_error(ret->clfd, path, "403", "Forbidden",
                    "xxx cannot open the file!");
        return 0;
    }
    ret->mtime = file.st_mtime;
    ret->filesize = file.st_size;
    ret->execable = S_IXUSR & file.st_mode;
    return 1;
}

//处理http请求，可能会改变传入的http_request字符数组的内容
static int parse_http_request(char *http_request, http_ret *ret) {
    char *method, *path, *version;
    int n = strlen(http_request); // http请求字符串的长度
    if (n == 0)
        return 0;

    ret->data = split_r_n(http_request);
    method = strsep(&http_request, " ");
    path = strsep(&http_request, " ");
    version = strsep(&http_request, "\r\n");
    if (http_request == NULL)
        return 0;
    *(http_request - 1) = 0;
    // log_info("%s\n%s\n%s\n", http_request, method, path);

    if (!parse_http_method(method, ret))
        return 0;
    if (!parse_http_path(path, ret))
        return 0;
    if (!parse_http_version(version, ret))
        return 0;
    if (!file_check(ret->path, ret))
        return 0;
    return 1;
}

//工作线程函数
//参数为建立连接的套接字
static void http_handle(char *buffer, int clfd) {
    char path[MAXLINE];
    http_ret ret;
    if (buffer == NULL)
        return;
    ret.clfd = clfd;
    ret.keep_alive = 0;
    ret.modified = 1;
    ret.path = path;
    if (!parse_http_request(buffer, &ret))
        return;
    ret.status = xxx_HTTP_OK;
    if (!ret.execable)
        serve_static(&ret);
    else
        serve_cgi(&ret);
}

static void handler(struct epoll_event *event) {
    assert(event);
    if ((event->events & EPOLLIN) && !(event->events & EPOLLERR)) {
        char buf[4096];
        int it = 0;
        for (int ret = 1; ret > 0;
             ret = read(event->data.fd, &buf[it], 4095 - it), it += ret)
            ;
        buf[it] = 0;
        // printf("string = \n%s\n", buf);
        http_handle(buf, event->data.fd);
    }
    close(event->data.fd);
}

static void *worker_thread(void *arg) {
    // printf("worker thread running!\n");
    threadarg_t *t = (threadarg_t *)arg;
    threadpool_t *tp = t->tp;
    assert(t && tp);

    while (tp->keep_running) {
        int n = epoll_wait(t->epfd, t->events, EVENT_SIZE, -1);
        // printf("n == %d\n", n);
        if (n <= 0)
            continue;
        for (int i = 0; i < n; i++) {
            handler(t->events + i);
        }
    }

    pthread_exit(NULL);
}

void threadpool_join(threadpool_t *tp) {
    assert(tp && tp->size > 0 && tp->tids);
    for (int i = 0; i < tp->size; i++) {
        pthread_join(tp->tids[i], NULL);
    }
    for (int i = 0; i < tp->size; i++) {
        pthread_mutex_destroy(&(tp->mutexs[i]));
        pthread_cond_destroy(&(tp->conds[i]));
        free(tp->args[i].events);
        close(tp->args[i].epfd);
    }
    free(tp->args);
    free(tp->mutexs);
    free(tp->conds);
    free(tp);
}

static void threadarg_init(threadarg_t *arg, threadpool_t *tp, pthread_t *tid,
                           pthread_mutex_t *mutex, pthread_cond_t *cond) {
    assert(arg && tid && mutex && cond && tp);
    arg->cond = cond;
    arg->mutex = mutex;
    arg->tid = tid;
    arg->tp = tp;
    arg->epfd = epoll_create1(EPOLL_CLOEXEC);
    arg->events = malloc(sizeof(struct epoll_event) * EVENT_SIZE);
}

threadpool_t *threadpool_create(size_t n) {
    assert(n > 0);

    threadpool_t *tp = malloc(sizeof(threadpool_t));
    memset(tp, 0, sizeof(threadpool_t));

    tp->mutexs = malloc(sizeof(pthread_mutex_t) * n);
    tp->conds = malloc(sizeof(pthread_cond_t) * n);
    tp->args = malloc(sizeof(threadarg_t) * n);
    tp->tids = malloc(sizeof(pthread_t) * n);
    tp->keep_running = 1;
    tp->size = n;

    for (int i = 0; i < n; i++) {
        pthread_mutex_init(&(tp->mutexs[i]), NULL);
        pthread_cond_init(&(tp->conds[i]), NULL);
        threadarg_init(&(tp->args[i]), tp, &(tp->tids[i]), &(tp->mutexs[i]),
                       &(tp->conds[i]));
        pthread_create(&(tp->tids[i]), NULL, worker_thread, &(tp->args[i]));
    }
    return tp;
}

void main_loop(threadpool_t *tp, int listenfd) {
    assert(tp && listenfd >= 0);
    int epfd = epoll_create1(EPOLL_CLOEXEC);
    struct epoll_event event;
    event.data.fd = listenfd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &event);

    for (int i = 0;;) {
        int nfds = epoll_wait(epfd, &event, 1, -1);
        if (nfds != 1)
            continue;
        while (1) {
            int clfd = accept(listenfd, NULL, NULL);
            if (clfd < 0) {
                if (errno == EAGAIN) {
                    break;
                } else {
                    fprintf(stderr, "accept error: %s\n", strerror(errno));
                    break;
                }
            }
            set_noblock(clfd);
            event.data.fd = clfd;
            event.events = EPOLLIN | EPOLLET;
            epoll_ctl(tp->args[i++].epfd, EPOLL_CTL_ADD, clfd, &event);
            // printf("push to epfd %d i = %d\n", tp->args[i - 1].epfd, i);
            if (i == tp->size)
                i = 0;
        }
    }
}

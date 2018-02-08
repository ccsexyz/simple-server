#include "config.h"
#include "server.h"
#include "xxx.h"

//跨文件的全局变量
server_config myconfig;
sigset_t mask;

int main(int argc, char **argv) {
    int err;
    sigset_t oldmask;
    pthread_t tid;

    sigemptyset(&mask);
    sigaddset(&mask, SIGPIPE); //向信号集中添加SIGPIPE
    if ((err = pthread_sigmask(SIG_BLOCK, &mask, &oldmask)) != 0)
        log_exit(err, "SIG_BLOCK error");
    err = pthread_create(&tid, NULL, sig_handler, 0);
    if (err != 0)
        log_exit(err, "Cannot create thread!");

    init_config();
    int listenfd = initserver();
    set_noblock(listenfd);
    threadpool_t *tp = threadpool_create(8);
    main_loop(tp, listenfd);

    threadpool_join(tp);
    exit(0);
}

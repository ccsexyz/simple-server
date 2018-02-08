#include "log.h"

static void log_doit(int errnoflag, int error, const char *fmt, va_list ap) {
    char buf[MAXLINE];
    vsnprintf(buf, MAXLINE - 1, fmt, ap);
    if (errnoflag) {
        snprintf(buf + strlen(buf), MAXLINE - strlen(buf) - 1, ": %s",
                 strerror(error));
    }
    strcat(buf, "\n");

    if (IS_LOG_STDIO) {
        fputs(buf, stdout);
    }

    if (IS_LOG_FILE) {
        fprintf(myconfig.server_fp, "%s", buf);
    }
}

void log_err(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    log_doit(1, errno, fmt, ap);
    va_end(ap);
}

void log_info(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    log_doit(0, 0, fmt, ap);
    va_end(ap);
}

void log_exit(int err, const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    log_doit(1, err, fmt, ap);
    va_end(ap);
    exit(1);
}

void log_quit(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    log_doit(0, 0, fmt, ap);
    va_end(ap);
    exit(1);
}

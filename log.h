#ifndef _LOG_H
#define _LOG_H
#include "comhdr.h"
#include <stdarg.h>
#include <syslog.h>

void log_err(const char *fmt, ...);
void log_info(const char *fmt, ...);
void log_exit(int err, const char *fmt, ...);
void log_quit(const char *fmt, ...);

#endif

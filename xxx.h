#ifndef _XXX_H_
#define _XXX_H_

#include "log.h"

#include <fcntl.h>
#include <limits.h>

#define BUFLEN 128
#define QLEN 10
#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 256
#endif

#define ROOT "/home/ccsexyz"

int initserver();
void set_noblock(int fd);
int initsocket(int type, const struct sockaddr *addr, socklen_t alen, int qlen);
int set_cloexec(int fd);
unsigned long file_length(int fd);
pvoid sig_handler(pvoid arg);

#endif

#ifndef CONFIG_H
#define CONFIG_H

#include "cJSON.h"
#include "comhdr.h"
#include "log.h"

#define CONFIG_PORT "port"
#define CONFIG_NAME "name"
#define CONFIG_ROOT "root"
#define CONFIG_LOG_FILE "logfile"
#define CONFIG_LOG_LEVEL "loglevel"

#define DEFAULT_CONFIG_PORT "9377"
#define DEFAULT_CONFIG_NAME "XXX-SERVER"
#define DEFAULT_CONFIG_ROOT "/home/xxx"
#define DEFAULT_CONFIG_LOG_LEVEL (1)

#define CONFIG_FILE_PATH "config.json"

int parse_config_string(const char *config_string,
                        psc p); // NULL == fail, >0 == success
int load_config_file(const char *filename, psc p);
void init_config(void);

#endif // CONFIG_H

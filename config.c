#include "config.h"
#include "xxx.h"

int parse_config_string(const char *config_string, psc p) {
    pvoid temp;

    cJSON *root = cJSON_Parse(config_string);
    if (root == NULL)
        goto parse_error;
    //获取端口号
    temp = cJSON_GetObjectItem(root, CONFIG_PORT);
    if (temp != NULL ||
        strlen(((cJSON *)temp)->valuestring) < SERVER_PORT_LENGTH)
        p->server_port = ((cJSON *)temp)->valuestring;
    //获取服务器名称
    temp = cJSON_GetObjectItem(root, CONFIG_NAME);
    if (temp != NULL ||
        strlen(((cJSON *)temp)->valuestring) < SERVER_NAME_LENGTH)
        p->server_name = ((cJSON *)temp)->valuestring;
    //获取服务器根目录
    temp = cJSON_GetObjectItem(root, CONFIG_ROOT);
    if (temp != NULL)
        p->server_root = ((cJSON *)temp)->valuestring;
    //获取日志文件路径，不需要向日志文件写入则不需要设置
    temp = cJSON_GetObjectItem(root, CONFIG_LOG_FILE);
    if (temp != NULL)
        p->server_logfile = ((cJSON *)temp)->valuestring;
    return 1;
parse_error:
    // cJSON_Delete(root); //不释放json结构所占用的空间
    return 0;
}

int load_config_file(const char *filename, psc p) {
    int fd = open(filename, O_RDONLY, 0);
    if (fd == -1) {
        fprintf(stderr, "thread %lx: open config file error!\n",
                pthread_self());
        goto errquit_withoutfd;
    }

    unsigned long filelength = file_length(fd);
    if (filelength == 0) {
        fprintf(stderr, "thread %lx: config file is empty!\n", pthread_self());
        goto errquit_withoutmmap;
    }

    char *fileaddr = mmap(NULL, filelength, PROT_READ, MAP_PRIVATE, fd, 0);
    if (fileaddr == MAP_FAILED) // mmap失败
        goto errquit_withoutfd;
    int ret = parse_config_string(fileaddr, p);
    munmap(fileaddr, filelength);

    if (ret == 0) {
        fprintf(stderr, "thread %lx: parse config file error!!\n",
                pthread_self());
        ;
    } else
        return 1;

errquit_withoutmmap: // mmap不成功
    close(fd);
errquit_withoutfd: //打开文件不成功
    return 0;
}

void init_config(void) {
    //设置服务器配置初始值
    memset(&myconfig, 0, sizeof(myconfig));
    myconfig.server_name = DEFAULT_CONFIG_NAME;
    myconfig.server_port = DEFAULT_CONFIG_PORT;
    myconfig.server_root = DEFAULT_CONFIG_ROOT;

    int ret = load_config_file(CONFIG_FILE_PATH, &myconfig);
    (void)ret;
    //如果设置了向日志文件路径则尝试打开
    if (myconfig.server_logfile != NULL) {
        if ((myconfig.server_fp = fopen(myconfig.server_logfile, "a")) ==
            NULL) {
            fprintf(stderr, "打开日志文件失败!\n");
        }
    }
}

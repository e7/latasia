/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#ifndef __LATASIA__CONF_H__
#define __LATASIA__CONF_H__


#include "adv_string.h"
#include "mem_pool.h"

// configuration {{
#define MAX_CONF_SIZE           65535
#define ENABLE_IPV6             FALSE
#define MODULE_POOL_SIZE        524288
#define CONN_POOL_SIZE          40960
#define CONN_BUFFER_SIZE        4096
#define MAX_CONNECTIONS         1024
// }} configuration



typedef struct lts_conf_s lts_conf_t;


struct lts_conf_s {
    int daemon; // 守护进程
    lts_str_t port; // 监听端口
    int workers; // 工作进程数
    int max_connections; // 单进程最大连接数
    lts_str_t pid_file; // pid文件
    lts_str_t log_file; // 日志
    int keepalive; // 连接超时时间

    lts_str_t http_cwd;
};


extern lts_conf_t lts_conf;
extern int lts_load_config(lts_conf_t *conf, lts_pool_t *pool);
#endif // __LATASIA__CONF_H__

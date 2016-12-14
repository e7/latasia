/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#ifndef __LATASIA__CONF_H__
#define __LATASIA__CONF_H__


#include "adv_string.h"
#include "mem_pool.h"
#include "file.h"


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// configuration {{
#define MAX_CONF_SIZE           65535
#define ENABLE_IPV6             FALSE
#define MODULE_POOL_SIZE        (1024 * 1024)
#define CONN_POOL_SIZE          (1024 * 32)
#define RBUF_SIZE               (1024 * 12)
#define SBUF_SIZE               (1024 * 4)
#define MAX_CONNECTIONS         1024
// }} configuration


typedef struct lts_conf_s lts_conf_t;
typedef struct lts_conf_sync_s lts_conf_sync_t;
typedef struct lts_conf_asyn_s lts_conf_asyn_t;
typedef struct lts_conf_lua_s lts_conf_lua_t;


struct lts_conf_s {
    int daemon; // 守护进程
    lts_str_t port; // 监听端口
    int workers; // 工作进程数
    int max_connections; // 单进程最大连接数
    int keepalive; // 连接超时时间
    lts_str_t pid_file; // pid文件
    lts_str_t log_file; // 日志
    lts_str_t app_mod_conf; // 应用模块配置文件
};

struct lts_conf_sync_s {
    lts_str_t host;
    uintptr_t port;
};

struct lts_conf_asyn_s {
    lts_str_t host;
    uintptr_t port;
};

struct lts_conf_lua_s {
    lts_str_t search_path;
    lts_str_t search_cpath;
    lts_str_t entry;
};


typedef struct {
    lts_str_t name;
    void (*match_handler)(void *conf,
                          lts_str_t *key,
                          lts_str_t *value,
                          lts_pool_t *pool);
} lts_conf_item_t;


extern lts_conf_t lts_main_conf;
extern lts_conf_sync_t lts_sync_conf;
extern lts_conf_asyn_t lts_asyn_conf;
extern lts_conf_lua_t lts_lua_conf;

extern int load_conf_file(lts_file_t *file, uint8_t **addr, off_t *sz);

// json配置解析，将配置文件内容转换为相应的配置结构体
// addr, sz: 配置文件内容
// conf_items: 需要处理的配置项
// pool: 内存池
// conf: 配置输出结构体
extern int parse_conf(uint8_t *addr, off_t sz,
                      lts_conf_item_t *conf_items[],
                      lts_pool_t *pool, void *conf);

extern void close_conf_file(lts_file_t *file, uint8_t *addr, off_t sz);
#ifdef __cplusplus
}
#endif // __cplusplus
#endif // __LATASIA__CONF_H__

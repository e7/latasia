/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#include "adv_string.h"
#include "mem_pool.h"
#include "conf.h"

#define MAX_KEEPALIVE       (24 * 60 * 60)


// 默认配置
lts_conf_t lts_main_conf = {
    FALSE, // 守护进程
    lts_string("6742"), // 监听端口
    1, // slave进程数
    MAX_CONNECTIONS, // 每个slave最大连接数
    60, // 连接超时
    lts_string("run/latasia.pid"), // pid文件路径
    lts_string("run/latasia.log"), // 日志路径
    lts_string("conf/mod_app.conf"), // 应用模块配置文件路径
};


// match of daemon
static void
cb_daemon_match(void *c, lts_str_t *k, lts_str_t *v, lts_pool_t *pool)
{
    uint8_t *item_buf;
    size_t item_buf_size = MAX(v->len, 8) + 1;
    lts_conf_t *conf = (lts_conf_t *)c;

    // 缓冲value
    item_buf  = (uint8_t *)lts_palloc(pool, item_buf_size);
    (void)memcpy(item_buf, v->data, v->len);
    item_buf[v->len] = 0;

    conf->daemon = atoi((char const *)item_buf);

    return;
}


// match of port
static void
cb_port_match(void *c, lts_str_t *k, lts_str_t *v, lts_pool_t *pool)
{
    int nport;
    uint8_t *port_buf;
    size_t port_buf_size = MAX(v->len, 8) + 1;
    lts_conf_t *conf = (lts_conf_t *)c;

    // 缓冲value
    port_buf  = (uint8_t *)lts_palloc(pool, port_buf_size);
    (void)memcpy(port_buf, v->data, v->len);
    port_buf[v->len] = 0;

    // 检查端口合法性
    nport = atoi((char const *)port_buf);
    if ((nport < 1) || (nport > 65535)) {
        nport = 6742;
    }

    // 更新配置
    lts_str_init(&conf->port, port_buf, port_buf_size - 1);
    assert(0 == lts_l2str(&conf->port, nport));

    return;
}


// match of pid_file
static void cb_pid_file_match(void *c,
                              lts_str_t *k,
                              lts_str_t *v,
                              lts_pool_t *pool)
{
    uint8_t *pid_file_buf;
    lts_conf_t *conf = (lts_conf_t *)c;
    size_t pid_file_buf_size = MAX(v->len, 8) + 1;

    // 缓冲value
    pid_file_buf  = (uint8_t *)lts_palloc(pool, pid_file_buf_size);
    (void)memcpy(pid_file_buf, v->data, v->len);
    pid_file_buf[v->len] = 0;

    // 更新配置
    lts_str_init(&conf->pid_file, pid_file_buf, v->len);

    return;
}


// match of log_file
static void cb_log_file_match(void *c,
                              lts_str_t *k,
                              lts_str_t *v,
                              lts_pool_t *pool)
{
    uint8_t *log_file_buf;
    lts_conf_t *conf = (lts_conf_t *)c;
    size_t log_file_buf_size = MAX(v->len, 8) + 1;

    // 缓冲value
    log_file_buf  = (uint8_t *)lts_palloc(pool, log_file_buf_size);
    (void)memcpy(log_file_buf, v->data, v->len);
    log_file_buf[v->len] = 0;

    // 更新配置
    lts_str_init(&conf->log_file, log_file_buf, v->len);

    return;
}


// match of workers_file
static void cb_workers_match(void *c,
                             lts_str_t *k,
                             lts_str_t *v,
                             lts_pool_t *pool)
{
    int nworkers;
    uint8_t *port_buf;
    lts_conf_t *conf = (lts_conf_t *)c;
    size_t port_buf_size = MAX(v->len, 8) + 1;

    // 缓冲value
    port_buf  = (uint8_t *)lts_palloc(pool, port_buf_size);
    (void)memcpy(port_buf, v->data, v->len);
    port_buf[v->len] = 0;

    // 更新配置
    nworkers = atoi((char const *)port_buf);
    if ((nworkers < 1) || (nworkers > 512)) {
        nworkers = 1;
    }
    conf->workers = nworkers;

    return;
}


// match of keepalive
static void cb_keepalive_match(void *c,
                               lts_str_t *k,
                               lts_str_t *v,
                               lts_pool_t *pool)
{
    int nkeepalive;
    uint8_t *item_buf;
    lts_conf_t *conf = (lts_conf_t *)c;
    size_t item_buf_size = MAX(v->len, 8) + 1;

    // 缓冲value
    item_buf  = (uint8_t *)lts_palloc(pool, item_buf_size);
    (void)memcpy(item_buf, v->data, v->len);
    item_buf[v->len] = 0;

    // 更新配置
    nkeepalive = atoi((char const *)item_buf);
    if ((nkeepalive < 0) || (nkeepalive > MAX_KEEPALIVE)) { // 不超过一天
        nkeepalive = MAX_KEEPALIVE;
    }
    conf->keepalive = nkeepalive;

    return;
}


// match of max_connections
static void cb_max_connections_match(void *c,
                                     lts_str_t *k,
                                     lts_str_t *v,
                                     lts_pool_t *pool)
{
    int max_connections;
    uint8_t *item_buf;
    lts_conf_t *conf = (lts_conf_t *)c;
    size_t item_buf_size = MAX(v->len, 8) + 1;

    // 缓冲value
    item_buf  = (uint8_t *)lts_palloc(pool, item_buf_size);
    (void)memcpy(item_buf, v->data, v->len);
    item_buf[v->len] = 0;

    // 更新配置
    max_connections = atoi((char const *)item_buf);
    if (max_connections < 1) {
        max_connections = 1;
    }
    conf->max_connections = max_connections;

    return;
}


// match of app_mod_conf
static void cb_app_mod_conf_match(void *c,
                                  lts_str_t *k,
                                  lts_str_t *v,
                                  lts_pool_t *pool)
{
    uint8_t *item_buf;
    lts_conf_t *conf = (lts_conf_t *)c;
    size_t item_buf_size = MAX(v->len, 8) + 1;

    // 缓冲value
    item_buf  = (uint8_t *)lts_palloc(pool, item_buf_size);
    (void)memcpy(item_buf, v->data, v->len);
    item_buf[v->len] = 0;

    // 更新配置
    lts_str_init(&conf->app_mod_conf, item_buf, v->len);

    return;
}


lts_conf_item_t *g_conf_main_items[] = {
    &(lts_conf_item_t){lts_string("daemon"), &cb_daemon_match},
    &(lts_conf_item_t){lts_string("port"), &cb_port_match},
    &(lts_conf_item_t){lts_string("workers"), &cb_workers_match},
    &(lts_conf_item_t){lts_string("pid_file"), &cb_pid_file_match},
    &(lts_conf_item_t){lts_string("log_file"), &cb_log_file_match},
    &(lts_conf_item_t){lts_string("keepalive"), &cb_keepalive_match},
    &(lts_conf_item_t){
        lts_string("max_connections"), &cb_max_connections_match
    },
    &(lts_conf_item_t){lts_string("app_mod_conf"), &cb_app_mod_conf_match},
    NULL,
};

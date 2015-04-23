/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#ifndef __LATASIA__LTS_H__
#define __LATASIA__LTS_H__


#include "adv_string.h"
#include "socket.h"
#include "shmem.h"


#define LTS_MAX_PROCESSES       1024


enum {
    LTS_MASTER = 0,
    LTS_SLAVE,
};

enum {
    LTS_CORE_MODULE = 1,
    LTS_EVENT_MODULE,
    LTS_APP_MODULE,
};


typedef struct lts_module_s lts_module_t;
typedef struct lts_event_module_itfc_s lts_event_module_itfc_t;
typedef struct lts_app_module_itfc_s lts_app_module_itfc_t;
typedef struct lts_process_s lts_process_t;


// 进程类型
struct lts_process_s {
    pid_t pid; // 工作进程id
    int channel[2];
};

// 模块类型
struct lts_module_s {
    lts_str_t   name; // 模块名称
    int         type; // 模块类型
    void        *itfc; // 特定模块类型的接口
    lts_pool_t  *pool; // 模块内存池
    void        *ctx;
    int         (*init_master)(lts_module_t *);
    int         (*init_worker)(lts_module_t *);
    void        (*exit_worker)(lts_module_t *);
    void        (*exit_master)(lts_module_t *);
};


// 事件模块接口
struct lts_event_module_itfc_s {
    int (*event_add)(lts_socket_t *);
    int (*event_del)(lts_socket_t *);
    int (*process_events)(void);
};


// app模块接口
struct lts_app_module_itfc_s {
    int (*process_iobuf)(lts_socket_t *);
};


extern void lts_recv(lts_socket_t *cs);
extern void lts_send(lts_socket_t *cs);
extern void lts_close_conn(int fd, lts_pool_t *c, int reset);


extern size_t lts_sys_pagesize; // 系统内存页
extern long lts_signals_mask; // 信号掩码

extern int lts_module_count; // 模块计数
extern lts_module_t lts_core_module; // 核心模块
extern lts_module_t lts_event_core_module; // 事件核心模块
extern lts_module_t lts_event_epoll_module; // epoll事件模块
extern lts_module_t lts_app_pb_parse_module; // protobuf消息解析模块
extern lts_module_t lts_app_http_core_module; // http core模块

extern lts_socket_t *lts_channel;
extern int lts_ps_slot;
extern lts_process_t lts_processes[LTS_MAX_PROCESSES]; // 进程组
extern lts_event_module_itfc_t *lts_event_itfc; // 事件模块接口
extern lts_shm_t lts_accept_lock; // accept锁
extern int lts_accept_lock_hold;
extern int lts_use_accept_lock;
extern pid_t lts_pid; // 进程id
extern int lts_process_role; // 进程角色
#endif // __LATASIA__LTS_H__

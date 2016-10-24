/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#ifndef __LATASIA__LTS_H__
#define __LATASIA__LTS_H__


#include "adv_string.h"
#include "socket.h"
#include "shmem.h"


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define LTS_MAX_PATH_LEN        256
#define LTS_MAX_PROCESSES       1024


enum {
    LTS_MASTER = 0,
    LTS_SLAVE,
};

enum {
    LTS_CORE_MODULE = 1,
    LTS_EVENT_MODULE,
    LTS_PROTO_MODULE,
    LTS_APP_MODULE,
};


typedef struct lts_status_machine_s lts_sm_t;
typedef struct lts_module_s lts_module_t;
typedef struct lts_event_module_itfc_s lts_event_module_itfc_t;
typedef struct lts_app_module_itfc_s lts_app_module_itfc_t;
typedef struct lts_process_s lts_process_t;


// 全局状态机
enum {
    LTS_CHANNEL_SIGEXIT = 1001,
};
struct lts_status_machine_s {
    uint32_t channel_signal;
    int unix_domain_enabled[LTS_MAX_PROCESSES];
};

// 进程类型
struct lts_process_s {
    pid_t ppid; // 父进程id
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
    lstack_t    s_node;
};


// 事件模块接口
struct lts_event_module_itfc_s {
    void (*event_add)(lts_socket_t *);
    void (*event_mod)(lts_socket_t *);
    void (*event_del)(lts_socket_t *);
    int (*process_events)(void);
};


// app模块接口
struct lts_app_module_itfc_s {
    void (*service)(lts_socket_t *);
    void (*send_more)(lts_socket_t *);
};


extern void lts_recv(lts_socket_t *cs);
extern void lts_send(lts_socket_t *cs);
extern void lts_timeout(lts_socket_t *cs);
extern void lts_close_conn_orig(int fd, int reset);
extern void lts_close_conn(lts_socket_t *cs, int reset);


extern lts_sm_t lts_global_sm; // 全局状态机
extern lts_str_t lts_cwd; // 当前工作目录
extern size_t lts_sys_pagesize; // 系统内存页
extern lts_atomic_t lts_signals_mask; // 信号掩码

extern int lts_module_count; // 模块计数
extern lts_module_t *lts_module_event_cur; // 当前事件模块
extern lts_module_t *lts_module_app_cur; // 当前应用模块

extern lts_module_t lts_core_module; // 核心模块
extern lts_module_t lts_event_core_module; // 事件核心模块
extern lts_module_t lts_event_epoll_module; // epoll事件模块
// app模块 {{
extern lts_module_t lts_app_asyn_backend_module; // 异步后端框架模块
extern lts_module_t lts_app_echo_module; // echo模块
extern lts_module_t lts_app_http_core_module; // http core模块
extern lts_module_t lts_app_sjsonb_module; // sjsonb协议demo模块
// }} app模块

extern lts_socket_t *lts_channels[LTS_MAX_PROCESSES];
extern int lts_ps_slot;
extern lts_process_t lts_processes[LTS_MAX_PROCESSES]; // 进程组
extern lts_event_module_itfc_t *lts_event_itfc; // 事件模块接口
extern lts_shm_t lts_accept_lock; // accept锁
extern int lts_accept_lock_hold;
extern int lts_use_accept_lock;
extern pid_t lts_pid; // 进程id
extern int lts_process_role; // 进程角色

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // __LATASIA__LTS_H__

/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#include <inttypes.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include "latasia.h"
#include "logger.h"
#include "conf.h"
#include "vsignal.h"


long lts_sys_pagesize;
long lts_signals_mask; // 信号掩码

int lts_module_count; // 模块计数
lts_socket_t *lts_channel;
int lts_ps_slot;
lts_process_t lts_processes[LTS_MAX_PROCESSES]; // 进程组
lts_shm_t lts_accept_lock = {
    NULL,
    128,
};
int lts_accept_lock_hold = FALSE;
int lts_use_accept_lock;
pid_t lts_pid;
int lts_process_role;
lts_module_t *lts_modules[] = {
    &lts_core_module,
    &lts_event_core_module,
    &lts_event_epoll_module,
    &lts_app_http_core_module,
    NULL,
};


static int lts_shmtx_trylock(lts_atomic_t *lock);
static void lts_shmtx_unlock(lts_atomic_t *lock);
static int enable_accept_events(void);
static int disable_accept_events(void);
static void process_post_sock_list(void);
static int event_loop_single(void);
static int event_loop_multi(void);
static int worker_main(void);
static int master_main(void);


// gcc spinlock {{
int lts_spin_trylock(sig_atomic_t *v)
{
    return  (0 == __sync_lock_test_and_set(v, 1));
}

void lts_spin_lock(sig_atomic_t *v)
{
    while(! lts_spin_trylock(v)) {
        LTS_CPU_PAUSE();
    }
}

void lts_spin_unlock(sig_atomic_t *v)
{
    __sync_lock_release(v);
    return;
}
// }} gcc spinlock

int lts_shmtx_trylock(lts_atomic_t *lock)
{
    sig_atomic_t val;

    val = *lock;

    return (((val & 0x80000000) == 0)
                && lts_atomic_cmp_set(lock, val, val | 0x80000000));
}


void lts_shmtx_unlock(lts_atomic_t *lock)
{
    sig_atomic_t val, old, wait;

    while (TRUE) {
        old = *lock;
        wait = old & 0x7fffffff;
        val = wait ? wait - 1 : 0;

        if (lts_atomic_cmp_set(lock, old, val)) {
            break;
        }
    }

    return;
}


int enable_accept_events(void)
{
    int rslt;

    rslt = 0;
    if (lts_accept_lock_hold) {
        return rslt;
    }

    lts_accept_lock_hold = TRUE;
    dlist_for_each_f(pos, &lts_listen_sock_list) {
        lts_socket_t *ls;

        ls = CONTAINER_OF(pos, lts_socket_t, dlnode);
        if (0 != (*lts_event_itfc->event_add)(ls)) {
            rslt = LTS_E_SYS;
            break;
        }
    }

    return rslt;
}


int disable_accept_events(void)
{
    int rslt;

    rslt = 0;
    if (!lts_accept_lock_hold) {
        return rslt;
    }

    lts_accept_lock_hold = FALSE;
    dlist_for_each_f(pos, &lts_listen_sock_list) {
        lts_socket_t *ls;

        ls = CONTAINER_OF(pos, lts_socket_t, dlnode);
        if (0 != (*lts_event_itfc->event_del)(ls)) {
            rslt = LTS_E_SYS;
            break;
        }
    }

    return rslt;
}


void process_post_sock_list(void)
{
    int i;
    lts_module_t *module;
    lts_app_module_itfc_t *app_itfc;

    dlist_for_each_f_safe(pos, cur_next, &lts_post_list) {
        lts_socket_t *cs;

        cs = CONTAINER_OF(pos, lts_socket_t, dlnode);
        if (cs->readable && cs->do_read) {
            (*cs->do_read)(cs);
        }

        if (NULL == cs->conn) { // 非连接套接字
            continue;
        }

        if (cs->closing) {
            lts_close_conn(cs->fd, cs->conn->pool, cs->closing & (1 << 1));
            lts_post_list_del(cs);
            lts_free_socket(cs);
            continue;
        }

        for (i = 0; lts_modules[i]; ++i) {
            module = lts_modules[i];

            if (LTS_APP_MODULE != module->type) {
                continue;
            }

            app_itfc = (lts_app_module_itfc_t *)module->itfc;
            if (NULL == app_itfc) {
                continue;
            }

            if (0 != (*app_itfc->process_iobuf)(cs)) {
                break;
            }
        }

        if (cs->closing) {
            lts_close_conn(cs->fd, cs->conn->pool, cs->closing & (1 << 1));
            lts_post_list_del(cs);
            lts_free_socket(cs);
            continue;
        }

        if (cs->writable && cs->do_read) {
            (*cs->do_write)(cs);
        }

        if (cs->closing) {
            lts_close_conn(cs->fd, cs->conn->pool, cs->closing & (1 << 1));
            lts_post_list_del(cs);
            lts_free_socket(cs);
            continue;
        }

        if (0 == (cs->writable | cs->readable)) {
            lts_post_list_del(cs);
            lts_sock_list_add(cs);
        }
    }

    return;
}


int event_loop_single(void)
{
    int rslt, hold;

    // 事件循环
    hold = FALSE;
    while (TRUE) {
        // 更新进程负载
        if (lts_signals_mask & LTS_MASK_SIGSTOP) {
            lts_accept_disabled = 1;
            if (dlist_empty(&lts_post_list)) {
                // 已处理完所有连接
                (void)lts_write_logger(&lts_file_logger, LTS_LOG_INFO,
                                       "worker will exit\n");
                break;
            }
        } else {
            lts_accept_disabled = lts_sock_inuse_n / 8 - lts_sock_cache_n;
        }

        if (lts_accept_disabled < 0) {
            if (!hold) {
                dlist_for_each_f(pos, &lts_listen_sock_list) {
                    lts_socket_t *ls;

                    ls = CONTAINER_OF(pos, lts_socket_t, dlnode);
                    if (0 != (*lts_event_itfc->event_add)(ls)) {
                        // log
                    }
                }

                hold = TRUE;
            }
        } else {
            if (hold) {
                dlist_for_each_f(pos, &lts_listen_sock_list) {
                    lts_socket_t *ls;

                    ls = CONTAINER_OF(pos, lts_socket_t, dlnode);
                    if (0 != (*lts_event_itfc->event_del)(ls)) {
                        // log
                    }
                }

                hold = FALSE;
            }
        }

        rslt = (*lts_event_itfc->process_events)();

        if (0 != rslt) {
            break;
        }

        process_post_sock_list();
    }

    if (0 != rslt) {
        return -1;
    }

    return 0;
}

int event_loop_multi(void)
{
    int rslt;

    // 事件循环
    while (TRUE) {
        // 更新进程负载
        if (lts_signals_mask & LTS_MASK_SIGSTOP) {
            lts_accept_disabled = 1;
            if (dlist_empty(&lts_post_list)) {
                // 已处理完所有连接
                (void)lts_write_logger(&lts_file_logger, LTS_LOG_INFO,
                                       "worker will exit\n");
                break;
            }
        } else {
            lts_accept_disabled = lts_sock_inuse_n / 8 - lts_sock_cache_n;
        }

        if (lts_accept_disabled < 0) {
            if (lts_shmtx_trylock((lts_atomic_t *)lts_accept_lock.addr)) {
                // 抢锁成功
                if (0 != enable_accept_events()) {
                    continue;
                }
            } else {
                if (0 != disable_accept_events()) {
                    continue;
                }
            }
        } else {
            if (0 != disable_accept_events()) {
                continue;
            }
        }

        rslt = (*lts_event_itfc->process_events)();
        if (lts_accept_lock_hold) {
            lts_shmtx_unlock((lts_atomic_t *)lts_accept_lock.addr);
        }
        if (0 != rslt) {
            break;
        }

        process_post_sock_list();
    }

    if (0 != rslt) {
        return -1;
    }

    return 0;
}


static
void close_socketpair(int channel[2])
{
    if (-1 == close(channel[0])) {
    }
    if (-1 == close(channel[1])) {
    }

    return;
}


static
pid_t wait_children(void)
{
    pid_t child;
    int status, slot;

    child = waitpid(-1, &status, WNOHANG);
    if (child <= 0) {
        return child;
    }

    // 有子进程退出
    for (slot = 0; slot < lts_conf.workers; ++slot) {
        if (child == lts_processes[slot].pid) {
            lts_processes[slot].pid = -1;
            if (-1 == close(lts_processes[slot].channel[0])) {
                // log
            }
            break;
        }
    }
    if (WIFSIGNALED(status)) {
        (void)lts_write_logger(
            &lts_file_logger, LTS_LOG_WARN,
            "child process %d terminated by %d\n",
            (long)child, WTERMSIG(status)
        );
    }
    if (WIFEXITED(status)) {
        (void)lts_write_logger(
            &lts_file_logger, LTS_LOG_INFO,
            "child process %d exit with code %d\n",
            (long)child, WEXITSTATUS(status)
        );
    }

    return child;
}


int master_main(void)
{
    pid_t child;
    int workers, slot;
    sigset_t tmp_mask;

    // 初始化lts_processes
    for (slot = 0; slot < lts_conf.workers; ++slot) {
        int fd;

        lts_processes[slot].pid = -1;
        if (-1 == socketpair(AF_UNIX, SOCK_STREAM,
                             0, lts_processes[slot].channel))
        {
            (void)lts_write_logger(
                &lts_file_logger, LTS_LOG_ERROR, "socketpair failed\n"
            );
            break;
        }

        fd = lts_processes[slot].channel[0];
        if (-1 == fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK)) {
            close_socketpair(lts_processes[slot].channel);
            break;
        }
        fd = lts_processes[slot].channel[1];
        if (-1 == fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK)) {
            close_socketpair(lts_processes[slot].channel);
            break;
        }
    }
    if (slot< lts_conf.workers) {
        while (slot-- > 0) {
            close_socketpair(lts_processes[slot].channel);
        }
        return -1;
    }

    // 初始化全局变量
    lts_pid = getpid();
    lts_process_role = LTS_MASTER;
    if (lts_conf.workers > 1) {
        lts_use_accept_lock = TRUE;
    } else {
        lts_use_accept_lock = FALSE;
    }

    workers = 0; // 当前工作进程数
    while (TRUE) {
        if (0 == (lts_signals_mask & LTS_MASK_SIGSTOP)) {
            // 重启工作进程
            while (workers  < lts_conf.workers) {
                pid_t p;

                // 寻找空闲槽
                for (slot = 0; slot < lts_conf.workers; ++slot) {
                    if (-1 == lts_processes[slot].pid) {
                        break;
                    }
                }
                assert(slot < lts_conf.workers);

                p = fork();

                if (-1 == p) {
                    (void)lts_write_logger(
                        &lts_file_logger, LTS_LOG_ERROR, "fork failed\n"
                    );
                    break;
                }

                if (0 == p) {
                    lts_pid = getpid();
                    lts_process_role = LTS_SLAVE;
                    lts_ps_slot = slot; // 新进程槽号
                    break;
                }

                lts_processes[slot].pid = p;
                ++workers;
            }

            if (LTS_SLAVE == lts_process_role) {
                _exit(worker_main());
            }
        }

        sigemptyset(&tmp_mask);
        (void)sigsuspend(&tmp_mask);

        if (lts_signals_mask & (LTS_MASK_SIGSTOP | LTS_MASK_SIGCHLD)) {
            if (lts_signals_mask & LTS_MASK_SIGSTOP) {
                for (int i = 0; i < lts_conf.workers; ++i) {
                    if (-1 == lts_processes[i].pid) {
                        continue;
                    }
                    if (-1 == kill(lts_processes[i].pid, SIGINT)) {
                        // log
                    }
                }
            }

            // 等待子进程退出
            child = wait_children();
            if (-1 == child) {
                assert(ECHILD == errno);
                (void)lts_write_logger(&lts_file_logger, LTS_LOG_INFO,
                                       "no more children to wait\n");
            }
            --workers;

            if (lts_signals_mask & LTS_MASK_SIGSTOP) {
                break;
            }
        }

        //清除信号标识
        lts_signals_mask &= ~LTS_MASK_SIGSTOP;
    }

    return 0;
}


static void do_exit_worker(int type, int last)
{
    lts_module_t *module;

    while (last > 0) {
        module = lts_modules[last--];

        if (type != module->type) {
            continue;
        }

        if (NULL == module->exit_worker) {
            continue;
        }

        (*module->exit_worker)(module);
    }

    return;
}

static int do_init_worker(int type)
{
    int i;
    lts_module_t *module;


    for (i = 0; lts_modules[i]; ++i) {
        module = lts_modules[i];

        if (type != module->type) {
            continue;
        }

        if (NULL == module->init_worker) {
            continue;
        }

        if (0 != (*module->init_worker)(module)) {
            break;
        }
    }

    if (lts_modules[i]) {
        do_exit_worker(type, i - 1);
        return -1;
    }

    return 0;
}

int worker_main(void)
{
    int i, rslt;

    // 初始化核心模块
    if (-1 == do_init_worker(LTS_CORE_MODULE)) {
        return EXIT_FAILURE;
    }

    // 初始化事件模块
    if (-1 == do_init_worker(LTS_EVENT_MODULE)) {
        do_exit_worker(LTS_CORE_MODULE, lts_module_count - 1);
        return EXIT_FAILURE;
    }

    // 初始化app模块
    if (-1 == do_init_worker(LTS_APP_MODULE)) {
        do_exit_worker(LTS_EVENT_MODULE, lts_module_count - 1);
        do_exit_worker(LTS_CORE_MODULE, lts_module_count - 1);
        return EXIT_FAILURE;
    }

    // 使用第一个初始化成功的事件模块
    for (i = 0; lts_modules[i]; ++i) {
        if (LTS_EVENT_MODULE == lts_modules[i]->type) {
            lts_event_itfc = (lts_event_module_itfc_t *)lts_modules[i]->itfc;
            break;
        }
    }

    // 事件循环
    (void)lts_write_logger(&lts_file_logger, LTS_LOG_INFO,
                           "start successfully\n");
    // (*lts_event_itfc->event_add)(lts_channel);
    if (lts_use_accept_lock) {
        rslt = event_loop_multi();
    } else {
        rslt = event_loop_single();
    }

    // 析构app模块
    do_exit_worker(LTS_APP_MODULE, lts_module_count - 1);

    // 析构事件模块
    do_exit_worker(LTS_EVENT_MODULE, lts_module_count - 1);

    // 析构核心模块
    do_exit_worker(LTS_CORE_MODULE, lts_module_count - 1);

    return (0 == rslt) ? EXIT_SUCCESS : EXIT_FAILURE;
}


int main(int argc, char *argv[], char *env[])
{
    int i, rslt;
    lts_module_t *module;

    // 全局初始化
    lts_init_log_prefixes();

    // 初始化核心模块
    for (i = 0; lts_modules[i]; ++i) {
        module = lts_modules[i];

        if (LTS_CORE_MODULE != module->type) {
            continue;
        }

        if (NULL == module->init_master) {
            continue;
        }

        if (0 != (*module->init_master)(module)) {
            break;
        }
    }

    if (NULL == lts_modules[i]) {
        lts_module_count = i;
        rslt = (0 == master_main()) ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    // 析构核心模块
    while (i > 0) {
        module = lts_modules[--i];

        if (LTS_CORE_MODULE != module->type) {
            continue;
        }

        if (NULL == module->exit_master) {
            continue;
        }

        (*module->exit_master)(module);
    }

    return rslt;
}

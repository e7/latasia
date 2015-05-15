/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#include <inttypes.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/epoll.h>

#include "latasia.h"
#include "logger.h"
#include "conf.h"
#include "vsignal.h"
#include "rbt_timer.h"


static uint8_t __lts_cwd_buf[LTS_MAX_PATH_LEN];
static char const *__lts_errno_desc[] = {
    // 0 ~ 9
    "success [0]",
    "operation not permitted [1]",
    "no such file or directory [2]",
    "no such process [3]",
    "interrupted system call [4]",
    "input or output error [5]",
    "no such device or address [6]",
    "argument list too long [7]",
    "exec format error [8]",
    "bad file descriptor [9]",

    // 10 ~ 19
    "no child process [10]",
    "resource temporarily unavailable [11]",
    "can not allocate memory[12]",
    "permission denied [13]",
    "bad address [14]",
    "block device required [15]",
    "device or resource busy [16]",
    "already exists [17]",
    "invalid cross-device link [18]",
    "no such device [19]",

    // 20 ~ 29
    "not a directory [20]",
    "is a directory [21]",
    "invalid argument [22]",
    "too many open files in system [23]",
    "too many open files in process [24]",
    "inappropriate ioctl for device [25]",
    "text file busy [26]",
    "file too large [27]",
    "no space left on device [28]",
    "illegal seek [29]",

    // 30 ~ 39
    "read-only file system [30]",
    "too many links [31]",
    "broken pipe [32]",
    NULL,
    NULL,
    "resource deadlock avoided [35]",
    NULL,
    NULL,
    NULL,
    NULL,

    // 40 ~ 49
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    // 50 ~ 59
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    // 60 ~ 69
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    // 70 ~ 79
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    // 80 ~ 89
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    // 90 ~ 99
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "address already in use [98]",
    NULL,

    // 100 ~ 109
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "Transport endpoint is not connected [107]",
    NULL,
    NULL,

    // 110 ~ 119
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    // 120 ~ 129
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    // 130 ~ 139
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};


char const **lts_errno_desc = __lts_errno_desc;
lts_sm_t lts_global_sm; // 全局状态机
lts_str_t lts_cwd = {__lts_cwd_buf, 0,}; // 当前工作目录
size_t lts_sys_pagesize;
lts_atomic_t lts_signals_mask; // 信号掩码

int lts_module_count; // 模块计数
lts_socket_t *lts_channels[LTS_MAX_PROCESSES];
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
static void enable_accept_events(void);
static void disable_accept_events(void);
static void process_post_list(void);
static int event_loop_single(void);
static int event_loop_multi(void);
static int worker_main(void);
static int master_main(void);


// gcc spinlock {{
int lts_spin_trylock(sig_atomic_t *v)
{
    int rslt;

    rslt = (0 == __sync_lock_test_and_set(v, 1));
    LTS_MEMORY_FENCE();

    return rslt;
}

void lts_spin_lock(sig_atomic_t *v)
{
    while(! lts_spin_trylock(v)) {
        LTS_CPU_PAUSE();
    }
}

void lts_spin_unlock(sig_atomic_t *v)
{
    LTS_MEMORY_FENCE();
    __sync_lock_release(v);
    return;
}
// }} gcc spinlock

int lts_shmtx_trylock(lts_atomic_t *lock)
{
    int rslt;
    sig_atomic_t val;

    val = *lock;

    rslt = (
        ((val & 0x80000000) == 0)
            && lts_atomic_cmp_set(lock, val, val | 0x80000000)
    );
    LTS_MEMORY_FENCE();

    return rslt;
}


void lts_shmtx_unlock(lts_atomic_t *lock)
{
    sig_atomic_t val, old, wait;

    LTS_MEMORY_FENCE();
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


void enable_accept_events(void)
{
    if (lts_accept_lock_hold) {
        return;
    }

    lts_accept_lock_hold = TRUE;
    dlist_for_each_f(pos, &lts_listen_list) {
        lts_socket_t *ls;

        ls = CONTAINER_OF(pos, lts_socket_t, dlnode);
        (*lts_event_itfc->event_add)(ls);
    }

    return;
}


void disable_accept_events(void)
{
    if (!lts_accept_lock_hold) {
        return;
    }

    lts_accept_lock_hold = FALSE;
    dlist_for_each_f(pos, &lts_listen_list) {
        lts_socket_t *ls;

        ls = CONTAINER_OF(pos, lts_socket_t, dlnode);
        (*lts_event_itfc->event_del)(ls);
    }

    return;
}


void process_post_list(void)
{
    int i;
    lts_module_t *module;
    lts_app_module_itfc_t *app_itfc;

    // 寻找app模块
    module = NULL;
    for (i = 0; lts_modules[i]; ++i) {
        module = lts_modules[i];

        if (LTS_APP_MODULE != module->type) {
            continue;
        }
    }
    if (NULL == module) {
        return;
    }

    // 获取app接口
    app_itfc = (lts_app_module_itfc_t *)module->itfc;
    if (NULL == app_itfc) {
        return;
    }

    // 处理事件
    dlist_for_each_f_safe(pos, cur_next, &lts_post_list) {
        lts_socket_t *cs;

        cs = CONTAINER_OF(pos, lts_socket_t, dlnode);

        // 监听套接字
        if (NULL == cs->conn) {
            if (cs->readable && cs->do_read) {
                (*cs->do_read)(cs);
            }
            continue;
        }

        if (cs->readable && cs->do_read) {
            if (-1 == (*cs->do_read)(cs)) {
                continue;
            }
        }

        if (! cs->writable) {
            if (cs->more && (0 == (cs->ev_mask & EPOLLOUT))) {
                cs->ev_mask |= EPOLLOUT;
                (*lts_event_itfc->event_mod)(cs);
            }
        } else if (cs->do_write) {
            if (-1 == (*cs->do_write)(cs)) {
                continue;
            }
        } else {
            abort();
        }

        if (cs->timeoutable && cs->do_timeout) {
            if (-1 == (*cs->do_timeout)(cs)) {
                continue;
            }
        }

        // 移出post链
        if ((! cs->readable) && (! cs->writable) && (! cs->timeoutable)) {
            lts_conn_list_add(cs);
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
        // 检查channel信号
        if (LTS_CHANNEL_SIGEXIT == lts_global_sm.channel_signal) {
            (void)lts_write_logger(
                &lts_file_logger, LTS_LOG_INFO,
                "slave ready to exit\n"
            );
            break;
        }

        // 检查父进程状态
        if (-1 == kill(lts_processes[lts_ps_slot].ppid, 0)) {
            (void)lts_write_logger(
                &lts_file_logger, LTS_LOG_WARN,
                "I am gona die because my parent has been dead\n"
            );
            break;
        }

        // 更新进程负载
        lts_accept_disabled = lts_sock_inuse_n / 8 - lts_sock_cache_n;

        if (lts_accept_disabled < 0) {
            if (!hold) {
                dlist_for_each_f(pos, &lts_listen_list) {
                    lts_socket_t *ls;

                    ls = CONTAINER_OF(pos, lts_socket_t, dlnode);
                    (*lts_event_itfc->event_add)(ls);
                }

                hold = TRUE;
            }
        } else {
            if (hold) {
                dlist_for_each_f(pos, &lts_listen_list) {
                    lts_socket_t *ls;

                    ls = CONTAINER_OF(pos, lts_socket_t, dlnode);
                    (*lts_event_itfc->event_del)(ls);
                }

                hold = FALSE;
            }
        }

        rslt = (*lts_event_itfc->process_events)();

        if (0 != rslt) {
            break;
        }

        process_post_list();
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
        // 检查channel信号
        if (LTS_CHANNEL_SIGEXIT == lts_global_sm.channel_signal) {
            (void)lts_write_logger(
                &lts_file_logger, LTS_LOG_INFO,
                "slave ready to exit\n"
            );
            break;
        }

        // 检查父进程状态
        if (-1 == kill(lts_processes[lts_ps_slot].ppid, 0)) {
            (void)lts_write_logger(
                &lts_file_logger, LTS_LOG_WARN,
                "I am gona die because my parent has been dead\n"
            );
            break;
        }

        // 更新进程负载
        lts_accept_disabled = lts_sock_inuse_n / 8 - lts_sock_cache_n;

        if (lts_accept_disabled < 0) {
            if (lts_shmtx_trylock((lts_atomic_t *)lts_accept_lock.addr)) {
                // 抢锁成功
                enable_accept_events();
            } else {
                 disable_accept_events();
            }
        } else {
            disable_accept_events();
        }

        rslt = (*lts_event_itfc->process_events)();
        if (lts_accept_lock_hold) {
            lts_shmtx_unlock((lts_atomic_t *)lts_accept_lock.addr);
        }
        if (0 != rslt) {
            break;
        }

        process_post_list();
    }

    if (0 != rslt) {
        return -1;
    }

    return 0;
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
                (void)lts_write_logger(&lts_file_logger, LTS_LOG_ERROR,
                                       "close channel failed\n");
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

    // 守护进程
    if (lts_conf.daemon && (-1 == daemon(FALSE, FALSE))) {
        return -1;
    }

    // 事件循环
    workers = 0; // 当前工作进程数
    if (lts_conf.workers > 1) {
        lts_use_accept_lock = TRUE;
    } else {
        lts_use_accept_lock = FALSE;
    }
    (void)lts_write_logger(&lts_file_logger, LTS_LOG_INFO, "master started\n");
    while (TRUE) {
        if (0 == (lts_signals_mask & LTS_MASK_SIGEXIT)) {
            // 重启工作进程
            while (workers  < lts_conf.workers) {
                pid_t p;
                int domain_fd;

                // 寻找空闲槽
                for (slot = 0; slot < lts_conf.workers; ++slot) {
                    if (-1 == lts_processes[slot].pid) {
                        break;
                    }
                }
                if (slot >= lts_conf.workers) { // bug defence
                    abort();
                }

                // 创建ipc域套接字
                lts_global_sm.unix_domain_enabled[slot] = TRUE;
                if (-1 == socketpair(AF_UNIX, SOCK_STREAM,
                                     0, lts_processes[slot].channel)) {
                    lts_global_sm.unix_domain_enabled[slot] = FALSE;
                    (void)lts_write_logger(&lts_file_logger,
                                           LTS_LOG_ERROR,
                                           "socketpair() failed: %s\n",
                                           lts_errno_desc[errno]);
                }
                domain_fd = -1;
                if (lts_global_sm.unix_domain_enabled[slot]) {
                    domain_fd = lts_processes[slot].channel[1];
                    if (-1 == lts_set_nonblock(domain_fd)) {
                        (void)lts_write_logger(
                            &lts_file_logger, LTS_LOG_WARN,
                            "set nonblock unix domain socket failed: %s\n",
                            lts_errno_desc[errno]
                        );
                    }
                }

                p = fork();

                if (-1 == p) {
                    (void)lts_write_logger(
                        &lts_file_logger, LTS_LOG_ERROR, "fork failed\n"
                    );
                    break;
                }

                if (0 == p) {
                    // 子进程
                    lts_processes[slot].ppid = lts_pid;
                    lts_pid = getpid();
                    lts_processes[slot].pid = getpid();
                    lts_process_role = LTS_SLAVE;
                    lts_ps_slot = slot; // 新进程槽号

                    // 初始化信号处理
                    if (-1 == lts_init_sigactions(lts_process_role)) {
                        (void)lts_write_logger(
                            &lts_stderr_logger, LTS_LOG_EMERGE,
                            "init sigactions failed\n"
                        );
                        _exit(EXIT_FAILURE);
                    }

                    _exit(worker_main());
                }

                lts_processes[slot].pid = p;
                ++workers;
            } // end while
        }

        sigemptyset(&tmp_mask);
        (void)sigsuspend(&tmp_mask);

        if (lts_signals_mask & LTS_MASK_SIGEXIT) {
            for (int i = 0; i < lts_conf.workers; ++i) {
                uint32_t sigexit = LTS_CHANNEL_SIGEXIT;

                if (-1 == lts_processes[i].pid) {
                    continue;
                }

                // 通知子进程退出
                if (-1 == send(lts_processes[i].channel[0],
                               &sigexit, sizeof(uint32_t), 0)) {
                    (void)lts_write_logger(&lts_file_logger, LTS_LOG_ERROR,
                                           "send() failed: %s\n",
                                           lts_errno_desc[errno]);
                }
            }
            (void)raise(SIGCHLD);
        }

        if (lts_signals_mask & LTS_MASK_SIGCHLD) {
            // 等待子进程退出
            while ((child = wait_children()) > 0) {
                --workers;
            }
            if (-1 == child) {
                assert(ECHILD == errno);
                (void)lts_write_logger(&lts_file_logger, LTS_LOG_INFO,
                                       "master ready to exit\n");
                break;
            }
        }
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
    int rslt;

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

    // 事件循环
    (void)lts_write_logger(&lts_file_logger, LTS_LOG_INFO, "slave started\n");
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
    int last, rslt;
    lts_module_t *module;

    do {
        /*
        lts_rb_root_t rt = RB_ROOT;
        lts_socket_t s[1000];
        int count = ARRAY_COUNT(s);

        for (int i = 0; i < count; ++i) {
            lts_init_socket(&s[i]);
            s[i].timeout = i + 10;
        }
        for (int i = 0; i < count; ++i) {
            lts_timer_heap_add(&rt, &s[i]);
        }
        for (int i = count - 1; i >= 0; --i) {
            lts_timer_heap_del(&rt, &s[i]);
        }

        return 0;
        */
    } while(0);

    // 全局初始化
    lts_pid = getpid(); // 初始化进程号
    lts_process_role = LTS_MASTER; // 进程角色
    lts_init_log_prefixes();
    lts_update_time();
    lts_sys_pagesize = (size_t)sysconf(_SC_PAGESIZE);

    // 初始化信号处理
    if (-1 == lts_init_sigactions(lts_process_role)) {
        (void)lts_write_logger(&lts_stderr_logger,
                               LTS_LOG_EMERGE, "init sigactions failed\n");
        return EXIT_FAILURE;
    }

    // 核心模块 {{
    last = 0;
    rslt = 0;
    while (lts_modules[last]) {
        module = lts_modules[last++];

        if (LTS_CORE_MODULE != module->type) {
            continue;
        }

        if (NULL == module->init_master) {
            continue;
        }

        if (0 != (*module->init_master)(module)) {
            rslt = -1;
            break;
        }
    }

    if (0 == rslt) {
        lts_module_count = last;
        rslt = master_main();
    }

    while (--last > 0) {
        module = lts_modules[last];

        if (LTS_CORE_MODULE != module->type) {
            continue;
        }

        if (NULL == module->exit_master) {
            continue;
        }

        (*module->exit_master)(module);
    }
    // }} 核心模块

    return (0 == rslt) ? EXIT_FAILURE : EXIT_SUCCESS;
}

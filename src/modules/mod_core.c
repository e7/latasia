#include "conf.h"
#include "vsignal.h"
#include "logger.h"
#include "latasia.h"

#define __THIS_FILE__       "src/modules/mod_core.c"

#define CONF_FILE           "conf/latasia.conf"


static int daemonize(const char *wd);
static int load_main_config(lts_conf_t *conf, lts_pool_t *pool);
static int init_core_master(lts_module_t *module);
static void exit_core_master(lts_module_t *module);


lts_module_t lts_core_module = {
    lts_string("lts_core_module"),
    LTS_CORE_MODULE,
    NULL,
    NULL,
    NULL,
    // interfaces
    &init_core_master,
    NULL,
    NULL,
    &exit_core_master,
};


int load_main_config(lts_conf_t *conf, lts_pool_t *pool)
{
    extern lts_conf_item_t *lts_conf_main_items[];

    off_t sz;
    uint8_t *addr;
    int rslt;
    lts_file_t lts_conf_file = {
        -1, {
            (uint8_t *)CONF_FILE, sizeof(CONF_FILE) - 1,
        },
    };

    if (-1 == load_conf_file(&lts_conf_file, &addr, &sz)) {
        return -1;
    }
    rslt = parse_conf(addr, sz, lts_conf_main_items, pool, conf);
    close_conf_file(&lts_conf_file, addr, sz);

    return rslt;
}


int daemonize(const char *wd)
{
    switch (fork()) {
    case -1:
        return -1;
    case 0:
        break;
    default:
        _exit(0);
    }

    if (-1 == setsid()) {
        _exit(0);
    }

    if (wd) {
        if (-1 == chdir(wd)) {
            _exit(0);
        }
    }

    if (-1 == close(STDIN_FILENO)) {
        _exit(0);
    }
    if (-1 == close(STDOUT_FILENO)) {
        _exit(0);
    }
    if (-1 == close(STDERR_FILENO)) {
        _exit(0);
    }

    int fd = open("/dev/null", O_RDWR, 0);
    if (-1 == fd) {
        _exit(0);
    }

    if(-1 == dup2(fd, STDIN_FILENO)) {
        _exit(0);
    }
    if(-1 == dup2(fd, STDOUT_FILENO)) {
        _exit(0);
    }
    if(-1 == dup2(fd, STDERR_FILENO)) {
        _exit(0);
    }

    return 0;
}


int init_core_master(lts_module_t *module)
{
    char const *wd = "./";
    lts_pool_t *pool;
    lts_str_t strpid = {NULL, 32};

    // 创建内存池
    pool = lts_create_pool(MODULE_POOL_SIZE);
    if (NULL == pool) {
        // log
        return -1;
    }
    module->pool = pool;

    // 读取配置
    if (-1 == load_main_config(&lts_main_conf, module->pool)) {
        (void)lts_write_logger(&lts_stderr_logger, LTS_LOG_WARN,
                               "%s:load config failed, using default\n",
                               STR_LOCATION);
    }

    // 初始化日志
    lts_str_init(&lts_log_file.name,
                 lts_main_conf.log_file.data, lts_main_conf.log_file.len);
    if (-1 == lts_file_open(&lts_log_file, O_RDWR | O_CREAT | O_APPEND,
                            S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH,
                            &lts_stderr_logger)) {
        (void)lts_write_logger(&lts_stderr_logger, LTS_LOG_EMERGE,
                               "%s:open log file failed\n", STR_LOCATION);

        return -1;
    }

    // 守护进程
    if (lts_main_conf.daemon && (-1 == daemonize(wd))) {
        (void)lts_write_logger(
            &lts_file_logger, LTS_LOG_INFO,
            "%s:fall into daemon failed\n", STR_LOCATION
        );

        return -1;
    }

    // 创建pid文件
    lts_pid = getpid(); // 初始化进程号
    lts_str_init(&lts_pid_file.name,
                 lts_main_conf.pid_file.data, lts_main_conf.pid_file.len);
    if (-1 == lts_file_open(&lts_pid_file, O_RDWR | O_CREAT | O_EXCL,
                            S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH,
                            &lts_stderr_logger)) {
        switch (errno) {
        case EEXIST:
            {
                (void)lts_write_logger(
                    &lts_file_logger, LTS_LOG_EMERGE,
                    "%s:pid file %s%s exists\n", STR_LOCATION,
                    wd, lts_pid_file.name.data
                );
                break;
            }
        case ENOENT:
            {
                (void)lts_write_logger(
                    &lts_file_logger, LTS_LOG_EMERGE,
                    "%s:plese ensure the directories where "
                       "the pid file %s%s located exists\n",
                    STR_LOCATION, wd, lts_pid_file.name.data
                );
                break;
            }
        default:
            {
                (void)lts_write_logger(
                    &lts_file_logger, LTS_LOG_EMERGE,
                    "%s:open pid file %s%s failed:%d\n",
                    STR_LOCATION, wd, lts_pid_file.name.data, errno
                );
                break;
            }
        }

        return -1;
    }
    strpid.data = (uint8_t *)lts_palloc(pool, strpid.len);
    lts_l2str(&strpid, lts_pid);
    (void)lts_file_write(&lts_pid_file,
                         strpid.data, strpid.len, &lts_stderr_logger);

    // 进程组信息初始化
    for (int i = 0; i < ARRAY_COUNT(lts_processes); ++i) {
        lts_processes[i].ppid = -1;
        lts_processes[i].pid = -1;
    }

    // 初始化时间堆
    lts_timer_heap = (lts_timer_t){0, RB_ROOT};

    return 0;
}


void exit_core_master(lts_module_t *module)
{
    // 删除pid文件
    if (-1 == unlink((char const *)lts_main_conf.pid_file.data)) {
        (void)lts_write_logger(&lts_file_logger, LTS_LOG_ERROR,
                               "%s:delete pid file failed:%d\n",
                               STR_LOCATION, errno);
    }

    // 关闭日志
    lts_file_close(&lts_log_file);

    // 释放内存池
    if (module->pool) {
        lts_destroy_pool(module->pool);
        module->pool = NULL;
    }

    return;
}

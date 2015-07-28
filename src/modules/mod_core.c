#include "conf.h"
#include "vsignal.h"
#include "logger.h"
#include "latasia.h"

#define __THIS_FILE__       "src/modules/mod_core.c"


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


int init_core_master(lts_module_t *module)
{
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
    if (-1 == lts_load_config(&lts_conf, module->pool)) {
        (void)lts_write_logger(&lts_stderr_logger, LTS_LOG_WARN,
                               "%s:load config failed, using default\n",
                               STR_LOCATION);
    }

    // 创建pid文件
    lts_str_init(&lts_pid_file.name,
                 lts_conf.pid_file.data, lts_conf.pid_file.len);
    if (-1 == lts_file_open(&lts_pid_file, O_RDWR | O_CREAT | O_EXCL,
                            S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH,
                            &lts_stderr_logger)) {
        switch (errno) {
        case EEXIST:
            {
                (void)lts_write_logger(&lts_stderr_logger, LTS_LOG_EMERGE,
                                       "%s:pid file exists\n", STR_LOCATION);
                break;
            }
        case ENOENT:
            {
                (void)lts_write_logger(
                    &lts_stderr_logger, LTS_LOG_EMERGE,
                    "%s:plese ensure the directories where "
                       "the pid file located exists\n",
                    STR_LOCATION
                );
                break;
            }
        default:
            {
                break;
            }
        }

        return -1;
    }
    strpid.data = (uint8_t *)lts_palloc(pool, strpid.len);
    lts_l2str(&strpid, lts_pid);
    (void)lts_file_write(&lts_pid_file,
                         strpid.data, strpid.len, &lts_stderr_logger);

    // 初始化日志
    lts_str_init(&lts_log_file.name,
                 lts_conf.log_file.data, lts_conf.log_file.len);
    if (-1 == lts_file_open(&lts_log_file, O_RDWR | O_CREAT | O_APPEND,
                            S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH,
                            &lts_stderr_logger)) {
        (void)lts_write_logger(&lts_stderr_logger, LTS_LOG_EMERGE,
                               "%s:open log file failed\n", STR_LOCATION);

        // 删除pid文件
        if (-1 == unlink((char const *)lts_conf.pid_file.data)) {
            (void)lts_write_logger(&lts_stderr_logger, LTS_LOG_ERROR,
                                   "%s:delete pid file failed\n", STR_LOCATION);
        }

        return -1;
    }

    // 进程组信息初始化
    for (int i = 0; i < ARRAY_COUNT(lts_processes); ++i) {
        lts_processes[i].ppid = -1;
        lts_processes[i].pid = -1;
    }

    return 0;
}

void exit_core_master(lts_module_t *module)
{
    // 删除pid文件
    if (-1 == unlink((char const *)lts_conf.pid_file.data)) {
        (void)lts_write_logger(&lts_file_logger, LTS_LOG_ERROR,
                               "%s:delete pid file failed\n", STR_LOCATION);
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

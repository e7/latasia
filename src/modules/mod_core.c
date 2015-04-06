#include "conf.h"
#include "vsignal.h"
#include "logger.h"
#include "latasia.h"


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

    // 创建内存池
    pool = lts_create_pool(MODULE_POOL_SIZE);
    if (NULL == pool) {
        // log
        return -1;
    }
    module->pool = pool;

    // 读取配置
    if (-1 == lts_load_config(&lts_conf, module->pool)) {
        (void)lts_write_logger(&lts_stderr_logger,
                               LTS_LOG_EMERGE, "load configure failed\n");
        return -1;
    }

    // 初始化日志
    lts_str_init(&lts_log_file.name,
                 lts_conf.log_file.data, lts_conf.log_file.len);
    if (-1 == lts_file_open(&lts_log_file, O_RDWR | O_CREAT | O_APPEND,
                            S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH,
                            &lts_stderr_logger))
    {
        (void)lts_write_logger(&lts_stderr_logger,
                               LTS_LOG_EMERGE, "open log file failed\n");
        return -1;
    }

    // 初始化信号处理
    if (-1 == lts_init_sigactions()) {
        (void)lts_write_logger(&lts_stderr_logger,
                               LTS_LOG_EMERGE, "init sigactions failed\n");
        return -1;
    }

    return 0;
}

void exit_core_master(lts_module_t *module)
{
    // 关闭日志
    lts_file_close(&lts_log_file);

    // 释放内存池
    if (module->pool) {
        lts_destroy_pool(module->pool);
        module->pool = NULL;
    }

    return;
}

#include <zlib.h>
#include <arpa/inet.h>
#include <pthread.h>

#include <strings.h>

#include "latasia.h"
#include "rbtree.h"
#include "logger.h"
#include "conf.h"

#define __THIS_FILE__       "src/modules/mod_app_asyn_backend.c"

#define N_NETIO_THREAD      1


typedef struct {
    pthread_t *netio_threads;
} asyn_backend_ctx_t;


static uintptr_t s_running;
static asyn_backend_ctx_t s_ctx;


static void *netio_thread_proc(void *args)
{
    while (s_running) {
        sleep(1);
    }

    return NULL;
}


static int init_asyn_backend_module(lts_module_t *module)
{
    int rslt;
    lts_pool_t *pool;

    // 创建内存池
    pool = lts_create_pool(MODULE_POOL_SIZE);
    if (NULL == pool) {
        // log
        return -1;
    }
    module->pool = pool;

    // 上下文初始化
    s_running = TRUE;
    s_ctx.netio_threads = (pthread_t *)lts_palloc(
        pool, N_NETIO_THREAD * sizeof(pthread_t)
    );

    // 创建后端网络线程
    rslt = 0;
    for (int i = 0; i < N_NETIO_THREAD; ++i) {
        rslt = pthread_create(
            &s_ctx.netio_threads[i], NULL, &netio_thread_proc, NULL
        );

        if (rslt) {
            s_running = FALSE; // 通知已有线程退出
            break;
        }
    }

    return rslt;
}


static void exit_asyn_backend_module(lts_module_t *module)
{
    s_running = FALSE; // 通知已有线程退出
    return;
}


static void asyn_backend_service(lts_socket_t *s)
{
    return;
}


static void asyn_backend_send_more(lts_socket_t *s)
{
    return;
}


static lts_app_module_itfc_t asyn_backend_itfc = {
    &asyn_backend_service,
    &asyn_backend_send_more,
};

lts_module_t lts_app_asyn_backend_module = {
    lts_string("lts_app_asyn_backend_module"),
    LTS_APP_MODULE,
    &asyn_backend_itfc,
    NULL,
    &s_ctx,

    // interfaces
    NULL,
    &init_asyn_backend_module,
    &exit_asyn_backend_module,
    NULL,
};

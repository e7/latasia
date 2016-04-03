#include <zlib.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <pthread.h>

#include <strings.h>

#include "latasia.h"
#include "rbtree.h"
#include "logger.h"
#include "conf.h"

#define __THIS_FILE__       "src/modules/mod_app_asyn_backend.c"


typedef struct {
    int channel;
} thread_args_t;

typedef struct {
    int pipeline[2];
    lts_socket_t *cs;
    pthread_t netio_thread;
} asyn_backend_ctx_t;


static uintptr_t s_running;
static asyn_backend_ctx_t s_ctx;


static void *netio_thread_proc(void *args)
{
    lts_pool_t *pool;
    thread_args_t local_args = *(thread_args_t *)args;

    // 创建内存池
    pool = lts_create_pool(MODULE_POOL_SIZE);
    if (NULL == pool) {
        // log
        return NULL;
    }

    // 工作循环
    while (s_running) {
        sleep(1);
    }

    // 销毁内存池
    lts_destroy_pool(pool);

    return NULL;
}


static void rs_pipe_recv(lts_socket_t *cs)
{
}


static void rs_pipe_send(lts_socket_t *cs)
{
}


static int init_asyn_backend_module(lts_module_t *module)
{
    lts_pool_t *pool;

    // 创建模块内存池
    pool = lts_create_pool(MODULE_POOL_SIZE);
    if (NULL == pool) {
        // log
        return -1;
    }
    module->pool = pool;

    // 创建通信管线
    if (-1 == socketpair(AF_UNIX, SOCK_STREAM, 0, s_ctx.pipeline)) {
        // log
        return -1;
    }
    if (lts_set_nonblock(s_ctx.pipeline[0])
        || lts_set_nonblock(s_ctx.pipeline[1])) {
        // log
        return -1;
    }
    s_ctx.cs = lts_alloc_socket();
    s_ctx.cs->fd = s_ctx.pipeline[0];
    s_ctx.cs->ev_mask = (EPOLLET | EPOLLIN);
    s_ctx.cs->conn = NULL;
    s_ctx.cs->do_read = &rs_pipe_recv;
    s_ctx.cs->do_write = &rs_pipe_send;
    s_ctx.cs->do_timeout = NULL;
    s_ctx.cs->timeout = 0;
    (*lts_event_itfc->event_add)(s_ctx.cs);

    // 创建后端网络线程
    s_running = TRUE;
    thread_args_t *args = (thread_args_t *)lts_palloc(
        pool, sizeof(thread_args_t)
    );
    args->channel = s_ctx.pipeline[1];
    if (pthread_create(&s_ctx.netio_thread, NULL, &netio_thread_proc, args)) {
        // log
        return -1;
    }

    return 0;
}


static void exit_asyn_backend_module(lts_module_t *module)
{
    // 等待后端线程退出
    s_running = FALSE; // 通知线程退出
    (void)pthread_join(s_ctx.netio_thread, NULL);

    // 关闭pipeline
    if (close(s_ctx.pipeline[0]) || close(s_ctx.pipeline[1])) {
        // log
    }
    lts_free_socket(s_ctx.cs);
    s_ctx.cs = NULL;

    // 销毁模块内存池
    if (module->pool) {
        lts_destroy_pool(module->pool);
        module->pool = NULL;
    }

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

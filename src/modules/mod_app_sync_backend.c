#include <zlib.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <pthread.h>

#include <strings.h>

#include "latasia.h"
#include "rbtree.h"
#include "logger.h"
#include "conf.h"

#define __THIS_FILE__       "src/modules/mod_app_sync_backend.c"

#define CONF_FILE           "conf/mod_app.conf"


typedef struct {
    int channel;
} thread_args_t;

typedef struct {
    int pipeline[2];
    lts_socket_t *cs; // channel connection
    pthread_t netio_thread;
    lts_buffer_t *push_buf; // 推送缓冲
} sync_backend_ctx_t;


static uintptr_t s_running;
static sync_backend_ctx_t s_ctx;

static int load_sync_config(lts_conf_sync_t *conf, lts_pool_t *pool);
static void *netio_thread_proc(void *args);


// 加载模块配置
int load_sync_config(lts_conf_sync_t *conf, lts_pool_t *pool)
{
    extern lts_conf_item_t *lts_conf_sync_items[];

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
    rslt = parse_conf(addr, sz, lts_conf_sync_items, pool, conf);
    close_conf_file(&lts_conf_file, addr, sz);

    return rslt;
}


void *netio_thread_proc(void *args)
{
#define RECV_BUF_SZ             4096

    uint8_t *recv_buf;
    lts_pool_t *pool;
    // thread_args_t local_args = *(thread_args_t *)args;

    // 创建内存池
    pool = lts_create_pool(MODULE_POOL_SIZE);
    if (NULL == pool) {
        // log
        return NULL;
    }

    // 工作循环
    recv_buf = (uint8_t *)lts_palloc(pool, RECV_BUF_SZ);
    while (s_running) {
        // connect
        // recv
        // send
        sleep(1);
    }

    // 销毁内存池
    lts_destroy_pool(pool);

    return NULL;
}


static void rs_channel_recv(lts_socket_t *cs)
{
}


static int init_sync_backend_module(lts_module_t *module)
{
    lts_pool_t *pool;

    // 创建模块内存池
    pool = lts_create_pool(MODULE_POOL_SIZE);
    if (NULL == pool) {
        // log
        return -1;
    }
    module->pool = pool;

    // 读取模块配置
    if (-1 == load_sync_config(&lts_sync_conf, module->pool)) {
        (void)lts_write_logger(&lts_stderr_logger, LTS_LOG_WARN,
                               "%s:load sync config failed, using default\n",
                               STR_LOCATION);
    }

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
    s_ctx.cs->do_read = &rs_channel_recv;
    s_ctx.cs->do_write = NULL;
    s_ctx.cs->do_timeout = NULL;
    s_ctx.cs->timeout = 0;
    (*lts_event_itfc->event_add)(s_ctx.cs);

    s_ctx.push_buf = lts_create_buffer(pool, 1024 * 16, 1024 * 16);

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


static void exit_sync_backend_module(lts_module_t *module)
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


static void sync_backend_service(lts_socket_t *s)
{
    ssize_t sent_sz;
    ssize_t sending_sz; // 推送的数据长度
    lts_buffer_t *rb = s->conn->rbuf;

    fprintf(stderr, "%ld\n", s->born_time);

    // 计算本次推送的数据长度
    sending_sz = 0;
    sending_sz += sizeof(int64_t) + sizeof(lts_socket_t *);
    sending_sz += (uintptr_t)rb->last - (uintptr_t)rb->seek;

    if (0 == s_ctx.push_buf->limit) {
        // 要求使用设限缓冲
        abort();
    }

    if ((s_ctx.push_buf->end - s_ctx.push_buf->last) < sending_sz) {
        // 响应系统繁忙
        return;
    }

    (void)lts_buffer_append(
        s_ctx.push_buf, (uint8_t *)&s->born_time, sizeof(int64_t)
    );
    (void)lts_buffer_append(
        s_ctx.push_buf, (uint8_t *)&s, sizeof(lts_socket_t *)
    );
    (void)lts_buffer_append(
        s_ctx.push_buf, rb->seek, (uintptr_t)rb->last - (uintptr_t)rb->seek
    );
    lts_buffer_clear(rb); // 清空接收缓冲

    // 向后端推送数据
    sent_sz = send(
        s_ctx.cs->fd, s_ctx.push_buf->seek,
        (uintptr_t)s_ctx.push_buf->last - (uintptr_t)s_ctx.push_buf->seek, 0
    );
    if (-1 == sent_sz) {
        if ((LTS_E_AGAIN == errno) || (LTS_E_WOULDBLOCK == errno)) {
            // 响应系统繁忙
        } else {
            abort(); // should NOT be
        }

        return;
    }

    // 调整推送缓冲
    s_ctx.push_buf->seek += sent_sz;
    lts_buffer_drop_accessed(s_ctx.push_buf);

    return;
}


static void sync_backend_send_more(lts_socket_t *s)
{
    return;
}


static lts_app_module_itfc_t sync_backend_itfc = {
    &sync_backend_service,
    &sync_backend_send_more,
};

lts_module_t lts_app_sync_backend_module = {
    lts_string("lts_app_sync_backend_module"),
    LTS_APP_MODULE,
    &sync_backend_itfc,
    NULL,
    &s_ctx,

    // interfaces
    NULL,
    &init_sync_backend_module,
    &exit_sync_backend_module,
    NULL,
};

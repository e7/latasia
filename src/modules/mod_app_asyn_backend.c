#include <zlib.h>
#include <arpa/inet.h>
#include <dirent.h>

#include <strings.h>

#include "latasia.h"
#include "rbtree.h"
#include "logger.h"
#include "conf.h"

#define __THIS_FILE__       "src/modules/mod_app_asyn_backend.c"


static int init_asyn_backend_module(lts_module_t *module)
{
    return 0;
}


static void exit_asyn_backend_module(lts_module_t *module)
{
    return;
}


static int asyn_backend_ibuf(lts_socket_t *s)
{
    lts_buffer_t *rb;
    lts_pool_t *pool;

    if (NULL == s->conn) {
        return -1;
    }

    pool = s->conn->pool;
    if (NULL == pool) {
        return -1;
    }

    rb = s->conn->rbuf;
    if (lts_buffer_empty(rb)) {
        abort();
    }

    return 0;
}


static int asyn_backend_obuf(lts_socket_t *s)
{
    lts_buffer_t *rb;
    lts_buffer_t *sb;

    if (NULL == s->conn) {
        abort();
    }

    rb = s->conn->rbuf;
    sb = s->conn->sbuf;

    // 清空发送缓冲
    lts_buffer_clear(sb);

    // exchange
    s->conn->rbuf = sb;
    s->conn->sbuf = rb;

    return 0;
}


static lts_app_module_itfc_t asyn_backend_itfc = {
    &asyn_backend_ibuf,
    &asyn_backend_obuf,
    NULL,
};

lts_module_t lts_app_asyn_backend_module = {
    lts_string("lts_app_asyn_backend_module"),
    LTS_APP_MODULE,
    &asyn_backend_itfc,
    NULL,
    NULL,
    // interfaces
    NULL,
    &init_asyn_backend_module,
    &exit_asyn_backend_module,
    NULL,
};

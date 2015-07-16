#include <zlib.h>
#include <arpa/inet.h>
#include <dirent.h>

#include <strings.h>

#include "latasia.h"
#include "rbtree.h"
#include "logger.h"
#include "conf.h"

#define __THIS_FILE__       "src/modules/mod_app_echo.c"


static int init_echo_module(lts_module_t *module)
{
    return 0;
}


static void exit_echo_module(lts_module_t *module)
{
    return;
}


static int echo_ibuf(lts_socket_t *s)
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

static int echo_obuf(lts_socket_t *s)
{
    size_t n, n_read;
    lts_buffer_t *sb;

    if (NULL == s->conn) {
        abort();
    }
    sb = s->conn->sbuf;

    return 0;
}

static lts_app_module_itfc_t echo_itfc = {
    &echo_ibuf,
    &echo_obuf,
    NULL,
};

lts_module_t lts_app_echo_module = {
    lts_string("lts_app_echo_module"),
    LTS_APP_MODULE,
    &echo_itfc,
    NULL,
    NULL,
    // interfaces
    NULL,
    &init_echo_module,
    &exit_echo_module,
    NULL,
};

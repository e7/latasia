#include <zlib.h>
#include <arpa/inet.h>
#include <dirent.h>

#include <strings.h>

#include "latasia.h"
#include "rbtree.h"
#include "logger.h"
#include "conf.h"
#include "protocol_sjsonb.h"

#define __THIS_FILE__       "src/modules/mod_app_sjsonb.c"


static int init_sjsonb_module(lts_module_t *module)
{
    return 0;
}


static void exit_sjsonb_module(lts_module_t *module)
{
    return;
}


static void sjsonb_on_connected(lts_socket_t *s)
{
    fprintf(stderr, "new connection....\n");
    return;
}


static void sjsonb_service(lts_socket_t *s)
{
    lts_sjson_t *sjson;
    lts_buffer_t *rb = s->conn->rbuf;
    lts_buffer_t *sb = s->conn->sbuf;
    lts_pool_t *pool;

    // 用于json处理的内存无法复用，每个请求使用新的内存池处理
    // 将来可以使用池中池解决
    pool = lts_create_pool(4096);

    sjson = lts_proto_sjsonb_decode(rb, pool);
    if (NULL == sjson) {
        lts_destroy_pool(pool);
        return;
    }

    lts_str_t key = lts_string("lala");
    lts_str_t val = lts_string("tiannalu");
    lts_sjson_add_kv(sjson, &key, &val);
    fprintf(stderr, "get json\n");
    fprintf(stderr, "%d\n", lts_proto_sjsonb_encode(sjson, sb));

    lts_destroy_pool(pool);
    return;
}


static void sjsonb_send_more(lts_socket_t *s)
{
    return;
}


static lts_app_module_itfc_t sjsonb_itfc = {
    &sjsonb_on_connected,
    &sjsonb_service,
    &sjsonb_send_more,
};

lts_module_t lts_app_sjsonb_module = {
    lts_string("lts_app_sjsonb_module"),
    LTS_APP_MODULE,
    &sjsonb_itfc,
    NULL,
    NULL,
    // interfaces
    NULL,
    &init_sjsonb_module,
    &exit_sjsonb_module,
    NULL,
};

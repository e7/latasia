#include <zlib.h>
#include <arpa/inet.h>
#include <dirent.h>
#include "latasia.h"
#include "rbtree.h"
#include "logger.h"
#include "conf.h"


typedef struct s_filename_list filename_list_t;

struct s_filename_list {
    lts_str_t name;
    filename_list_t *next;
};
static DIR *cwd;


static int init_http_core_module(lts_module_t *module)
{
    struct dirent entry, *rslt;

    module->pool = lts_create_pool(MODULE_POOL_SIZE);
    if (NULL == module->pool) {
        return -1;
    }

    cwd = opendir((char const *)lts_cwd.data);
    if (NULL == cwd) {
        return -1;
    }
    while (TRUE) {
        if (readdir_r(cwd, &entry, &rslt)) {
            // perror
            break;
        }

        if (NULL == rslt) {
            break;
        }

        if ('.' == entry.d_name[0]) {
            continue;
        }
    }

    return 0;
}


static void exit_http_core_module(lts_module_t *module)
{
    if (module->pool) {
        lts_destroy_pool(module->pool);
        module->pool = NULL;
    }

    if (cwd) {
        (void)closedir(cwd);
    }

    return;
}


static int http_core_iobuf(lts_socket_t *s)
{
    lts_buffer_t *rb;
    lts_buffer_t *sb;
    uint8_t rsp_buf[] = {
        "HTTP/1.1 200 OK\r\n"
        "Server: hixo\r\n"
        "Content-Length: 169\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n\r\n"
        "<!DOCTYPE html>\r\n"
        "<html>\r\n"
        "<head>\r\n"
        "<title>welcome to latasia</title>\r\n"
        "</head>\r\n"
        "<body bgcolor=\"white\" text=\"black\">\r\n"
        "<center><h1>welcome to latasia!</h1></center>\r\n"
        "</body>\r\n"
        "</html>\r\n"
    };

    rb = s->conn->rbuf;
    sb = s->conn->sbuf;
    if (rb->last == rb->start) {
        return 0;
    }

    assert((uintptr_t)(sb->end - sb->last) > sizeof(rsp_buf));
    (void)memcpy(sb->start, rsp_buf, sizeof(rsp_buf));
    rb->last = rb->start;
    sb->last += sizeof(rsp_buf);

    return 0;
}


static lts_app_module_itfc_t http_core_itfc = {
    http_core_iobuf,
};

lts_module_t lts_app_http_core_module = {
    lts_string("lts_app_http_core_module"),
    LTS_APP_MODULE,
    &http_core_itfc,
    NULL,
    NULL,
    // interfaces
    NULL,
    &init_http_core_module,
    &exit_http_core_module,
    NULL,
};

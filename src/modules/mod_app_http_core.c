#include <zlib.h>
#include <arpa/inet.h>
#include <dirent.h>
#include "latasia.h"
#include "rbtree.h"
#include "logger.h"
#include "conf.h"


#define DEFAULT_FILENAME        "index.html"
#define HTTP_404_HEADER         "HTTP/1.1 404 NOT FOUND\r\n"\
                                "Server: latasia\r\n"\
                                "Content-Length: 133\r\n"\
                                "Content-Type: text/html\r\n"\
                                "Connection: close\r\n\r\n"
#define HTTP_404_BODY           "<!DOCTYPE html>"\
                                "<html>"\
                                "<head>"\
                                "<meta charset=\"utf-8\" />"\
                                "<title>404</title>"\
                                "</head>"\
                                "<body>"\
                                "{\"result\": 404, \"message\": not found}"\
                                "</body>"\
                                "</html>"
#define HTTP_200_HEADER         "HTTP/1.1 200 OK\r\n"\
                                "Server: latasia\r\n"\
                                "Content-Length: %lu\r\n"\
                                "Content-Type: application/octet-stream\r\n"\
                                "Connection: close\r\n\r\n"


typedef struct {
    lts_str_t req_path;
    lts_file_t req_file;
} http_core_ctx_t;


void dump_mem_to_file(char const *filepath, uint8_t *data, size_t n)
{
    FILE *f = fopen(filepath, "wb");
    fwrite(data, n, 1, f);
    fclose(f);
}


static int init_http_core_module(lts_module_t *module)
{
    lts_pool_t *pool;

    pool = lts_create_pool(MODULE_POOL_SIZE);
    if (NULL == pool) {
        return -1;
    }
    module->pool = pool;

    return 0;
}


static void exit_http_core_module(lts_module_t *module)
{
    if (module->pool) {
        lts_destroy_pool(module->pool);
        module->pool = NULL;
    }

    return;
}


static void http_core_ibuf(lts_socket_t *s)
{
    lts_buffer_t *rb;
    lts_str_t idata, req_line;
    int pattern_s;
    lts_str_t pattern;
    lts_str_t uri;
    lts_pool_t *pool;
    http_core_ctx_t *ctx;

    if (NULL == s->conn) {
        return;
    }

    pool = s->conn->pool;
    if (NULL == pool) {
        return;
    }
    rb = s->conn->rbuf;
    if (lts_buffer_empty(rb)) {
        return;
    }

    /*
    if (lts_buffer_full(rb)) {
        *(rb->last - 1) = 0;
    } else {
        *(rb->last - 0) = 0;
    }
    fprintf(stderr, "%s\n", rb->start);
    */

    // 获取请求第一行
    idata.data = rb->start;
    idata.len = (size_t)(rb->last - rb->start);
    pattern = (lts_str_t)lts_string("\r\n");
    pattern_s = lts_str_find(&idata, &pattern, 0);
    if (-1 == pattern_s) {
        // log
        return;
    }
    req_line.data = idata.data;
    req_line.len = pattern_s;

    // 获取请求路径
    uri.data = NULL;
    uri.len = 0;
    for (size_t i = 0; i < req_line.len; ++i) {
        if (uri.data) {
            ++uri.len;
            if (' ' == req_line.data[i]) {
                break;
            }
        } else {
            if ('/' == req_line.data[i]) {
                uri.data = &req_line.data[i];
            }
        }
    }
    if (0 == uri.len) {
        return;
    }

    // 生成绝对路径
    ctx = (http_core_ctx_t *)lts_palloc(pool, sizeof(http_core_ctx_t));
    ctx->req_path.len = lts_cwd.len + uri.len;
    if ('/' == uri.data[uri.len - 1]) {
        ctx->req_path.len += sizeof(DEFAULT_FILENAME) - 1;
    }
    ctx->req_path.data = lts_palloc(pool, ctx->req_path.len + 1);
    (void)memcpy(ctx->req_path.data, lts_cwd.data, lts_cwd.len);
    (void)memcpy(ctx->req_path.data + lts_cwd.len, uri.data, uri.len);
    if ('/' == uri.data[uri.len - 1]) {
        (void)memcpy(ctx->req_path.data + lts_cwd.len + uri.len,
                     DEFAULT_FILENAME, sizeof(DEFAULT_FILENAME) - 1);
    }
    ctx->req_path.data[ctx->req_path.len] = 0;
    ctx->req_file.fd = -1;

    // 清空接收缓冲
    lts_buffer_clear(rb);

    s->app_ctx = ctx;

    return;
}

static void http_core_obuf(lts_socket_t *s)
{
    size_t n, n_read;
    lts_buffer_t *sb;
    http_core_ctx_t *ctx;

    if (NULL == s->conn) {
        return;
    }
    sb = s->conn->sbuf;
    if (! lts_buffer_empty(sb)) {
        return; // 等清空再发
    }

    ctx = s->app_ctx;
    if ((NULL == ctx) || (0 == ctx->req_path.len)) {
        return;
    }

    n = sb->end - sb->last;
    ctx->req_file.name = ctx->req_path;
    if (-1 == ctx->req_file.fd) {
        struct stat st;

        // 打开文件
        if (-1 == lts_file_open(&ctx->req_file, O_RDONLY, S_IWUSR | S_IRUSR,
                                &lts_file_logger)) {
            // 404
            n = sizeof(HTTP_404_HEADER) - 1;
            (void)memcpy(sb->last, HTTP_404_HEADER, n);
            sb->last += n;
            n = sizeof(HTTP_404_BODY) - 1;
            (void)memcpy(sb->last, HTTP_404_BODY, n);
            sb->last += n;
            return;
        }

        (void)fstat(ctx->req_file.fd, &st);

        // 发送http头
        if (sizeof(HTTP_200_HEADER) - 1 > (n + 32)) { // 预留长度字符串
            abort();
        }

        n_read = snprintf((char *)sb->last, n, HTTP_200_HEADER, st.st_size);
        if (n_read > 0) {
            sb->last += n_read;
        }

        return;
    }

    // 读文件数据
    n_read = lts_file_read(&ctx->req_file, sb->last, n, &lts_file_logger);
    if (n_read > 0) {
        sb->last += n_read;
        return;
    }

    // 读取完毕
    s->closing = 1;
    lts_file_close(&ctx->req_file);
    ctx->req_path.len = 0;
    ctx->req_file.fd = -1;

    return;
}


static lts_app_module_itfc_t http_core_itfc = {
    http_core_ibuf,
    http_core_obuf,
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

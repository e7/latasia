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


static lts_file_t req_file = {-1};


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
    lts_str_t req_path;

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

    // 生成绝对路径
    req_path.len = lts_cwd.len + uri.len;
    if ('/' == uri.data[uri.len - 1]) {
        req_path.len += sizeof(DEFAULT_FILENAME) - 1;
    }
    req_path.data = lts_palloc(pool, req_path.len + 1);
    (void)memcpy(req_path.data, lts_cwd.data, lts_cwd.len);
    (void)memcpy(req_path.data + lts_cwd.len, uri.data, uri.len);
    if ('/' == uri.data[uri.len - 1]) {
        (void)memcpy(req_path.data + lts_cwd.len + uri.len,
                     DEFAULT_FILENAME, sizeof(DEFAULT_FILENAME) - 1);
    }
    req_path.data[req_path.len] = 0;

    // 清空接收缓冲
    lts_buffer_clear(rb);

    // 打开文件
    req_file.name = req_path;
    if (-1 == lts_file_open(&req_file, O_RDONLY, S_IWUSR | S_IRUSR,
                            &lts_file_logger)) {
        fprintf(stderr, "open file failed: %s\n", req_path.data);
        return;
    }

    return;
}

static void http_core_obuf(lts_socket_t *s)
{
    lts_buffer_t *sb;
    size_t n;

    if (NULL == s->conn) {
        return;
    }
    sb = s->conn->sbuf;

    if (! lts_buffer_empty(sb)) {
        return; // 等清空再发
    }

    if (-1 == req_file.fd) {
        // 404
        n = sizeof(HTTP_404_HEADER) - 1;
        (void)memcpy(sb->last, HTTP_404_HEADER, n);
        sb->last += n;
        n = sizeof(HTTP_404_BODY) - 1;
        (void)memcpy(sb->last, HTTP_404_BODY, n);
        sb->last += n;
        return;
    }

    // 读文件数据
    n = sb->end - sb->last;
    if (lts_file_read(&req_file, sb->last, n) > 0) {
        sb->last += n;
        return;
    }

    // 读取完毕
    assert(sb->start == sb->last);
    lts_file_close(&req_file);

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

#include <zlib.h>
#include <arpa/inet.h>
#include <dirent.h>
#include "latasia.h"
#include "rbtree.h"
#include "logger.h"
#include "conf.h"


#define DEFAULT_FILENAME            "index.html"
#define HTTP_404_HEADER             "HTTP/1.1 404 NOT FOUND\r\n"\
                                    "Server: latasia\r\n"\
                                    "Content-Length: 132\r\n"\
                                    "Content-Type: text/html\r\n"\
                                    "Connection: close\r\n\r\n"
#define HTTP_404_BODY               "<!DOCTYPE html>"\
                                    "<html>"\
                                    "<head>"\
                                    "<meta charset=\"utf-8\" />"\
                                    "<title>404</title>"\
                                    "</head>"\
                                    "<body>"\
                                    "{\"result\": 404, \"message\":not found}"\
                                    "</body>"\
                                    "</html>"


typedef struct s_filename_list filename_list_t;

struct s_filename_list {
    lts_str_t name;
    filename_list_t *next;
};


static filename_list_t *fn_list;
static lts_file_t req_file = {-1};
static lts_buffer_t *output;


void dump_mem_to_file(char const *filepath, uint8_t *data, size_t n)
{
    FILE *f = fopen(filepath, "wb");
    fwrite(data, n, 1, f);
    fclose(f);
}

static int generate_rsp(lts_pool_t *pool)
{
    // 生成响应
    lts_buffer_t *body;
    uint8_t rsp_buf_head_tplt[] = {
        "HTTP/1.1 200 OK\r\n"
        "Server: latasia\r\n"
        "Content-Length: %u\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n\r\n"
    };
    uint8_t *rsp_buf_head;
    size_t head_sz;
    uint8_t rsp_buf_body_s[] = {
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<title>latasia</title>"
        "</head>"
        "<body>"
    };
    uint8_t rsp_buf_body_e[] = {
        "</body>"
        "</html>"
    };

    // body
    body = lts_create_buffer(pool, 1024, TRUE);
    if (NULL == body) {
        return -1;
    }
    if (lts_buffer_append(body, rsp_buf_body_s, sizeof(rsp_buf_body_s) - 1)) {
        return -1;
    }
    do {
        uint8_t ahref_s[] = {
            "<a href=\"#\">"
        };
        uint8_t ahref_e[] = {
            "</a><br>"
        };
        filename_list_t *p = fn_list;

        while (p) {
            if (lts_buffer_append(body, ahref_s, sizeof(ahref_s) - 1)) {
                return -1;
            }
            if (lts_buffer_append(body, p->name.data, p->name.len)) {
                return -1;
            }
            if (lts_buffer_append(body, ahref_e, sizeof(ahref_e) - 1)) {
                return -1;
            }
            p = p->next;
        }
    } while (0);
    if (lts_buffer_append(body, rsp_buf_body_e, sizeof(rsp_buf_body_e) - 1)) {
        return -1;
    }

    // head
    head_sz = sizeof(rsp_buf_head_tplt) + 16;
    rsp_buf_head = lts_palloc(pool, head_sz);
    (void)snprintf((char *)rsp_buf_head, head_sz,
                   (char const *)rsp_buf_head_tplt,
                   body->last - body->start);

    //output
    output = lts_create_buffer(pool, 1024, TRUE);
    if (NULL == output) {
        return -1;
    }
    if (lts_buffer_append(output, rsp_buf_head,
                          strlen((char const *)rsp_buf_head))) {
        return -1;
    }
    if (lts_buffer_append(output, body->start, body->last - body->start)) {
        return -1;
    }

    return 0;
}

static int init_http_core_module(lts_module_t *module)
{
    DIR *cwd;
    lts_pool_t *pool;
    struct dirent entry, *rslt;

    pool = lts_create_pool(MODULE_POOL_SIZE);
    if (NULL == pool) {
        return -1;
    }
    module->pool = pool;

    cwd = opendir((char const *)lts_cwd.data);
    if (NULL == cwd) {
        return -1;
    }

    fn_list = NULL;
    while (TRUE) {
        size_t len;
        filename_list_t *node;

        if (readdir_r(cwd, &entry, &rslt)) {
            // perror
            break;
        }

        if (NULL == rslt) {
            break;
        }

        if (DT_REG != entry.d_type) {
            continue;
        }

        len = strlen(entry.d_name);
        node = lts_palloc(pool, sizeof(filename_list_t));
        node->name.data = lts_palloc(pool, len + 1);
        (void)strncpy((char *)node->name.data, entry.d_name, len);
        node->name.data[len] = '\0';
        node->name.len = len;

        // 挂到链表上
        node->next = fn_list;
        fn_list = node;
    }

    (void)closedir(cwd);

    /*
    do {
        filename_list_t *p = fn_list;

        while (p) {
            fprintf(stderr, "%s\n", p->name.data);
            p = p->next;
        }

        return -1;
    } while (0);
    */

    if (generate_rsp(pool)) {
        return -1;
    }

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

    n = sizeof(HTTP_404_HEADER) - 1;
    (void)memcpy(sb->last, HTTP_404_HEADER, n);
    sb->last += n;
    n = sizeof(HTTP_404_BODY) - 1;
    (void)memcpy(sb->last, HTTP_404_BODY, n);
    sb->last += n;
    if (-1 == req_file.fd) {
        // 404
        return;
    }

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

/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#include <sys/mman.h>

#include "extra_errno.h"
#include "conf.h"
#include "file.h"
#include "simple_json.h"
#include "logger.h"

#define __THIS_FILE__       "src/conf.c"


#define CONF_FILE           "latasia.conf"
#define MAX_KEEPALIVE       (24 * 60 * 60)


typedef struct {
    lts_str_t name;
    void (*match_handler)(lts_conf_t *,
                          lts_str_t *,
                          lts_str_t *,
                          lts_pool_t *);
} conf_item_t;


// 分割字符串
static lts_str_t *split_str(lts_str_t *text, char delemiter, lts_pool_t *pool)
{
    int dlm_count, nitems;
    lts_str_t *rslt;

    // counting delemiter
    dlm_count = 0;
    for (int i = 0; i < text->len; ++i) {
        if (delemiter == text->data[i]) {
            ++dlm_count;
        }
    }

    // alloc memory
    nitems = dlm_count + 1 + 1;
    rslt = (lts_str_t *)lts_palloc(
        pool, sizeof(lts_str_t) * (nitems)
    );
    rslt[0].data = text->data;
    rslt[0].len = text->len;
    for (int i = 1; i < nitems; ++i) {
        rslt[i] = (lts_str_t)lts_null_string;
    }

    // split
    nitems = 0;
    for (int i = 0; i < text->len; ++i) {
        for (int j = i; j < text->len; ++j) {
            if (delemiter == text->data[j]) {
                rslt[nitems++].len = j - i;
                rslt[nitems].data = text->data + j + 1;
                rslt[nitems].len = text->len - j - 1;
                i = j;
                break;
            }
        }
    }

    return rslt;
}


// match of daemon
static void
cb_daemon_match(lts_conf_t *conf, lts_str_t *k, lts_str_t *v, lts_pool_t *pool)
{
    uint8_t *item_buf;
    size_t item_buf_size = MAX(v->len, 8) + 1;

    // 缓冲value
    item_buf  = (uint8_t *)lts_palloc(pool, item_buf_size);
    (void)memcpy(item_buf, v->data, v->len);
    item_buf[v->len] = 0;

    conf->daemon = atoi((char const *)item_buf);

    return;
}


// match of port
static void
cb_port_match(lts_conf_t *conf, lts_str_t *k, lts_str_t *v, lts_pool_t *pool)
{
    int nport;
    uint8_t *port_buf;
    size_t port_buf_size = MAX(v->len, 8) + 1;

    // 缓冲value
    port_buf  = (uint8_t *)lts_palloc(pool, port_buf_size);
    (void)memcpy(port_buf, v->data, v->len);
    port_buf[v->len] = 0;

    // 检查端口合法性
    nport = atoi((char const *)port_buf);
    if ((nport < 1) || (nport > 65535)) {
        nport = 6742;
    }

    // 更新配置
    lts_str_init(&conf->port, port_buf, port_buf_size - 1);
    assert(0 == lts_l2str(&conf->port, nport));

    return;
}


static void cb_pid_file_match(lts_conf_t *conf,
                              lts_str_t *k,
                              lts_str_t *v,
                              lts_pool_t *pool)
{
    uint8_t *pid_file_buf;
    size_t pid_file_buf_size = MAX(v->len, 8) + 1;

    // 缓冲value
    pid_file_buf  = (uint8_t *)lts_palloc(pool, pid_file_buf_size);
    (void)memcpy(pid_file_buf, v->data, v->len);
    pid_file_buf[v->len] = 0;

    // 更新配置
    lts_str_init(&conf->pid_file, pid_file_buf, v->len);

    return;
}


static void cb_log_file_match(lts_conf_t *conf,
                              lts_str_t *k,
                              lts_str_t *v,
                              lts_pool_t *pool)
{
    uint8_t *log_file_buf;
    size_t log_file_buf_size = MAX(v->len, 8) + 1;

    // 缓冲value
    log_file_buf  = (uint8_t *)lts_palloc(pool, log_file_buf_size);
    (void)memcpy(log_file_buf, v->data, v->len);
    log_file_buf[v->len] = 0;

    // 更新配置
    lts_str_init(&conf->log_file, log_file_buf, v->len);

    return;
}


// 工作进程配置
static void cb_workers_match(lts_conf_t *conf,
                             lts_str_t *k,
                             lts_str_t *v,
                             lts_pool_t *pool)
{
    int nworkers;
    uint8_t *port_buf;
    size_t port_buf_size = MAX(v->len, 8) + 1;

    // 缓冲value
    port_buf  = (uint8_t *)lts_palloc(pool, port_buf_size);
    (void)memcpy(port_buf, v->data, v->len);
    port_buf[v->len] = 0;

    // 更新配置
    nworkers = atoi((char const *)port_buf);
    if ((nworkers < 1) || (nworkers > 512)) {
        nworkers = 1;
    }
    conf->workers = nworkers;

    return;
}


// 连接超时配置
static void cb_keepalive_match(lts_conf_t *conf,
                               lts_str_t *k,
                               lts_str_t *v,
                               lts_pool_t *pool)
{
    int nkeepalive;
    uint8_t *port_buf;
    size_t port_buf_size = MAX(v->len, 8) + 1;

    // 缓冲value
    port_buf  = (uint8_t *)lts_palloc(pool, port_buf_size);
    (void)memcpy(port_buf, v->data, v->len);
    port_buf[v->len] = 0;

    // 更新配置
    nkeepalive = atoi((char const *)port_buf);
    if ((nkeepalive < 0) || (nkeepalive > MAX_KEEPALIVE)) { // 不超过一天
        nkeepalive = MAX_KEEPALIVE;
    }
    conf->keepalive = nkeepalive;

    return;
}


static conf_item_t conf_items[] = {
    {lts_string("daemon"), &cb_daemon_match},
    {lts_string("port"), &cb_port_match},
    {lts_string("workers"), &cb_workers_match},
    {lts_string("pid_file"), &cb_pid_file_match},
    {lts_string("log_file"), &cb_log_file_match},
    {lts_string("keepalive"), &cb_keepalive_match},
};

static int load_conf_file(lts_file_t *file, uint8_t **addr, off_t *sz)
{
    struct stat st;

    if (-1 == lts_file_open(file, O_RDONLY, S_IWUSR | S_IRUSR,
                            &lts_stderr_logger))
    {
        (void)lts_write_logger(&lts_stderr_logger, LTS_LOG_ERROR,
                               "%s:open configuration file failed\n",
                               STR_LOCATION);
        return -1;
    }

    if (-1 == fstat(file->fd, &st)) {
        lts_file_close(file);
        (void)lts_write_logger(&lts_stderr_logger, LTS_LOG_ERROR,
                               "%s:fstat() failed:%s\n",
                               STR_LOCATION, lts_errno_desc[errno]);
        return -1;
    }

    // 配置文件size检查
    if (st.st_size > MAX_CONF_SIZE) {
        lts_file_close(file);
        (void)lts_write_logger(&lts_stderr_logger, LTS_LOG_ERROR,
                               "%s:too large configuration file\n",
                               STR_LOCATION);
        return -1;
    }
    *sz = st.st_size;

    *addr = (uint8_t *)mmap(NULL, st.st_size,
                            PROT_READ | PROT_WRITE, MAP_PRIVATE, file->fd, 0);
    if (MAP_FAILED == *addr) {
        lts_file_close(file);
        (void)lts_write_logger(&lts_stderr_logger, LTS_LOG_ERROR,
                               "%s:mmap() failed: %s\n",
                               STR_LOCATION, lts_errno_desc[errno]);
        return -1;
    }

    return 0;
}

static void close_conf_file(lts_file_t *file, uint8_t *addr, off_t sz)
{
    if (-1 == munmap(addr, sz)) {
        (void)lts_write_logger(&lts_stderr_logger, LTS_LOG_ERROR,
                               "%s:munmap() configure file failed:%s\n",
                               STR_LOCATION, lts_errno_desc[errno]);
    }
    lts_file_close(file);

    return;
}


/*
static void log_invalid_conf(lts_str_t *item, lts_pool_t *pool)
{
    char *tmp;

    tmp = (char *)lts_palloc(pool, item->len + 1);
    (void)memmove(tmp, item->data, item->len);
    tmp[item->len] = '\0';
    (void)lts_write_logger(
        &lts_stderr_logger, LTS_LOG_EMERGE,
        "%s:invalid conf '%s'\n", STR_LOCATION, tmp
    );

    return;
}
*/


// json配置解析
static int parse_conf(lts_conf_t *conf,
                      uint8_t *addr,
                      off_t sz,
                      lts_pool_t *pool)
{
    lts_sjson_t conf_json;
    lts_sjson_obj_node_t *iter;
    lts_str_t conf_text = {addr, sz};

    // 过滤注释
    for (size_t i = 0; i < conf_text.len; ++i) {
        size_t start, len;

        if ('#' != conf_text.data[i]) {
            continue;
        }

        start = i;
        for (len = i + 1; len < conf_text.len; ++len) {
            if ('\n' == conf_text.data[len]) {
                break;
            }
        }
        len -= start;

        lts_str_hollow(&conf_text, start, len);
    }

    // 解码json
    if (lts_sjson_decode(&conf_text, pool, &conf_json)) {
        (void)lts_write_logger(
            &lts_stderr_logger, LTS_LOG_WARN, "%s:invalid json\n", STR_LOCATION
        );
        return -1;
    }

    for (iter = lts_sjson_first(&conf_json);
         iter; iter = lts_sjson_next(iter)) {
        lts_sjson_kv_t *kv;

        // 忽略复杂类型的结点
        if (STRING_VALUE != iter->node_type) {
            continue;
        }

        kv = CONTAINER_OF(iter, lts_sjson_kv_t, _obj_node);

        for (int j = 0; j < (int)ARRAY_COUNT(conf_items); ++j) {
            if (lts_str_compare(&conf_items[j].name, &iter->key)) {
                continue;
            }

            if (NULL == conf_items[j].match_handler) {
                abort();
            }

            conf_items[j].match_handler(conf, &iter->key, &kv->val, pool);
            break;
        }
    }

    return 0;
}


// 键值对型配置解析
/*
static int parse_conf(lts_conf_t *conf,
                      uint8_t *addr,
                      off_t sz,
                      lts_pool_t *pool)
{
    lts_str_t *iter;
    lts_str_t conf_text = {addr, sz};

    iter = split_str(&conf_text, '\n', pool); // 换行分割
    for (int i = 0; iter[i].data; ++i) {
        int iter_len, valid_item;
        lts_str_t *kv;

        // 过滤注释
        iter_len = iter[i].len;
        iter[i].len = 0;
        for (int j = 0; j < iter_len; ++j) {
            if ('#' == iter[i].data[j]) {
                break;
            }

            ++iter[i].len;
        }

        // 空项
        if (0 == iter[i].len) {
            continue;
        }

        kv = split_str(&iter[i], '=', pool); // 等号分割

        // 无效键值
        if ((0 == kv[1].len) || (0 == kv[1].len)) {
            log_invalid_conf(&iter[i], pool);

            return -1;
        }

        // 过滤前后空白
        lts_str_trim(&kv[0]);
        lts_str_trim(&kv[1]);

        // 处理配置项
        valid_item = 0;
        for (int j = 0; j < (int)ARRAY_COUNT(conf_items); ++j) {
            if (0 == lts_str_compare(&conf_items[j].name, &kv[0])) {
                if (NULL == conf_items[j].match_handler) {
                    abort();
                }
                conf_items[j].match_handler(conf, &kv[0], &kv[1], pool);
                valid_item = 1;
                break;
            }
        }

        if (! valid_item) {
            log_invalid_conf(&iter[i], pool);

            return -1;
        }
    }

    return 0;
}
*/


/*
 * 配置解析早期实现
static int
parse_conf(lts_conf_t *conf, uint8_t *addr, off_t sz, lts_pool_t *pool)
{
    uint8_t *start, *end;

    start = addr;
    while (*start) {
        if ((0x09 == *start) || (0x0A == *start)
                || (0x0D == *start) || (0x20 == *start))
        {
            ++start;
            continue;
        }

        if ('#' == *start) {
            while (0x0A != (*start++));
            continue;
        }

        for (end = start; TRUE; ++end) {
            uint8_t *kv;
            size_t kv_len = end - start;
            size_t seprator;
            lts_str_t k_str, v_str, kv_str;

            if (0 == *end) {
                if (*start) {
                    kv = (uint8_t *)alloca(kv_len + 1);
                    (void)memcpy(kv, start, kv_len);
                    kv[kv_len] = 0;

                    (void)lts_write_logger(
                        &lts_stderr_logger, LTS_LOG_EMERGE,
                        "invalid configration near '%s'\n", kv
                    );

                    return -1;
                }
                start = end;
                break;
            }

            if ('#' == *end) {
                kv = (uint8_t *)alloca(kv_len + 1);
                (void)memcpy(kv, start, kv_len);
                kv[kv_len] = 0;

                (void)lts_write_logger(
                    &lts_stderr_logger, LTS_LOG_EMERGE,
                    "invalid configration near '%s'\n", kv
                );

                return -1;
            }

            if (';' == *end) {
                kv = (uint8_t *)alloca(kv_len + 1);
                (void)memcpy(kv, start, kv_len);
                kv[kv_len] = 0;
                for (seprator = 0; ('=' != kv[seprator]); ++seprator) {
                    if (0 == kv[seprator]) { // 无效配置
                        (void)lts_write_logger(
                            &lts_stderr_logger, LTS_LOG_EMERGE,
                            "invalid configration near '%s'\n", kv
                        );

                        return -1;
                    }
                }
                lts_str_init(&kv_str, kv, kv_len);

                // 过滤干扰字符
                (void)lts_str_filter(&kv_str, 0x20);
                (void)lts_str_filter(&kv_str, 0x0A);
                (void)lts_str_filter(&kv_str, 0x0D);

                // 重寻分隔符
                for (seprator = 0; '=' != kv_str.data[seprator]; ++seprator);

                // 处理配置项
                lts_str_init(&k_str, &kv_str.data[0], seprator);
                lts_str_init(&v_str, &kv_str.data[seprator + 1],
                             kv_str.len - (seprator + 1));
                for (size_t i = 0; i < ARRAY_COUNT(conf_items); ++i) {
                    if (lts_str_compare(&conf_items[i].name, &k_str)) {
                        continue;
                    }

                    if (NULL == conf_items[i].match_handler) {
                        // 不能识别的配置项
                        break;
                    }

                    conf_items[i].match_handler(conf, &k_str, &v_str, pool);
                }
                start = end + 1;
                break;
            }
        }
    }

    return 0;
}
*/

// 默认配置
lts_conf_t lts_conf = {
    FALSE, // 守护进程
    lts_string("6742"), // 监听端口
    1, // slave进程数
    MAX_CONNECTIONS, // 每个slave最大连接数
    lts_string("latasia.pid"), // pid文件路径
    lts_string("latasia.log"), // 日志路径
    60, // 连接超时

    lts_string("/"),
};

int lts_load_config(lts_conf_t *conf, lts_pool_t *pool)
{
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
    rslt = parse_conf(conf, addr, sz, pool);
    // rslt = parse_conf(conf, addr, sz, pool);
    close_conf_file(&lts_conf_file, addr, sz);

    return rslt;
}

#include <sys/mman.h>
#include <alloca.h>

#include "conf.h"
#include "file.h"
#include "logger.h"


#define CONF_FILE           "latasia.conf"


typedef struct {
    lts_str_t name;
    void (*match_handler)(lts_conf_t *conf, lts_pool_t *pool,
                          lts_str_t const *, lts_str_t const *);
} conf_item_t;


// 端口配置
static void cb_port_match(lts_conf_t *conf, lts_pool_t *pool,
                          lts_str_t const *k, lts_str_t const *v)
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
    if ((nport < 1024) || (nport > 65535)) {
        nport = 22122;
    }

    // 更新配置
    lts_str_init(&conf->port, port_buf, port_buf_size - 1);
    assert(0 == lts_l2str(&conf->port, nport));

    return;
}

static void cb_log_file_match(lts_conf_t *conf, lts_pool_t *pool,
                              lts_str_t const *k, lts_str_t const *v)
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


// 缓存配置
static void cb_servers_match(lts_conf_t *conf, lts_pool_t *pool,
                             lts_str_t const *k, lts_str_t const *v)
{
    int cache_count = 1;
    size_t last = 0;
    uint8_t *backends_buf;
    size_t backends_buf_size;
    lts_str_t prefix = lts_string("--SERVER=");

    if ((',' == v->data[0]) || (',' == v->data[v->len - 1])) {
        (void)lts_write_logger(&lts_stderr_logger, LTS_ERROR,
                               "invalid cache server configuration\n");
        return;
    }

    for (size_t i = 0; i < v->len; ++i) {
        if (',' == v->data[i]) {
            ++cache_count;
        }
    }
    backends_buf_size = (v->len + prefix.len * cache_count + 1);
    backends_buf = (uint8_t *)lts_palloc(pool, backends_buf_size);
    for (size_t i = 0, j = 0; TRUE; ++j) {
        if ((',' == v->data[j]) || (j == v->len)) {
            (void)memcpy(&backends_buf[last], prefix.data, prefix.len);
            last += prefix.len;
            (void)memcpy(&backends_buf[last], &v->data[i], j - i);
            last += j - i;
            i = ++j;
            backends_buf[last] = 0x20;
            ++last;
        }
        if (j > v->len) {
            break;
        }
    }
    backends_buf[backends_buf_size - 1] = 0;
    lts_str_init(&conf->backends,
                 backends_buf, backends_buf_size - 1);

    return;
}


// 工作进程配置
static void cb_workers_match(lts_conf_t *conf, lts_pool_t *pool,
                             lts_str_t const *k, lts_str_t const *v)
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
static void cb_keepalive_match(lts_conf_t *conf, lts_pool_t *pool,
                               lts_str_t const *k, lts_str_t const *v)
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
    if ((nkeepalive < 0) || (nkeepalive > 300)) {
        nkeepalive = 300;
    }
    conf->keepalive = nkeepalive;

    return;
}


static conf_item_t conf_items[] = {
    {lts_string("port"), &cb_port_match},
    {lts_string("workers"), &cb_workers_match},
    {lts_string("log_file"), &cb_log_file_match},
    {lts_string("servers"), &cb_servers_match},
    {lts_string("keepalive"), &cb_keepalive_match},
};

lts_conf_t lts_conf = {
    1, lts_string("22122"),
    lts_string("tm_lts.log"),
    lts_string("--SERVER=127.0.0.1"), 60,
};

static int load_conf_file(lts_file_t *file, uint8_t **addr, off_t *sz)
{
    struct stat st;

    if (-1 == lts_file_open(file, O_RDONLY, S_IWUSR | S_IRUSR,
                            &lts_stderr_logger))
    {
        (void)lts_write_logger(&lts_stderr_logger, LTS_ERROR,
                               "open configuration file failed\n");
        return -1;
    }

    if (-1 == fstat(file->fd, &st)) {
        lts_file_close(file);
        (void)lts_write_logger(&lts_stderr_logger, LTS_ERROR,
                               "fstat configuration file failed: %s\n",
                               strerror(errno));
        return -1;
    }

    // 配置文件size检查
    if (st.st_size > ((~0u) >> 1)) {
        lts_file_close(file);
        (void)lts_write_logger(&lts_stderr_logger, LTS_ERROR,
                               "too large configuration file\n");
        return -1;
    }
    *sz = st.st_size;

    *addr = (uint8_t *)mmap(NULL, st.st_size,
                            PROT_READ | PROT_WRITE, MAP_PRIVATE, file->fd, 0);
    if (MAP_FAILED == *addr) {
        lts_file_close(file);
        (void)lts_write_logger(&lts_stderr_logger, LTS_ERROR,
                               "mmap configuration file failed: %s\n",
                               strerror(errno));
        return -1;
    }

    return 0;
}

static void close_conf_file(lts_file_t *file, uint8_t *addr, off_t sz)
{
    if (-1 == munmap(addr, sz)) {
        (void)lts_write_logger(&lts_stderr_logger, LTS_ERROR,
                               "munmap configure file failed(%d)\n", errno);
    }
    lts_file_close(file);

    return;
}

static int parse_conf(lts_conf_t *conf, lts_pool_t *pool,
                      uint8_t *addr, off_t sz)
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
                        &lts_stderr_logger, LTS_ERROR,
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
                    &lts_stderr_logger, LTS_ERROR,
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
                            &lts_stderr_logger, LTS_ERROR,
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

                    conf_items[i].match_handler(conf, pool, &k_str, &v_str);
                }
                start = end + 1;
                break;
            }
        }
    }

    return 0;
}

int lts_load_config(lts_conf_t *conf, lts_pool_t *pool)
{
    off_t sz;
    uint8_t *addr;
    lts_file_t lts_conf_file = {
        -1,
        {
            (uint8_t *)CONF_FILE, sizeof(CONF_FILE) - 1,
        },
    };

    if (-1 == load_conf_file(&lts_conf_file, &addr, &sz)) {
        return -1;
    }

    // 解析配置
    if (-1 == parse_conf(conf, pool, addr, sz)) {
        close_conf_file(&lts_conf_file, addr, sz);
        return -1;
    }

    close_conf_file(&lts_conf_file, addr, sz);

    return 0;
}

/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#include <sys/mman.h>

#include "extra_errno.h"
#include "conf.h"
#include "simple_json.h"
#include "logger.h"

#define __THIS_FILE__       "src/conf.c"


int load_conf_file(lts_file_t *file, uint8_t **addr, off_t *sz)
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


void close_conf_file(lts_file_t *file, uint8_t *addr, off_t sz)
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


// json配置解析，将配置文件内容转换为相应的配置结构体
// addr, sz: 配置文件内容
// conf_items: 需要处理的配置项
// pool: 内存池
// conf: 配置输出结构体
int parse_conf(uint8_t *addr, off_t sz,
               lts_conf_item_t *conf_items[],
               lts_pool_t *pool, void *conf)
{
    lts_sjson_t conf_json = lts_empty_sjson(pool);
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
    if (lts_sjson_decode(&conf_text, &conf_json)) {
        (void)lts_write_logger(
            &lts_stderr_logger, LTS_LOG_WARN, "%s:invalid json\n", STR_LOCATION
        );
        return -1;
    }

    for (iter = lts_sjson_first(&conf_json);
         iter; iter = lts_sjson_next(iter)) {
        // for loop
        lts_sjson_kv_t *kv;

        // 忽略复杂类型的结点
        if (STRING_VALUE != iter->node_type) {
            continue;
        }

        kv = CONTAINER_OF(iter, lts_sjson_kv_t, _obj_node);

        for (int j = 0; conf_items[j]; ++j) {
            if (lts_str_compare(&conf_items[j]->name, &iter->key)) {
                continue;
            }

            if (conf_items[j]->match_handler) {
                conf_items[j]->match_handler(
                    conf, &iter->key, &kv->val, pool
                );
                break;
            }

            abort();
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
        for (int j = 0; j < (int)ARRAY_COUNT(s_conf_items); ++j) {
            if (0 == lts_str_compare(&s_conf_items[j].name, &kv[0])) {
                if (NULL == s_conf_items[j].match_handler) {
                    abort();
                }
                s_conf_items[j].match_handler(conf, &kv[0], &kv[1], pool);
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
                    (void)memmove(kv, start, kv_len);
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
                (void)memmove(kv, start, kv_len);
                kv[kv_len] = 0;

                (void)lts_write_logger(
                    &lts_stderr_logger, LTS_LOG_EMERGE,
                    "invalid configration near '%s'\n", kv
                );

                return -1;
            }

            if (';' == *end) {
                kv = (uint8_t *)alloca(kv_len + 1);
                (void)memmove(kv, start, kv_len);
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
                for (size_t i = 0; i < ARRAY_COUNT(s_conf_items); ++i) {
                    if (lts_str_compare(&s_conf_items[i].name, &k_str)) {
                        continue;
                    }

                    if (NULL == s_conf_items[i].match_handler) {
                        // 不能识别的配置项
                        break;
                    }

                    s_conf_items[i].match_handler(conf, &k_str, &v_str, pool);
                }
                start = end + 1;
                break;
            }
        }
    }

    return 0;
}
*/

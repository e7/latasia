/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#include "simple_json.h"
#include "extra_errno.h"


// 状态机
enum {
    SJSON_EXP_START = 1,
    SJSON_EXP_K_QUOT_START_OR_END,
    SJSON_EXP_K_QUOT_START,
    SJSON_EXP_K_QUOT_END,
    SJSON_EXP_COLON,
    SJSON_EXP_V_MAP_OR_QUOT_OR_BRACKET_START,
    SJSON_EXP_V_QUOT_START,
    SJSON_EXP_V_QUOT_END,
    SJSON_EXP_COMMA_OR_BRACKET_END,
    SJSON_EXP_COMMA_OR_END,
    SJSON_EXP_END,
    SJSON_EXP_NOTHING,
};


// link，节点不存在时是否挂到树上
static lts_sjson_key_t *__lts_sjon_search(lts_rb_root_t *root,
                                          lts_sjson_key_t *key,
                                          int link)
{
    lts_sjson_key_t *s;
    lts_rb_node_t *parent, **iter;

    parent = NULL;
    iter = &root->rb_node;
    while (*iter) {
        int balance;
        parent = *iter;
        s = rb_entry(parent, lts_sjson_key_t, rb_node);

        balance = lts_str_compare(&key->key, &s->key);
        if (balance < 0) {
            iter = &(parent->rb_left);
        } else if (balance > 0) {
            iter = &(parent->rb_right);
        } else {
            return s;
        }
    }

    if (link) {
        rb_link_node(&key->rb_node, parent, iter);
        rb_insert_color(&key->rb_node, root);
    }

    return key;
}


ssize_t lts_sjon_encode_size(lts_sjson_t *sjson)
{
    return 0;
}


int lts_sjon_encode(lts_sjson_t *sjson, lts_pool_t *pool, lts_str_t *output)
{
    return 0;
}


int lts_sjon_decode(lts_str_t *src, lts_pool_t *pool, lts_sjson_t *output)
{
    static uint8_t invisible[] = {'\t', '\n', '\r', '\x20'};

    int rdepth = 0; // 嵌套层次
    int in_bracket = FALSE;
    int current_stat = SJSON_EXP_START;
    lts_sjson_key_t *newkey;

    // 过滤不可见字符
    (void)lts_str_filter_multi(src, invisible, ARRAY_COUNT(invisible));

    for (size_t i = 0; i < src->len; ++i) {
        switch (current_stat) {
        case SJSON_EXP_START:
        {
            if ('{' != src->data[i]) {
                errno = LTS_E_INVALID_FORMAT;
                return -1;
            }

            current_stat = SJSON_EXP_K_QUOT_START_OR_END; // only
            continue;
        }

        case SJSON_EXP_K_QUOT_START_OR_END:
        {
            if ('"' == src->data[i]) {
                current_stat = SJSON_EXP_K_QUOT_END;

                newkey = (lts_sjson_key_t *)lts_palloc(
                    pool, sizeof(lts_sjson_key_t)
                );
                if (NULL == newkey) {
                    errno = LTS_E_NOMEM;
                    return -1;
                }

                newkey->key.data = &src->data[i + 1];
                newkey->key.len = 0;
            } else if ('}' == src->data[i]) {
                if (rdepth) {
                    --rdepth;
                    current_stat = SJSON_EXP_COMMA_OR_END; // only
                } else {
                    current_stat = SJSON_EXP_NOTHING; // only
                }
            } else {
                errno = LTS_E_INVALID_FORMAT;
                return -1;
            }
            continue;
        }

        case SJSON_EXP_K_QUOT_START:
        {
            if ('"' == src->data[i]) {
                current_stat = SJSON_EXP_K_QUOT_END;
            }
            continue;
        }

        case SJSON_EXP_K_QUOT_END:
        {
            if ('"' == src->data[i]) {
                current_stat = SJSON_EXP_COLON; // only
            }
            continue;
        }

        case SJSON_EXP_NOTHING:
        {
            errno = LTS_E_INVALID_FORMAT;
            return -1;
        }

        case SJSON_EXP_COLON:
        {
            if (':' != src->data[i]) {
                errno = LTS_E_INVALID_FORMAT;
                return -1;
            }

            current_stat = SJSON_EXP_V_MAP_OR_QUOT_OR_BRACKET_START; // only
            continue;
        }

        case SJSON_EXP_V_MAP_OR_QUOT_OR_BRACKET_START:
        {
            if ('"' == src->data[i]) {
                current_stat = SJSON_EXP_V_QUOT_END;
            } else if ('[' == src->data[i]) {
                in_bracket = TRUE;
                current_stat = SJSON_EXP_V_QUOT_START; // only
            } else if ('{' == src->data[i]) {
                ++rdepth;
                current_stat = SJSON_EXP_K_QUOT_START_OR_END; // only
            } else {
                errno = LTS_E_INVALID_FORMAT;
                return -1;
            }
            continue;
        }

        case SJSON_EXP_V_QUOT_START:
        {
            if ('"' != src->data[i]) {
                errno = LTS_E_INVALID_FORMAT;
                return -1;
            }

            current_stat = SJSON_EXP_V_QUOT_END;
            continue;
        }

        case SJSON_EXP_V_QUOT_END:
        {
            if ('"' == src->data[i]) {
                if (in_bracket) {
                    current_stat = SJSON_EXP_COMMA_OR_BRACKET_END; // only
                } else {
                    current_stat = SJSON_EXP_COMMA_OR_END; // only
                }
            }
            continue;
        }

        case SJSON_EXP_COMMA_OR_BRACKET_END:
        {
            // 必定在bracket中
            if (',' == src->data[i]) {
                current_stat = SJSON_EXP_V_QUOT_START; // only
            } else if (']' == src->data[i]) {
                in_bracket = FALSE;
                current_stat = SJSON_EXP_COMMA_OR_END; // only
            } else {
                errno = LTS_E_INVALID_FORMAT;
                return -1;
            }
            continue;
        }

        case SJSON_EXP_COMMA_OR_END:
        {
            // 必定不在bracket中
            if (',' == src->data[i]) {
                current_stat = SJSON_EXP_K_QUOT_START;
            } else if ('}' == src->data[i]) {
                if (rdepth > 0) {
                    --rdepth;
                    current_stat = SJSON_EXP_COMMA_OR_END;
                } else {
                    current_stat = SJSON_EXP_NOTHING;
                }
            } else {
                errno = LTS_E_INVALID_FORMAT;
                return -1;
            }
            continue;
        }

        default:
        {
            abort();
        }}
    }

    return (SJSON_EXP_NOTHING == current_stat) ? 0 : -1;
}

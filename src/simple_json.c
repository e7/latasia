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
static lts_sjson_obj_node_t *__lts_sjon_search(lts_rb_root_t *root,
                                               lts_sjson_obj_node_t *obj_node,
                                               int link)
{
    lts_sjson_obj_node_t *s;
    lts_rb_node_t *parent, **iter;

    parent = NULL;
    iter = &root->rb_node;
    while (*iter) {
        int balance;
        parent = *iter;
        s = rb_entry(parent, lts_sjson_obj_node_t, rb_node);

        balance = lts_str_compare(&obj_node->key, &s->key);
        if (balance < 0) {
            iter = &(parent->rb_left);
        } else if (balance > 0) {
            iter = &(parent->rb_right);
        } else {
            return s;
        }
    }

    if (link) {
        rb_link_node(&obj_node->rb_node, parent, iter);
        rb_insert_color(&obj_node->rb_node, root);
    }

    return obj_node;
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

    DEFINE_LSTACK(obj_stack);
    int rdepth = 0; // 嵌套层次
    int in_bracket = FALSE;
    int current_stat = SJSON_EXP_START;
    lts_str_t current_key = (lts_str_t)lts_null_string;
    lts_sjson_kv_t *json_kv = NULL;
    lts_sjson_li_node_t *li_node = NULL;
    lts_sjson_list_t *json_list = NULL;
    lts_sjson_t *json_obj = NULL;

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

                current_key.data = &src->data[i + 1];
                current_key.len = 0;
            } else if ('}' == src->data[i]) {
                // 空对象
                if (rdepth) {
                    --rdepth;
                    current_stat = SJSON_EXP_COMMA_OR_END; // only
                    lstack_pop(&obj_stack);
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

                current_key.data = &src->data[i + 1];
                current_key.len = 0;
            }
            continue;
        }

        case SJSON_EXP_K_QUOT_END:
        {
            if ('"' == src->data[i]) {
                current_stat = SJSON_EXP_COLON; // only
            } else {
                ++current_key.len;
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

                json_kv = (lts_sjson_kv_t *)lts_palloc(pool, sizeof(*json_kv));
                if (NULL == json_kv) {
                    errno = LTS_E_NOMEM;
                    return -1;
                }

                lts_str_copy(&json_kv->obj_node.key, &current_key);
                json_kv->val.data = &src->data[i + 1];
                json_kv->val.len = 0;
                json_kv->obj_node.node_type = STRING_VALUE;
                json_kv->obj_node.rb_node = RB_NODE;
            } else if ('[' == src->data[i]) {
                in_bracket = TRUE;
                current_stat = SJSON_EXP_V_QUOT_START; // only

                json_list = (lts_sjson_list_t *)lts_palloc(pool,
                                                           sizeof(*json_list));
                if (NULL == json_list) {
                    errno = LTS_E_NOMEM;
                    return -1;
                }

                lts_str_copy(&json_list->obj_node.key, &current_key);
                list_set_empty(&json_list->val);
                json_list->obj_node.node_type = LIST_VALUE;
                json_list->obj_node.rb_node = RB_NODE;
            } else if ('{' == src->data[i]) {
                lts_sjson_t *new_obj;

                current_stat = SJSON_EXP_K_QUOT_START_OR_END; // only
                ++rdepth;

                new_obj = (lts_sjson_t *)lts_palloc(pool, sizeof(*json_obj));
                if (NULL == new_obj) {
                    errno = LTS_E_NOMEM;
                    return -1;
                }

                lts_str_copy(&new_obj->obj_node.key, &current_key);
                new_obj->val = RB_ROOT;
                new_obj->obj_node.node_type = OBJ_VALUE;
                new_obj->obj_node.rb_node = RB_NODE;

                // 挂到树上
                if (lstack_is_empty(&obj_stack)) {
                    json_obj = output;
                } else {
                    json_obj = CONTAINER_OF(
                        lstack_top(&obj_stack), lts_sjson_t, _stk_node
                    );
                }
                __lts_sjon_search(&json_obj->val, &new_obj->obj_node, TRUE);

                // 压栈
                lstack_push(&obj_stack, &new_obj->_stk_node);

                json_obj = NULL;
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

            if (! in_bracket) {
                abort();
            }

            current_stat = SJSON_EXP_V_QUOT_END;

            li_node = (lts_sjson_li_node_t *)lts_palloc(pool,
                                                        sizeof(*li_node));
            if (NULL == li_node) {
                errno = LTS_E_NOMEM;
                return -1;
            }
            li_node->val.data = &src->data[i + 1];
            li_node->val.len = 0;

            continue;
        }

        case SJSON_EXP_V_QUOT_END:
        {
            if (in_bracket) {
                if ('"' == src->data[i]) {
                    current_stat = SJSON_EXP_COMMA_OR_BRACKET_END; // only

                    list_add_node(&json_list->val, &li_node->node);
                    li_node = NULL;
                } else {
                    ++li_node->val.len;
                }
            } else {
                if ('"' == src->data[i]) {
                    current_stat = SJSON_EXP_COMMA_OR_END; // only

                    if (lstack_is_empty(&obj_stack)) {
                        json_obj = output;
                    } else {
                        json_obj = CONTAINER_OF(
                            lstack_top(&obj_stack), lts_sjson_t, _stk_node
                        );
                    }

                    __lts_sjon_search(&json_obj->val,
                                      &json_kv->obj_node,
                                      TRUE);
                    json_kv = NULL;
                } else {
                    ++json_kv->val.len;
                }
            }
            continue;
        }

        case SJSON_EXP_COMMA_OR_BRACKET_END:
        {
            // 必定在bracket中
            if (! in_bracket) {
                abort();
            }

            if (',' == src->data[i]) {
                current_stat = SJSON_EXP_V_QUOT_START; // only
            } else if (']' == src->data[i]) {
                in_bracket = FALSE;
                current_stat = SJSON_EXP_COMMA_OR_END; // only

                if (lstack_is_empty(&obj_stack)) {
                    json_obj = output;
                } else {
                    json_obj = CONTAINER_OF(
                        lstack_top(&obj_stack), lts_sjson_t, _stk_node
                    );
                }

                __lts_sjon_search(&json_obj->val,
                                  &json_list->obj_node,
                                  TRUE);
                json_list = NULL;
            } else {
                errno = LTS_E_INVALID_FORMAT;
                return -1;
            }
            continue;
        }

        case SJSON_EXP_COMMA_OR_END:
        {
            // 必定不在bracket中
            if (in_bracket) {
                abort();
            }

            if (',' == src->data[i]) {
                current_stat = SJSON_EXP_K_QUOT_START;
            } else if ('}' == src->data[i]) {
                if (rdepth > 0) {
                    --rdepth;
                    current_stat = SJSON_EXP_COMMA_OR_END;

                    lstack_pop(&obj_stack);
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

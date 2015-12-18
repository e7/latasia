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


// 查询或添加结点
// link，结点不存在时是否挂到树上
static lts_sjson_obj_node_t *__lts_sjson_search(lts_rb_root_t *root,
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


ssize_t lts_sjson_encode_size(lts_sjson_t *sjson)
{
    ssize_t sz;
    lts_rb_node_t *p;

    sz = 2; // {}
    p = rb_first(&sjson->val);
    while (p) {
        lts_sjson_obj_node_t *node = CONTAINER_OF(
            p, lts_sjson_obj_node_t, rb_node
        );

        if (STRING_VALUE == node->node_type) {
            lts_sjson_kv_t *kv = CONTAINER_OF(node, lts_sjson_kv_t, _obj_node);

            sz += node->key.len + 2; // ""
            ++sz; // :
            sz += kv->val.len + 2; // ""
        } else if (LIST_VALUE == node->node_type) {
            list_t *it;
            lts_sjson_list_t *lv = CONTAINER_OF(
                node, lts_sjson_list_t, _obj_node
            );

            sz += 2; // []
            sz += node->key.len + 2; // ""
            ++sz; // :
            it = list_first(&lv->val);
            while (it) {
                lts_sjson_li_node_t *ln = CONTAINER_OF(
                    it, lts_sjson_li_node_t, node
                );

                sz += ln->val.len + 2; // ""
                ++sz; // ,

                it = list_next(it);
            }
        } else if (OBJ_VALUE == node->node_type) {
            lts_sjson_t *ov = CONTAINER_OF(node, lts_sjson_t, _obj_node);

            sz += node->key.len + 2; // ""
            ++sz; // :
            sz += lts_sjson_encode_size(ov);
        } else {
            return -1;
        }
        ++sz; // ,

        p = rb_next(p);
    }

    return ++sz; // harmless extra size
}


static void __lts_sjson_encode(lts_sjson_t *sjson,
                               lts_str_t *output,
                               ssize_t i)
{
    lts_rb_node_t *p;

    // 序列化
    output->data[i++] = '{';
    p = rb_first(&sjson->val);
    while (p) {
        lts_rb_node_t *next;
        lts_sjson_obj_node_t *node = CONTAINER_OF(
            p, lts_sjson_obj_node_t, rb_node
        );

        if (STRING_VALUE == node->node_type) {
            lts_sjson_kv_t *kv = CONTAINER_OF(node, lts_sjson_kv_t, _obj_node);

            // key
            output->data[i++] = '"';
            (void)memcpy(&output->data[i], node->key.data, node->key.len);
            i += node->key.len;
            output->data[i++] = '"';

            output->data[i++] = ':';

            // val
            output->data[i++] = '"';
            (void)memcpy(&output->data[i], kv->val.data, kv->val.len);
            i += kv->val.len;
            output->data[i++] = '"';
        } else if (LIST_VALUE == node->node_type) {
            list_t *it;
            lts_sjson_list_t *lv = CONTAINER_OF(
                node, lts_sjson_list_t, _obj_node
            );

            // key
            output->data[i++] = '"';
            (void)memcpy(&output->data[i], node->key.data, node->key.len);
            i += node->key.len;
            output->data[i++] = '"';

            output->data[i++] = ':';

            // val
            output->data[i++] = '[';
            it = list_first(&lv->val);
            while (it) {
                list_t *next;
                lts_sjson_li_node_t *ln = CONTAINER_OF(
                    it, lts_sjson_li_node_t, node
                );

                output->data[i++] = '"';
                (void)memcpy(&output->data[i], ln->val.data, ln->val.len);
                i += ln->val.len;
                output->data[i++] = '"';

                next = list_next(it);
                if (next) {
                    output->data[i++] = ',';
                }
                it = next;
            }
            output->data[i++] = ']';
        } else if (OBJ_VALUE == node->node_type) {
            lts_sjson_t *ov = CONTAINER_OF(node, lts_sjson_t, _obj_node);

            // key
            output->data[i++] = '"';
            (void)memcpy(&output->data[i], node->key.data, node->key.len);
            i += node->key.len;
            output->data[i++] = '"';

            output->data[i++] = ':';

            __lts_sjson_encode(ov, output, i);
            i = output->len + 1;
        } else {
            abort();
        }

        next = rb_next(p);
        if (next) {
            output->data[i++] = ',';
        }
        p = next;
    }
    output->data[i++] = '}';
    output->len = i; // 回写长度

    return;
}

int lts_sjson_encode(lts_sjson_t *sjson, lts_pool_t *pool, lts_str_t *output)
{
    uint8_t *data;
    ssize_t data_sz;

    data_sz = lts_sjson_encode_size(sjson);
    if (-1 == data_sz) {
        return -1;
    }

    // 初始化缓冲区
    data = (uint8_t *)lts_palloc(pool, data_sz);
    if (NULL == data) {
        return -1;
    }

    (void)memset(data, 0, data_sz);
    output->data = data;
    output->len = 0;

    __lts_sjson_encode(sjson, output, 0);
    assert(output->len < data_sz);

    return 0;
}



int lts_sjson_decode(lts_str_t *src, lts_pool_t *pool, lts_sjson_t *output)
{
    static uint8_t invisible[] = {'\t', '\n', '\r', '\x20'};

    DEFINE_LSTACK(obj_stack);
    int in_bracket = FALSE;
    int current_stat = SJSON_EXP_START;
    lts_str_t current_key = lts_null_string;
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
                if (lstack_is_empty(&obj_stack)) {
                    current_stat = SJSON_EXP_NOTHING; // only
                } else {
                    current_stat = SJSON_EXP_COMMA_OR_END; // only
                    lstack_pop(&obj_stack);
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

                lts_str_copy(&json_kv->_obj_node.key, &current_key);
                json_kv->val.data = &src->data[i + 1];
                json_kv->val.len = 0;
                json_kv->_obj_node.node_type = STRING_VALUE;
                json_kv->_obj_node.rb_node = RB_NODE;
            } else if ('[' == src->data[i]) {
                in_bracket = TRUE;
                current_stat = SJSON_EXP_V_QUOT_START; // only

                json_list = (lts_sjson_list_t *)lts_palloc(pool,
                                                           sizeof(*json_list));
                if (NULL == json_list) {
                    errno = LTS_E_NOMEM;
                    return -1;
                }

                lts_str_copy(&json_list->_obj_node.key, &current_key);
                list_set_empty(&json_list->val);
                json_list->_obj_node.node_type = LIST_VALUE;
                json_list->_obj_node.rb_node = RB_NODE;
            } else if ('{' == src->data[i]) {
                lts_sjson_t *new_obj;

                current_stat = SJSON_EXP_K_QUOT_START_OR_END; // only

                new_obj = (lts_sjson_t *)lts_palloc(pool, sizeof(*json_obj));
                if (NULL == new_obj) {
                    errno = LTS_E_NOMEM;
                    return -1;
                }

                lts_str_copy(&new_obj->_obj_node.key, &current_key);
                new_obj->val = RB_ROOT;
                new_obj->_obj_node.node_type = OBJ_VALUE;
                new_obj->_obj_node.rb_node = RB_NODE;

                // 挂到树上
                if (lstack_is_empty(&obj_stack)) {
                    json_obj = output;
                } else {
                    json_obj = CONTAINER_OF(
                        lstack_top(&obj_stack), lts_sjson_t, _stk_node
                    );
                }
                __lts_sjson_search(&json_obj->val, &new_obj->_obj_node, TRUE);

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

                    __lts_sjson_search(&json_obj->val,
                                       &json_kv->_obj_node,
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

                __lts_sjson_search(&json_obj->val,
                                  &json_list->_obj_node,
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
                if (lstack_is_empty(&obj_stack)) {
                    current_stat = SJSON_EXP_NOTHING;
                } else {
                    current_stat = SJSON_EXP_COMMA_OR_END;
                    lstack_pop(&obj_stack);
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

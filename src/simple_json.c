/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#include "simple_json.h"
#include "extra_errno.h"


// 状态机
enum { // 期待的下一个字符
    // 注释中^表示起始，$表示结束
    SJSON_EXP_START = 1,
    SJSON_EXP_K_QUOT_START_OR_END, // ^" | $}
    SJSON_EXP_K_QUOT_START, // ^"
    SJSON_EXP_K_QUOT_END, // $"
    SJSON_EXP_COLON, // :
    SJSON_EXP_V_MAP_OR_QUOT_OR_BRACKET_START, // ^{ | ^" | ^[
    SJSON_EXP_V_QUOT_END, // $"
    SJSON_EXP_V_QUOT_START_OR_BRACKET_END, // ^" | $]
    SJSON_EXP_COMMA_OR_BRACKET_END, // , | $]
    SJSON_EXP_COMMA_OR_END, // , | $}
    SJSON_EXP_END, // $}
    SJSON_EXP_NOTHING,
};


// 查询或添加结点
// link，结点不存在时是否挂到树上
// 结点存在则返回存在的结点，不存在则返回obj_node
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
        s = rb_entry(parent, lts_sjson_obj_node_t, tnode);

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
        rb_link_node(&obj_node->tnode, parent, iter);
        rb_insert_color(&obj_node->tnode, root);
    }

    return obj_node;
}


// 构造sjson {{
lts_sjson_t *lts_sjson_add_sjson(lts_sjson_t *sjson, char const *key)
{
    lts_str_t tmpkey = {(uint8_t *)key, strlen(key)};

    return lts_sjson_add_sjson2(sjson, &tmpkey);
}


lts_sjson_t *lts_sjson_add_sjson2(lts_sjson_t *sjson, lts_str_t *key)
{
    lts_sjson_t *sub;
    lts_pool_t *pool = sjson->pool;

    sub = (lts_sjson_t *)lts_palloc(pool, sizeof(lts_sjson_t));
    if (NULL == sub) {
        return NULL;
    }

    // 初始化
    *sub = lts_key_sjson(*lts_str_clone(key, pool), pool);

    // attach
    if (&sub->_obj_node
        != __lts_sjson_search(&sjson->val, &sub->_obj_node, TRUE)) {
        return NULL;
    }

    return sub;
}


lts_sjson_kv_t *lts_sjson_add_kv(lts_sjson_t *sjson,
                                 char const *key, char const *val)
{
    lts_str_t tmpkey = {(uint8_t *)key, strlen(key)};
    lts_str_t tmpval = {(uint8_t *)val, strlen(val)};

    return lts_sjson_add_kv2(sjson, &tmpkey, &tmpval);
}


lts_sjson_kv_t *lts_sjson_add_kv2(lts_sjson_t *sjson,
                                  lts_str_t *key, lts_str_t *val)
{
    lts_pool_t *pool = sjson->pool;
    lts_str_t *clone_key, *clone_val;
    lts_sjson_kv_t *kv;

    kv = (lts_sjson_kv_t *)lts_palloc(pool, sizeof(lts_sjson_kv_t));
    clone_key = lts_str_clone(key, pool);
    clone_val = lts_str_clone(val, pool);

    // 初始化
    kv->val = *clone_val;
    kv->_obj_node.node_type = STRING_VALUE;
    kv->_obj_node.key = *clone_key;
    kv->_obj_node.tnode = RB_NODE;

    // attach
    if (&kv->_obj_node
        != __lts_sjson_search(&sjson->val, &kv->_obj_node, TRUE)) {
        return NULL;
    }

    return kv;
}
// }} 构造sjson


// 获取指定key的obj_node
lts_sjson_obj_node_t *lts_sjson_get_obj_node(lts_sjson_t *sjson,
                                             lts_str_t *key)
{
    lts_sjson_obj_node_t tmp_node;

    tmp_node.key = *key;

    return __lts_sjson_search(&sjson->val, &tmp_node, FALSE);
}


ssize_t lts_sjson_encode_size(lts_sjson_t *sjson)
{
    ssize_t sz;
    lts_rb_node_t *p;

    sz = 2; // {}
    p = rb_first(&sjson->val);
    if (NULL == p) {
        return sz;
    }

    while (p) {
        lts_sjson_obj_node_t *node = CONTAINER_OF(
            p, lts_sjson_obj_node_t, tnode
        );

        ++sz; // ,

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
            if (it) {
                while (it) {
                    lts_sjson_li_node_t *ln = CONTAINER_OF(
                        it, lts_sjson_li_node_t, node
                    );

                    sz += ln->val.len + 2; // ""
                    ++sz; // ,

                    it = list_next(it);
                }
                --sz; // 补偿第一次循环多算的逗号
            }
        } else if (OBJ_VALUE == node->node_type) {
            lts_sjson_t *ov = CONTAINER_OF(node, lts_sjson_t, _obj_node);

            sz += node->key.len + 2; // ""
            ++sz; // :
            sz += lts_sjson_encode_size(ov);
        } else {
            ASSERT(0);
        }

        p = rb_next(p);
    }

    return --sz; // 补偿第一次循环多算的逗号
}


lts_sjson_obj_node_t *lts_sjson_pop_min(lts_sjson_t *obj)
{
    lts_rb_node_t *p;
    lts_sjson_obj_node_t *obj_node;
    lts_rb_root_t *root = &obj->val;

    p = rb_first(root);
    if (NULL == p) {
        return NULL;
    }

    obj_node = CONTAINER_OF(p, lts_sjson_obj_node_t, tnode);
    rb_erase(p, root);

    return obj_node;
}


static void __lts_sjson_encode(lts_sjson_t *sjson,
                               lts_str_t *output,
                               ssize_t *offset)
{
    lts_rb_node_t *p;

    // 序列化
    output->data[(*offset)++] = '{';
    p = rb_first(&sjson->val);
    while (p) {
        lts_rb_node_t *next;
        lts_sjson_obj_node_t *node = CONTAINER_OF(
            p, lts_sjson_obj_node_t, tnode
        );

        if (STRING_VALUE == node->node_type) {
            lts_sjson_kv_t *kv = CONTAINER_OF(node, lts_sjson_kv_t, _obj_node);

            // key
            output->data[(*offset)++] = '"';
            (void)memmove(&output->data[(*offset)], node->key.data, node->key.len);
            (*offset) += node->key.len;
            output->data[(*offset)++] = '"';

            output->data[(*offset)++] = ':';

            // val
            output->data[(*offset)++] = '"';
            (void)memmove(&output->data[(*offset)], kv->val.data, kv->val.len);
            (*offset) += kv->val.len;
            output->data[(*offset)++] = '"';
        } else if (LIST_VALUE == node->node_type) {
            list_t *it;
            lts_sjson_list_t *lv = CONTAINER_OF(
                node, lts_sjson_list_t, _obj_node
            );

            // key
            output->data[(*offset)++] = '"';
            (void)memmove(&output->data[(*offset)], node->key.data, node->key.len);
            (*offset) += node->key.len;
            output->data[(*offset)++] = '"';

            output->data[(*offset)++] = ':';

            // val
            output->data[(*offset)++] = '[';
            it = list_first(&lv->val);
            while (it) {
                list_t *next;
                lts_sjson_li_node_t *ln = CONTAINER_OF(
                    it, lts_sjson_li_node_t, node
                );

                output->data[(*offset)++] = '"';
                (void)memmove(&output->data[(*offset)], ln->val.data, ln->val.len);
                (*offset) += ln->val.len;
                output->data[(*offset)++] = '"';

                next = list_next(it);
                if (next) {
                    output->data[(*offset)++] = ',';
                }
                it = next;
            }
            output->data[(*offset)++] = ']';
        } else if (OBJ_VALUE == node->node_type) {
            lts_sjson_t *ov = CONTAINER_OF(node, lts_sjson_t, _obj_node);

            // key
            output->data[(*offset)++] = '"';
            (void)memmove(&output->data[(*offset)], node->key.data, node->key.len);
            (*offset) += node->key.len;
            output->data[(*offset)++] = '"';

            output->data[(*offset)++] = ':';

            __lts_sjson_encode(ov, output, offset);
        } else {
            abort();
        }

        next = rb_next(p);
        if (next) {
            output->data[(*offset)++] = ',';
        }
        p = next;
    }
    output->data[(*offset)++] = '}';
    output->len = (*offset); // 回写长度

    return;
}


int lts_sjson_encode(lts_sjson_t *sjson, lts_str_t *output)
{
    uint8_t *data;
    ssize_t data_sz, offset;
    lts_pool_t *pool = sjson->pool;

    data_sz = lts_sjson_encode_size(sjson);

    // 初始化缓冲区
    data = (uint8_t *)lts_palloc(pool, data_sz + 1);
    if (NULL == data) {
        errno = LTS_E_NOMEM;
        return -1;
    }

    (void)memset(data, 0, data_sz);
    output->data = data;
    output->len = 0;

    offset = 0;
    __lts_sjson_encode(sjson, output, &offset);
    ASSERT(output->len == data_sz);

    return 0;
}


int lts_sjson_decode(lts_str_t *src, lts_sjson_t *output)
{
    static uint8_t invisible[] = {'\t', '\n', '\r', '\x20'};

    DEFINE_LSTACK(obj_stack);
    int in_bracket = FALSE;
    int current_stat = SJSON_EXP_START;
    lts_str_t current_key = lts_null_string;
    lts_sjson_kv_t *json_kv = NULL;
    lts_sjson_li_node_t *li_node = NULL;
    lts_sjson_list_t *json_list = NULL;
    lts_sjson_t *json_obj = NULL; // 当前所在对象
    lts_pool_t *pool = output->pool;

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

                lts_str_share(&json_kv->_obj_node.key, &current_key);
                json_kv->val.data = &src->data[i + 1];
                json_kv->val.len = 0;
                json_kv->_obj_node.node_type = STRING_VALUE;
                json_kv->_obj_node.tnode = RB_NODE;
            } else if ('[' == src->data[i]) {
                in_bracket = TRUE;
                current_stat = SJSON_EXP_V_QUOT_START_OR_BRACKET_END; // only

                json_list = (lts_sjson_list_t *)lts_palloc(pool,
                                                           sizeof(*json_list));
                if (NULL == json_list) {
                    errno = LTS_E_NOMEM;
                    return -1;
                }

                lts_str_share(&json_list->_obj_node.key, &current_key);
                list_set_empty(&json_list->val);
                json_list->_obj_node.node_type = LIST_VALUE;
                json_list->_obj_node.tnode = RB_NODE;
            } else if ('{' == src->data[i]) {
                lts_sjson_t *new_obj;

                current_stat = SJSON_EXP_K_QUOT_START_OR_END; // only

                new_obj = (lts_sjson_t *)lts_palloc(pool, sizeof(*json_obj));
                if (NULL == new_obj) {
                    errno = LTS_E_NOMEM;
                    return -1;
                }

                lts_str_share(&new_obj->_obj_node.key, &current_key);
                new_obj->val = RB_ROOT;
                new_obj->_obj_node.node_type = OBJ_VALUE;
                new_obj->_obj_node.tnode = RB_NODE;

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

        case SJSON_EXP_V_QUOT_START_OR_BRACKET_END:
        {
            if (! in_bracket) {
                abort();
            }

            if ('"' == src->data[i]) {
                current_stat = SJSON_EXP_V_QUOT_END;

                li_node = (lts_sjson_li_node_t *)lts_palloc(pool,
                                                            sizeof(*li_node));
                if (NULL == li_node) {
                    errno = LTS_E_NOMEM;
                    return -1;
                }
                li_node->val.data = &src->data[i + 1];
                li_node->val.len = 0;
            } else if (']' == src->data[i]) {
                in_bracket = FALSE;
                current_stat = SJSON_EXP_COMMA_OR_END;

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
                current_stat = SJSON_EXP_V_QUOT_START_OR_BRACKET_END; // only
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
                current_stat = SJSON_EXP_K_QUOT_START_OR_END;
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

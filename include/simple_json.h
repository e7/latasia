/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#ifndef __LATASIA__SIMPLE_JSON_H__
#define __LATASIA__SIMPLE_JSON_H__


#include "list.h"
#include "rbtree.h"
#include "adv_string.h"
#include "mem_pool.h"


// simple json表示键值叶子类型均为字符串
// 值类型
enum {
    STRING_VALUE = 1,
    LIST_VALUE,
    OBJ_VALUE,
};

typedef struct {
    int node_type;
    lts_str_t key;
    lts_rb_node_t rb_node;
} lts_sjson_obj_node_t;

typedef struct {
    lts_rb_root_t val;
    lts_sjson_obj_node_t _obj_node; // 内部所用红黑树结点
    lstack_t _stk_node; // 内部所用栈结点
} lts_sjson_t;
#define lts_empty_json (lts_sjson_t){\
    RB_ROOT, (lts_sjson_obj_node_t){\
        0, lts_null_string, RB_NODE,\
    }\
}

typedef struct {
    list_t node;
    lts_str_t val;
} lts_sjson_li_node_t;

typedef struct {
    list_t *val;
    lts_sjson_obj_node_t _obj_node; // 内部所用红黑树结点
} lts_sjson_list_t;

typedef struct {
    lts_str_t val;
    lts_sjson_obj_node_t _obj_node; // 内部所用红黑树结点
} lts_sjson_kv_t;


extern ssize_t lts_sjson_encode_size(lts_sjson_t *sjson);
extern int lts_sjson_encode(lts_sjson_t *sjson,
                            lts_pool_t *pool,
                            lts_str_t *output);
extern int lts_sjson_decode(lts_str_t *src,
                            lts_pool_t *pool,
                            lts_sjson_t *output);
#endif // __LATASIA__SIMPLE_JSON_H__

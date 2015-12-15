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


// simple json表示键值对均为字符串
typedef struct {
    lstack_t _stk_node; // 内部所用栈节点
    lts_rb_root_t obj; // json对象
} lts_sjson_t;

typedef union {
    lts_str_t str_val;
    list_t *list_val;
    lts_sjson_t obj_val;
} lts_sjon_val_t;

typedef struct {
    list_t l_node;
    lts_str_t val;
} lts_strlist_t;

typedef struct {
    int val_type; // 值类型
    lts_str_t key;
    lts_rb_node_t rb_node;
    lts_sjon_val_t val;
} lts_sjson_key_t;


extern ssize_t lts_sjon_encode_size(lts_sjson_t *sjson);
extern int lts_sjon_encode(lts_sjson_t *sjson,
                           lts_pool_t *pool,
                           lts_str_t *output);
extern int lts_sjon_decode(lts_str_t *src,
                           lts_pool_t *pool,
                           lts_sjson_t *output);
#endif // __LATASIA__SIMPLE_JSON_H__

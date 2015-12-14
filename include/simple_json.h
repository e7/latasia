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
    lts_rb_root_t obj; // json对象
} lts_sjson_t;


extern ssize_t lts_sjon_encode_size(lts_sjson_t *sjson);
extern int lts_sjon_encode(lts_sjson_t *sjson,
                           lts_pool_t *pool,
                           lts_str_t *output);
extern int lts_sjon_decode(lts_str_t *src,
                           lts_pool_t *pool,
                           lts_sjson_t *output);
#endif // __LATASIA__SIMPLE_JSON_H__

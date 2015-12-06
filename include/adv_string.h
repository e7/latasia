/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#ifndef __LATASIA__ADV_STRING_H__
#define __LATASIA__ADV_STRING_H__


#include "common.h"


typedef struct {
    uint8_t *data;
    size_t len;
} lts_str_t;


#define lts_string(str) {\
    (uint8_t *)(str), sizeof(str) - 1,\
}
#define lts_null_string {\
    NULL, 0,\
};

static inline
void lts_str_init(lts_str_t *str, uint8_t *data, size_t len)
{
    str->data = data;
    str->len = len;
}


// 去除字符串前后空格
extern void lts_str_trim(lts_str_t *str);

// 反转字符串
extern void lts_str_reverse(lts_str_t *src);

// 字符过滤
extern size_t lts_str_filter(lts_str_t *src, uint8_t c);

// 字符串比较
extern int lts_str_compare(lts_str_t *a, lts_str_t *b);

// 子串搜索
extern int lts_str_find(lts_str_t *text, lts_str_t *pattern, int offset);

// long转字符串
extern int lts_l2str(lts_str_t *str, long x);
#endif // __LATASIA__ADV_STRING_H__

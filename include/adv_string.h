/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#ifndef __LATASIA__ADV_STRING_H__
#define __LATASIA__ADV_STRING_H__


#include "common.h"


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// start of charmap {{
typedef union {
    uint8_t bytes[32];
    uint32_t ints[8];
} charmap_t;

// 统计整数中比特位为1的个数
static inline
int count_bit_1(uint32_t x)
{
    x = (x & 0x55555555U) + ((x >> 1) & 0x55555555U);
    x = (x & 0x33333333U) + ((x >> 2) & 0x33333333U);
    x = (x & 0x0F0F0F0FU) + ((x >> 4) & 0x0F0F0F0FU);
    x = (x & 0x00FF00FFU) + ((x >> 8) & 0x00FF00FFU);
    x = (x & 0x0000FFFFU) + ((x >> 16) & 0x0000FFFFU);

    return x;
}

static inline
void charmap_clean(charmap_t *cm)
{
    (void)memset(cm->bytes, 0, sizeof(cm->bytes));
}

static inline
void charmap_set(charmap_t *cm, uint8_t c)
{
    cm->bytes[c / 8] |= (1U << (c % 8));
}

static inline
int charmap_isset(charmap_t *cm, uint8_t c)
{
    return (cm->bytes[c / 8] & (1U << (c % 8)));
}

static inline
int charmap_count(charmap_t *cm)
{
    int count = 0;

    for (int i = 0; i < ARRAY_COUNT(cm->ints); ++i) {
        count += count_bit_1(cm->ints[i]);
    }

    return count;
}
// }} end of charmap


typedef struct {
    uint8_t *data;
    size_t len;
} lts_str_t;


// 用数组初始化
#define lts_string(str)         {(uint8_t *)(str), sizeof(str) - 1,}
#define lts_null_string         {NULL, 0,}

static inline
void lts_str_init(lts_str_t *str, uint8_t *data, size_t len)
{
    str->data = data;
    str->len = len;
}

static inline
void lts_str_copy(lts_str_t *dst, lts_str_t *src)
{
    dst->data = src->data;
    dst->len = src->len;
}


// 去除字符串前后空格
extern void lts_str_trim(lts_str_t *str);

// 反转字符串
extern void lts_str_reverse(lts_str_t *src);

// 移除子串
extern void lts_str_hollow(lts_str_t *src, size_t start, size_t len);

// 字符过滤
extern ssize_t lts_str_filter(lts_str_t *src, uint8_t c);
extern ssize_t lts_str_filter_multi(lts_str_t *src, uint8_t *c, ssize_t len);

// 字符串比较
extern int lts_str_compare(lts_str_t *a, lts_str_t *b);

// 子串搜索
extern int lts_str_find(lts_str_t *text, lts_str_t *pattern, int offset);

// long转字符串
extern int lts_l2str(lts_str_t *str, long x);

// print
extern void lts_str_println(FILE *stream, lts_str_t *s);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // __LATASIA__ADV_STRING_H__

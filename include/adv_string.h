/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#ifndef __EVIEO__ADV_STRING_H__
#define __EVIEO__ADV_STRING_H__


#include "common.h"


typedef struct {
    uint8_t *data;
    size_t len;
} lts_str_t;


#define lts_string(str) {\
    (uint8_t *)(str), sizeof(str) - 1,\
}
#define lts_empty_string {\
    "", 0,\
};
#define lts_null_string {\
    NULL, 0,\
};

static inline
void lts_str_init(lts_str_t *str, uint8_t *data, size_t len)
{
    str->data = data;
    str->len = len;
}


extern void lts_str_trim(lts_str_t *str);
extern void lts_str_reverse(lts_str_t *src);
extern size_t lts_str_filter(lts_str_t *src, uint8_t c);
extern int lts_str_compare(lts_str_t *a, lts_str_t *b);
extern int lts_str_find(lts_str_t *text, lts_str_t *pattern);
extern int lts_l2str(lts_str_t *str, long x);
#endif // __EVIEO__ADV_STRING_H__

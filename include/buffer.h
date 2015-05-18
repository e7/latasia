/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#ifndef __LATASIA__BUFFER_H__
#define __LATASIA__BUFFER_H__


#include "mem_pool.h"


typedef struct lts_buffer_s lts_buffer_t;


struct lts_buffer_s {
    lts_pool_t *pool;
    int expandable; // 是否可扩展
    uint8_t *start; // 缓冲起始地址
    uint8_t *seek; // 访问指针
    uint8_t *last; // 空闲区起始地址
    uint8_t *end; // 缓冲结束地址
};


static inline int lts_buffer_empty(lts_buffer_t *buffer)
{
    return buffer->start == buffer->last;
}

static inline int lts_buffer_full(lts_buffer_t *buffer)
{
    return buffer->last == buffer->end;
}

static inline int lts_buffer_has_pending(lts_buffer_t *buffer)
{
    return buffer->seek < buffer->last;
}

static inline void lts_buffer_clear(lts_buffer_t *buffer)
{
    buffer->seek = buffer->start;
    buffer->last = buffer->start;
}

extern lts_buffer_t *lts_create_buffer(lts_pool_t *pool, size_t size, int exp);
extern int lts_buffer_append(lts_buffer_t *buffer, uint8_t *data, size_t n);
#endif // __LATASIA__BUFFER_H__

/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#ifndef __LATASIA__BUFFER_H__
#define __LATASIA__BUFFER_H__


#include "mem_pool.h"


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct lts_buffer_s lts_buffer_t;


struct lts_buffer_s {
    lts_pool_t *pool;
    ssize_t limit; // 容量扩展上限，0表示不设限
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

static inline int lts_buffer_space(lts_buffer_t *buffer)
{
    return buffer->end - buffer->last;
}

static inline int lts_buffer_pending(lts_buffer_t *buffer)
{
    return buffer->last - buffer->seek;
}

static inline void lts_buffer_clear(lts_buffer_t *buffer)
{
    buffer->seek = buffer->start;
    buffer->last = buffer->start;
}

extern lts_buffer_t *lts_create_buffer(lts_pool_t *pool,
                                       ssize_t size, ssize_t limit);

// 追加数据
extern int lts_buffer_append(lts_buffer_t *buffer, uint8_t *data, ssize_t n);

// 丢弃seek之前的数据
extern void lts_buffer_drop_accessed(lts_buffer_t *buffer);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // __LATASIA__BUFFER_H__

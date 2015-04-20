/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#ifndef __LATASIA__BUFFER_H__
#define __LATASIA__BUFFER_H__


#include "mem_pool.h"


typedef struct lts_buffer_s lts_buffer_t;


struct lts_buffer_s {
    uint8_t *start; // 缓冲起始地址
    uint8_t *seek; // 访问指针
    uint8_t *last; // 空闲区起始地址
    uint8_t *end; // 缓冲结束地址
};


extern lts_buffer_t *lts_create_buffer(lts_pool_t *pool, size_t size);
#endif // __LATASIA__BUFFER_H__

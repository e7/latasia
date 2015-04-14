/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#include "buffer.h"


lts_buffer_t *lts_create_buffer(lts_pool_t *pool, size_t size)
{
    lts_buffer_t *b;

    b = (lts_buffer_t *)lts_palloc(pool, sizeof(lts_buffer_t));
    if (NULL == b) {
        return NULL;
    }

    b->start = (uint8_t *)lts_palloc(pool, size);
    if (NULL == b->start) {
        return NULL;
    }

    b->seek = b->start;
    b->last = b->start;
    b->end = b->start + size;

    return b;
}

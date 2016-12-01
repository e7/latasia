/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#include "buffer.h"

#define __THIS_FILE__       "src/buffer.c"


lts_buffer_t *lts_create_buffer(lts_pool_t *pool,
                                ssize_t size, ssize_t limit)
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

    b->pool = pool;
    b->limit = limit;
    b->seek = b->start;
    b->last = b->start;
    b->end = b->start + size;

    return b;
}


int lts_buffer_append(lts_buffer_t *buffer, uint8_t const *data, ssize_t n)
{
    if ((buffer->end - buffer->last) < n) { // 可用空间不足
        uint8_t *tmp;
        ssize_t ctx_size, curr_size;

        // 指数增长
        curr_size = 2 * MAX(buffer->end - buffer->start, n);
        if (buffer->limit > 0) {
            if (curr_size > buffer->limit) {
                abort();
            }
            if (buffer->limit - curr_size < n) { // 天花板
                return -1;
            }

            if (curr_size > buffer->limit) {
                curr_size = buffer->limit;
            }
        }

        tmp = (uint8_t *)lts_palloc(buffer->pool, curr_size);
        if (NULL == tmp) {
            return -1;
        }

        // append
        ctx_size = (ssize_t)(buffer->last - buffer->start);
        (void)memcpy(tmp, buffer->start, ctx_size);

        buffer->start = tmp;
        buffer->seek = tmp + (ssize_t)(buffer->seek - buffer->start);
        buffer->last = tmp + ctx_size;
        buffer->end = tmp + curr_size;
    }

    (void)memcpy(buffer->last, data, n);
    buffer->last += n;

    return 0;
}


void lts_buffer_drop_accessed(lts_buffer_t *buffer)
{
    ssize_t drop_len = buffer->seek - buffer->start;

    (void)memmove(buffer->start, buffer->seek, drop_len);
    buffer->seek = buffer->start;
    buffer->last = (uint8_t *)(
        (uintptr_t)buffer->last - (uintptr_t)drop_len
    );

    return;
}

/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#include "buffer.h"


lts_buffer_t *lts_create_buffer(lts_pool_t *pool,
                                size_t size, size_t limit)
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


int lts_buffer_append(lts_buffer_t *buffer, uint8_t *data, size_t n)
{
    if ((buffer->end - buffer->last) < n) { // 可用空间不足
        uint8_t *tmp;
        size_t curr_size, ctx_size;

        curr_size = buffer->end - buffer->start;
        if (curr_size > buffer->limit) {
            abort();
        }
        if (buffer->limit - curr_size < n) { // 天花板
            return -1;
        }

        // 指数增长
        curr_size = 2 * MAX(curr_size, n);
        if (curr_size > buffer->limit) {
            curr_size = buffer->limit;
        }
        tmp = (uint8_t *)lts_palloc(buffer->pool, curr_size);
        if (NULL == tmp) {
            return -1;
        }

        // append
        ctx_size = (size_t)(buffer->last - buffer->start);
        (void)memcpy(tmp, buffer->start, ctx_size);

        buffer->start = tmp;
        buffer->seek = tmp + (size_t)(buffer->seek - buffer->start);
        buffer->last = tmp + ctx_size;
        buffer->end = tmp + curr_size;
    }

    (void)memcpy(buffer->last, data, n);
    buffer->last += n;

    return 0;
}

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


int lts_buffer_append(lts_buffer_t *buffer, void *data, ssize_t n)
{
    uint8_t *seg = (uint8_t *)data;
    ssize_t space_left = buffer->end - buffer->last;

    if (space_left < n) { // 可用空间不足
        uint8_t *tmp;
        ssize_t new_size = 2 * MAX(buffer->end - buffer->start, n); // 指数增长
        ssize_t ctx_size = buffer->last - buffer->start;

        if (buffer->limit > 0) {
            // 开启上限
            if (new_size > buffer->limit) {
                new_size = buffer->limit;
            }
            if ((new_size - ctx_size) < n) { // 尽力了
                return -1;
            }
        }

        tmp = (uint8_t *)lts_palloc(buffer->pool, new_size);
        if (NULL == tmp) {
            return -1;
        }

        // append
        (void)memcpy(tmp, buffer->start, ctx_size);

        buffer->start = tmp;
        buffer->seek = tmp + (buffer->seek - buffer->start);
        buffer->last = tmp + ctx_size;
        buffer->end = tmp + new_size;
    }

    (void)memcpy(buffer->last, seg, n);
    buffer->last += n;

    return 0;
}


void lts_buffer_drop_accessed(lts_buffer_t *buffer)
{
    ssize_t drop_len = buffer->seek - buffer->start;

    if (0 == drop_len) {
        return;
    }

    (void)memmove(buffer->start, buffer->seek, drop_len);
    buffer->seek = buffer->start;
    buffer->last = (uint8_t *)(
        (uintptr_t)buffer->last - (uintptr_t)drop_len
    );

    return;
}

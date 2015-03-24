#include "common.h"
#include "mem_pool.h"


#define MIN_POOL_SIZE       4096


struct lts_pool_data_s {
    uint8_t *last;
    uint8_t *end;
    size_t failed; // 失败次数
    lts_pool_data_t *next; // 内存池链表
};

struct lts_pool_s {
    size_t max_size; // 每个内存块能容纳的数据大小
    lts_pool_data_t *current; // 当前内存块
    lts_pool_data_t data;
};


static void *lts_memalign(size_t alignment, size_t size)
{
    void *p;
    int tmp_err;

    tmp_err = posix_memalign(&p, alignment, size);
    if (tmp_err) {
        errno = ENOMEM;
        // log
    }

    return p;
}


static void *lts_palloc_block(lts_pool_t *pool, size_t size)
{
    void *m;
    uint8_t *p;
    size_t psize;
    lts_pool_data_t *new_block, *iter;

    psize = (size_t)LTS_ALIGN(sizeof(lts_pool_data_t), LTS_ALIGNMENT);
    psize += pool->max_size;
    p = (uint8_t *)lts_memalign(LTS_ALIGNMENT, psize);
    if (NULL == p) {
        // log
        return NULL;
    }

    // 初始化新块
    new_block = (lts_pool_data_t *)p;
    new_block->last = p + sizeof(lts_pool_t);
    new_block->last = (uint8_t *)LTS_ALIGN(new_block->last, LTS_ALIGNMENT);
    new_block->end = p + psize;
    new_block->failed = 0;
    new_block->next = NULL;

    // 分配内存
    m = new_block->last;
    new_block->last += size;
    new_block->last = (uint8_t *)LTS_ALIGN(new_block->last, LTS_ALIGNMENT);

    // 挂载内存块
    for (iter = pool->current; NULL != iter->next; iter = iter->next) {
        ++iter->failed;

        if (iter->failed > 4) {
            pool->current = iter->next;
        }
    }
    iter->next = new_block;
    if (NULL == pool->current) {
        pool->current = new_block;
    }

    return m;
}


lts_pool_t *lts_create_pool(size_t size)
{
    uint8_t *p;
    lts_pool_t *pool;

    if (size < MIN_POOL_SIZE) {
        size = MIN_POOL_SIZE;
    }

    if (size < sizeof(lts_pool_t)) {
        return NULL;
    }

    p = (uint8_t *)lts_memalign(LTS_ALIGNMENT, size);
    if (NULL == p) {
        // log
        return NULL;
    }

    // 初始化
    pool = (lts_pool_t *)p;
    pool->current = &pool->data;
    pool->data.last = p + sizeof(lts_pool_t);
    pool->data.last = (uint8_t *)LTS_ALIGN(pool->data.last, LTS_ALIGNMENT);
    pool->data.end = p + size;
    pool->max_size = pool->data.end - pool->data.last;
    pool->data.failed = 0;
    pool->data.next = NULL;

    return pool;
}


void *lts_palloc(lts_pool_t *pool, size_t size)
{
    uint8_t *m;
    lts_pool_data_t *p;

    if (size > pool->max_size) {
        return NULL;
    } else {
        p = pool->current;
        while (p) {
            if ((size_t)(p->end - p->last) < size) {
                p = p->next;
                continue;
            }

            m = p->last;
            p->last += size;
            pool->data.last = (uint8_t *)(
                LTS_ALIGN(pool->data.last, LTS_ALIGNMENT)
            );

            return m;
        }

        return lts_palloc_block(pool, size);
    }
}


void lts_destroy_pool(lts_pool_t* pool)
{
    lts_pool_data_t *iter;
    lts_pool_data_t *tmp_next;

    iter = pool->data.next;
    if (NULL != iter) {
        while (NULL != iter->next) {
            tmp_next = iter->next;
            free(iter);
            iter = tmp_next;
        }
    }
    free(pool);

    return;
}

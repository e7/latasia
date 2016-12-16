#include "extra_errno.h"
#include "obj_pool.h"

#define __THIS_FILE__       "src/obj_pool.c"


void lts_init_opool(
    lts_obj_pool_t *pool, void *cache, ssize_t len, ssize_t size, int offset
)
{
    uint8_t *obj = (uint8_t *)cache;

    pool->offset = offset;
    dlist_init(&pool->freelist);
    pool->left_boundary = cache;
    pool->right_boundary = obj + len - 1;

    for (int i = 0; i < len; i += size) {
        dlist_add_head(&pool->freelist, (dlist_t *)(&obj[i]+ offset));
    }

    return;
}


void *lts_op_instance(lts_obj_pool_t *pool)
{
    dlist_t *rslt;

    if (dlist_empty(&pool->freelist)) {
        return NULL;
    }

    rslt = dlist_get_head(&pool->freelist);
    dlist_del(rslt);

    return (uint8_t *)rslt - pool->offset;
}


void lts_op_release(lts_obj_pool_t *pool, void *obj)
{
    dlist_t *node = (dlist_t *)((uint8_t *)obj + pool->offset);

    if (((void *)node < pool->left_boundary)
        || ((void *)node > pool->right_boundary)) {
        ASSERT(0);
    }

    dlist_del(node);
    dlist_add_tail(&pool->freelist, node);
}

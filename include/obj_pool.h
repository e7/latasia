#ifndef __LATASIA__OBJ_POOL_H__
#define __LATASIA__OBJ_POOL_H__


#include "list.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct lts_obj_pool_s {
    int offset; // 链表节点偏移
    dlist_t freelist;

    // just for security
    void *left_boundary;
    void *right_boundary;
} lts_obj_pool_t;


extern void lts_init_opool(
    lts_obj_pool_t *pool, void *cache, ssize_t len, ssize_t size, int offset
);
extern void *lts_op_instance(lts_obj_pool_t *pool);
extern void lts_op_release(lts_obj_pool_t *pool, void *obj);

#ifdef __cplusplus
}
#endif

#endif // __LATASIA__OBJ_POOL_H__

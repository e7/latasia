/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 *
 * 基于linux内核红黑树的rbmap
 * */


#ifndef __LATASIA__RBMAP_H__
#define __LATASIA__RBMAP_H__

#include "common.h"
#include "rbtree.h"


typedef struct {
    ssize_t nsize; // 容器对象计数
    intptr_t neg_ofst; // 对象负偏移
    uint32_t (*hash)(void *);
    lts_rb_root_t root;
} lts_rbmap_t;
#define lts_rbmap_entity(t, node, func_hash) (lts_rbmap_t){\
    0, (intptr_t)CONTAINER_OF(NULL, t, node), func_hash, RB_ROOT\
}


static inline
void lts_rbmap_init(lts_rbmap_t *rbmap,
                      intptr_t neg_offset, uint32_t (*hash)(void *))
{
    rbmap->nsize = 0;
    rbmap->neg_ofst = neg_offset;
    rbmap->hash = hash;
    rbmap->root = RB_ROOT;
}


extern int lts_rbmap_add(lts_rbmap_t *rbmap, void *obj);
extern void lts_rbmap_del(lts_rbmap_t *rbmap, void *obj);
extern void *lts_rbmap_get(lts_rbmap_t *rbmap, void *obj);
#endif // __LATASIA__RBMAP_H__

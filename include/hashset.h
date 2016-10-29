/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 *
 * 基于linux内核红黑树的hashset
 * */


#ifndef __LATASIA__HASHSET_H__
#define __LATASIA__HASHSET_H__

#include "common.h"
#include "rbtree.h"


typedef struct {
    intptr_t neg_ofst; // 对象负偏移
    uint32_t (*hash)(void *);
    lts_rb_root_t root;
} lts_hashset_t;
#define lts_hashset_entity(t, node, func_hash) (lts_hashset_t){\
    CONTAINER_OF(NULL, t, node), func_hash, RB_ROOT\
}


static inline
void lts_hashset_init(lts_hashset_t *hashset,
                      intptr_t neg_offset, uint32_t (*hash)(void *))
{
    hashset->neg_ofst = neg_offset;
    hashset->hash = hash;
    hashset->root = RB_ROOT;
}


extern int lts_hashset_add(lts_hashset_t *hashset, void *obj);
extern void lts_hashset_del(lts_hashset_t *hashset, void *obj);
extern void *lts_hashset_get(lts_hashset_t *hashset, void *obj);
#endif // __LATASIA__HASHSET_H__

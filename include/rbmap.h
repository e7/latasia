/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 *
 * 基于linux内核红黑树的map
 * */


#ifndef __LATASIA__RBMAP_H__
#define __LATASIA__RBMAP_H__

#include "common.h"
#include "rbtree.h"


typedef struct {
    uintptr_t key;
    lts_rb_node_t rbnode;
} lts_rbmap_node_t;
static inline
void lts_rbmap_node_init(lts_rbmap_node_t *node, uintptr_t key)
{
    node->key = key;
    node->rbnode = RB_NODE;
    RB_CLEAR_NODE(&node->rbnode);
}

typedef struct {
    ssize_t nsize; // 容器对象计数
    lts_rb_root_t root;
} lts_rbmap_t;
#define lts_rbmap_entity (lts_rbmap_t){0, RB_ROOT}


extern int lts_rbmap_add(lts_rbmap_t *rbmap, lts_rbmap_node_t *node);
extern lts_rbmap_node_t *lts_rbmap_del(lts_rbmap_t *rbmap, uintptr_t key);
extern lts_rbmap_node_t *lts_rbmap_get(lts_rbmap_t *rbmap, uintptr_t key);
#endif // __LATASIA__RBMAP_H__

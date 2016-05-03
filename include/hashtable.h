/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#ifndef __LATASIA__HASHTABLE_H__
#define __LATASIA__HASHTABLE_H__

#include "mem_pool.h"


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct hlist_head_s hlist_head_t;
typedef struct hlist_node_s hlist_node_t;
typedef struct lts_hashtable_s lts_hashtable_t;


struct hlist_head_s {
    hlist_node_t *first;
};

struct hlist_node_s {
    hlist_node_t *next;
    hlist_node_t **pprev;
};

struct lts_hashtable_s {
    ssize_t size;
    hlist_head_t *table;
};


static inline int hlist_unhashed(hlist_node_t *node)
{
    return (! node->pprev);
}

static inline void hlist_add(hlist_head_t *hlist, hlist_node_t *node)
{
    node->next = hlist->first;
    node->pprev = &hlist->first;

    hlist->first = node;
}


extern void lts_init_hashtable(lts_hashtable_t *ht);
extern lts_hashtable_t
*lts_create_hashtable(lts_pool_t *pool, ssize_t nbucket);


// 备选哈希函数
extern uintptr_t time33(char const *str, size_t len);
extern uintptr_t hash_long(uintptr_t val, uintptr_t bits);
#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __LATASIA__HASHTABLE_H__

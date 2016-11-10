/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#include <sys/time.h>

#include "rbt_timer.h"

#define __THIS_FILE__       "src/rbt_timer.c"


int lts_timer_add(lts_timer_t *heap, lts_timer_node_t *node)
{
    return lts_rbmap_add(heap, &node->mapnode);
}


int lts_timer_reset(lts_timer_t *heap,
                    lts_timer_node_t *node,
                    uintptr_t timeout)
{
    lts_rbmap_node_t *rbnd;

    rbnd = lts_rbmap_get(heap, timeout);
    if (NULL == rbnd) {
        node->mapnode.key = timeout;
        lts_rbmap_add(heap, &node->mapnode);
        return 0;
    }

    if (node != CONTAINER_OF(rbnd, lts_timer_node_t, mapnode)) {
        // 已存在另外节点
        return -1;
    }

    lts_rbmap_del(heap, node->mapnode.key);
    node->mapnode.key = timeout;
    lts_rbmap_add(heap, &node->mapnode);

    return 0;
}


void lts_timer_del(lts_timer_t *heap, lts_timer_node_t *node)
{
    lts_rbmap_del(heap, node->mapnode.key);
    return;
}


lts_timer_node_t *lts_timer_min(lts_timer_t *heap)
{
    lts_rbmap_node_t *node = lts_rbmap_min(heap);

    if (node) {
        return CONTAINER_OF(lts_rbmap_min(heap), lts_timer_node_t, mapnode);
    } else {
        return NULL;
    }
}



void lts_update_time(void)
{
    lts_timeval_t current;

    (void)gettimeofday(&current, NULL);
    lts_current_time = current.tv_sec * 10 + current.tv_usec / 1000 / 100;

    return;
}


int64_t lts_current_time; // 当前时间
lts_timer_t lts_timer_heap = {0, RB_ROOT}; // 时间堆

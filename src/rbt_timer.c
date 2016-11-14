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
    // 删除旧结点，已存在其它节点则忽略
    (void)lts_rbmap_safe_del(heap, &node->mapnode);

    node->mapnode.key = timeout;
    return lts_rbmap_add(heap, &node->mapnode);
}


int lts_timer_del(lts_timer_t *heap, lts_timer_node_t *node)
{
    return (NULL != lts_rbmap_safe_del(heap, &node->mapnode));
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
lts_timer_t lts_timer_heap; // 时间堆

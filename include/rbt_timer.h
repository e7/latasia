/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#ifndef __LATASIA__RBT_TIMER_H__
#define __LATASIA__RBT_TIMER_H__


#include "common.h"
#include "rbtree.h"
#include "socket.h"


extern int lts_timer_heap_add(lts_rb_root_t *root, lts_socket_t *s);
extern void lts_timer_heap_del(lts_rb_root_t *root, lts_socket_t *s);
extern lts_socket_t *lts_timer_heap_pop_min(lts_rb_root_t *root);

extern void lts_update_time(void);
extern int64_t lts_current_time; // 当前时间
extern lts_rb_root_t lts_timer_heap; // 时间堆
#endif // __LATASIA__RBT_TIMER_H__

/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#ifndef __LATASIA__LIST_H__
#define __LATASIA__LIST_H__

#include "common.h"

#if __cplusplus
extern "C" {
#endif // __cplusplus

// 单链表
typedef intptr_t list_t;

static inline void list_add(list_t **head, list_t *node)
{
    *node = (list_t)*head;
    *head = node;
}

static inline void list_remove(list_t **node)
{
    list_t *p_del = *node;

    *node = (list_t *)**node;
    *p_del = 0;
}

static inline
void add_node(list_t **pp_list, list_t *p_node)
{
    list_t *p_tmp = NULL;

    p_tmp = *pp_list;
    *p_node = (list_t)p_tmp;
    *pp_list = p_node;

    return;
}

static inline
int rm_node(list_t **pp_list, list_t *p_node)
{
    int rslt;
    list_t **pp_curr;

    rslt = -1;
    pp_curr = pp_list;
    while (*pp_curr) {
        if (p_node == *pp_curr) {
            *pp_curr = (list_t *)*p_node;
            *p_node = 0;
            rslt = 0;

            break;
        }

        pp_curr = (list_t **)*pp_curr;
    }

    return rslt;
}


// 栈
typedef list_t lstack_t;
#define lstack_top(stack)           (*(stack))
#define lstack_is_empty(stack)      (NULL == (*(stack)))
#define lstack_set_empty(stack)     ((*(stack)) = NULL)
#define lstack_push(stack, node)    add_node(stack, node)
#define lstack_pop(stack)           rm_node(stack, *stack)


// 双链表
typedef struct s_dlist_t dlist_t;
struct s_dlist_t {
    dlist_t *mp_next;
    dlist_t *mp_prev;
};
#define DLIST_NODE(node)            {&(node), &(node)}
#define DECLARE_DLIST(name)         extern dlist_t name
#define DEFINE_DLIST(name)          dlist_t name = (dlist_t)DLIST_NODE(name)

static inline
void dlist_init(dlist_t *p_list)
{
    p_list->mp_next = p_list;
    p_list->mp_prev = p_list;
}

static inline
void dlist_add_orig(dlist_t *p_new_node,
                    dlist_t *p_prev_node,
                    dlist_t *p_next_node)
{
    p_next_node->mp_prev = p_new_node;
    p_new_node->mp_next = p_next_node;
    p_new_node->mp_prev = p_prev_node;
    p_prev_node->mp_next = p_new_node;
}
#define dlist_add_head(p_list, p_node)      dlist_add_orig((p_node), \
                                                           (p_list), \
                                                           (p_list)->mp_next)
#define dlist_add_tail(p_list, p_node)      dlist_add_orig((p_node), \
                                                           (p_list)->mp_prev, \
                                                           (p_list))

static inline
int dlist_empty(dlist_t *p_list)
{
    return ((p_list->mp_prev == p_list->mp_next)
                && (p_list == p_list->mp_prev));
}

static inline
dlist_t *dlist_get_head(dlist_t *p_list)
{
    dlist_t *p_rslt;

    if (dlist_empty(p_list)) {
        p_rslt = NULL;
    } else {
        p_rslt = p_list->mp_next;
    }

    return p_rslt;
}

static inline
dlist_t *dlist_get_tail(dlist_t *p_list)
{
    dlist_t *p_rslt;

    if (dlist_empty(p_list)) {
        p_rslt = NULL;
    } else {
        p_rslt = p_list->mp_prev;
    }

    return p_rslt;
}

static inline
void dlist_merge(dlist_t *p_head, dlist_t *p_body)
{
    if (dlist_empty(p_body)) {
        return;
    }

    p_body->mp_next->mp_prev = p_head->mp_prev;
    p_body->mp_prev->mp_next = p_head;
    p_head->mp_prev->mp_next = p_body->mp_next;
    p_head->mp_prev = p_body->mp_prev;
    dlist_init(p_body);

    return;
}

static inline
void dlist_del_orig(dlist_t *p_prev_node, dlist_t *p_next_node)
{
    p_next_node->mp_prev = p_prev_node;
    p_prev_node->mp_next = p_next_node;
}
#define dlist_del(p_node)       dlist_del_orig((p_node)->mp_prev, \
                                               (p_node)->mp_next)

#define dlist_for_each_f(p_pos_node, p_list)    \
            for(dlist_t *(p_pos_node) = (p_list)->mp_next; \
                (p_pos_node) != (p_list); \
                (p_pos_node) = (p_pos_node)->mp_next)
#define dlist_for_each_p(p_pos_node, p_list)    \
            for(dlist_t *(p_pos_node) = (p_list)->mp_prev; \
                (p_pos_node) != (p_list); \
                (p_pos_node) = (p_pos_node)->mp_prev)
#define dlist_for_each_f_safe(p_pos_node, p_cur_next, p_list)   \
            for (dlist_t *(p_pos_node) = (p_list)->mp_next, \
                     *(p_cur_next) = (p_pos_node)->mp_next; \
                 (p_pos_node) != (p_list); \
                 (p_pos_node) = (p_cur_next), \
                     (p_cur_next) = (p_pos_node)->mp_next)
#define dlist_for_each_p_safe(p_pos_node, p_cur_prev, p_list)   \
            for (dlist_t *(p_pos_node) = (p_list)->mp_prev, \
                     *(p_cur_prev) = (p_pos_node)->mp_prev; \
                 (p_pos_node) != (p_list); \
                 (p_pos_node) = (p_cur_prev), \
                     (p_cur_prev) = (p_pos_node)->mp_prev)

#if __cplusplus
}
#endif // __cplusplus
#endif // __LATASIA__LIST_H__

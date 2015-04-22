/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#include "rbt_timer.h"


/*
static inline struct page * rb_search_page_cache(struct inode * inode,
                         unsigned long offset)
{
    struct rb_node * n = inode->i_rb_page_cache.rb_node;
    struct page * page;

    while (n)
    {
        page = rb_entry(n, struct page, rb_page_cache);

        if (offset < page->offset)
            n = n->rb_left;
        else if (offset > page->offset)
            n = n->rb_right;
        else
            return page;
    }
    return NULL;
}*/
int lts_timer_heap_add(lts_rb_root_t *root, lts_socket_t *s)
{
    return 0;
}

int lts_timer_heap_del(lts_rb_root_t *root, lts_socket_t *s)
{
    return 0;
}

lts_socket_t *lts_timer_heap_pop_min(lts_rb_root_t *root)
{
    lts_socket_t *s;
    lts_rb_node_t *p;

    s = NULL;
    p = root->rb_node;
    while (p) {
        if (NULL == p->rb_left) {
            rb_erase(p, root);
            s = rb_entry(p, lts_socket_t, rbnode);
            break;
        }
        p = p->rb_left;
    }

    return s;
}


lts_timeval_t lts_current_time; // 当前时间
lts_rb_root_t lts_timer_heap = {NULL}; // 时间堆

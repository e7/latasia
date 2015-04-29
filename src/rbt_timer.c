/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#include <sys/time.h>

#include "rbt_timer.h"


static lts_socket_t *__lts_timer_heap_search(lts_rb_root_t *root,
                                             lts_socket_t *sock,
                                             int link)
{
    lts_socket_t *s;
    lts_rb_node_t *parent, **iter;

    parent = NULL;
    iter = &root->rb_node;
    while (*iter) {
        parent = *iter;
        s = rb_entry(parent, lts_socket_t, rbnode);

        if (sock->timeout < s->timeout) {
            iter = &(parent->rb_left);
        } else if (sock->timeout > s->timeout) {
            iter = &(parent->rb_right);
        } else {
            return s;
        }
    }

    if (link) {
        rb_link_node(&sock->rbnode, parent, iter);
        rb_insert_color(&sock->rbnode, root);
    }

    return sock;
}

int lts_timer_heap_add(lts_rb_root_t *root, lts_socket_t *s)
{
    if (s != __lts_timer_heap_search(root, s, TRUE)) {
        return -1;
    }

    return 0;
}

void lts_timer_heap_del(lts_rb_root_t *root, lts_socket_t *s)
{
    rb_erase(&s->rbnode, root);

    return;
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


void lts_update_time(void)
{
    (void)gettimeofday(&lts_current_time, NULL);

    return;
}


lts_timeval_t lts_current_time; // 当前时间
lts_rb_root_t lts_timer_heap; // 时间堆

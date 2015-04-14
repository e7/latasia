/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#ifndef __EVIEO__SOCKET_H__
#define __EVIEO__SOCKET_H__


#include <sys/socket.h>
#include "list.h"
#include "buffer.h"
#include "mem_pool.h"


#define LTS_SOCKADDRLEN         256
#define EVENT_READ              (1 << 1)
#define EVENT_WRITE             (1 << 2)


typedef struct lts_socket_s lts_socket_t;
typedef struct lts_conn_s lts_conn_t;
typedef void (*lts_handle_event_pt)(lts_socket_t *);


struct lts_conn_s {
    lts_pool_t *pool;
    lts_buffer_t *rbuf;
    lts_buffer_t *sbuf;
};


struct lts_socket_s {
    int fd;
    int family;
    struct sockaddr *local_addr;
    socklen_t addr_len;
    uint32_t ev_mask;

    unsigned short_lived: 1;
    unsigned readable: 1;
    unsigned writable: 1;
    unsigned closing: 2;
    unsigned instance: 1;

    lts_conn_t *conn;
    dlist_t dlnode;
    lts_handle_event_pt on_readable;
    lts_handle_event_pt do_read;
    lts_handle_event_pt on_writable;
    lts_handle_event_pt do_write;
    uintptr_t lifetime;
};


extern int lts_accept_disabled;
extern dlist_t lts_sock_list; // socket缓存列表
extern size_t lts_sock_cache_n; // socket缓存余量
extern size_t lts_sock_inuse_n; // socket缓存使用量
extern dlist_t lts_listen_sock_list; // 监听socket列表
extern dlist_t lts_post_list; // post链表，事件延迟处理
#define lts_sock_list_add(s)    dlist_add_tail(&lts_sock_list, &s->dlnode)
#define lts_sock_list_del(s)    dlist_del(&s->dlnode)
#define lts_post_list_add(s)    dlist_add_tail(&lts_post_list, &s->dlnode)
#define lts_post_list_del(s)    dlist_del(&s->dlnode)
#define lts_listen_sock_list_add(s)     \
        dlist_add_tail(&lts_listen_sock_list, &s->dlnode)
#define lts_listen_sock_list_del(s)     dlist_del(&s->dlnode)


static inline
void lts_init_socket(lts_socket_t *s)
{
    s->fd = -1;
    s->ev_mask = 0;
    s->short_lived = 0;
    s->readable = 0;
    s->writable = 0;
    s->closing = 0;
    s->instance = (!s->instance);
    s->conn = NULL;
    dlist_init(&s->dlnode);
    s->on_readable = NULL;
    s->do_read = NULL;
    s->on_writable = NULL;
    s->do_write = NULL;
}


static inline
lts_socket_t *lts_alloc_socket(void)
{
    dlist_t *rslt;
    lts_socket_t *s;

    if (dlist_empty(&lts_sock_list)) {
        assert(0 == lts_sock_cache_n);
        errno = ENOMEM;
        return NULL;
    }

    rslt = lts_sock_list.mp_next;
    dlist_del(rslt);
    --lts_sock_cache_n;
    ++lts_sock_inuse_n;
    s = CONTAINER_OF(rslt, lts_socket_t, dlnode);
    lts_init_socket(s);

    return s;
}


static inline
void lts_free_socket(lts_socket_t *s)
{
    dlist_add_tail(&lts_sock_list, &s->dlnode);
    ++lts_sock_cache_n;
    --lts_sock_inuse_n;

    return;
}
#endif // __EVIEO__SOCKET_H__

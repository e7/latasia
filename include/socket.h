/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#ifndef __LATASIA__SOCKET_H__
#define __LATASIA__SOCKET_H__


#include <sys/socket.h>

#include "extra_errno.h"
#include "list.h"
#include "rbtree.h"
#include "buffer.h"
#include "mem_pool.h"


#define LTS_SOCKADDRLEN         256
#define EVENT_READ              (1 << 1)
#define EVENT_WRITE             (1 << 2)

#define lts_set_nonblock(fd)    \
        fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK)


typedef struct lts_socket_s lts_socket_t;
typedef struct lts_conn_s lts_conn_t;
typedef ssize_t (*lts_handle_event_pt)(lts_socket_t *);


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

    unsigned readable: 1;
    unsigned writable: 1;
    unsigned timeoutable: 1;
    unsigned more: 1;
    unsigned reset: 1; // 重置连接
    unsigned shutdown: 1; // 关闭连接
    unsigned instance: 1;

    lts_conn_t *conn;
    dlist_t dlnode;

    int64_t timeout; // 超时时间
    lts_rb_node_t rb_node; // timer heap

    lts_handle_event_pt do_read;
    lts_handle_event_pt do_write;
    lts_handle_event_pt do_timeout;

    void *app_ctx; // 应用上下文
};


extern int lts_accept_disabled;
extern dlist_t lts_sock_list; // socket缓存列表
extern size_t lts_sock_cache_n; // socket缓存余量
extern size_t lts_sock_inuse_n; // socket缓存使用量
extern dlist_t lts_addr_list; // 地址列表
extern dlist_t lts_listen_list; // 监听socket列表
extern dlist_t lts_conn_list; // 连接列表
extern dlist_t lts_post_list; // post链表，事件延迟处理

#define lts_sock_list_add(s)    dlist_add_tail(&lts_sock_list, &s->dlnode)
#define lts_addr_list_add(s)    do {\
            dlist_del(&s->dlnode);\
            dlist_add_tail(&lts_addr_list, &s->dlnode);\
        } while(0)
#define lts_listen_list_add(s)  do {\
            dlist_del(&s->dlnode);\
            dlist_add_tail(&lts_listen_list, &s->dlnode);\
        } while (0)
#define lts_conn_list_add(s)    do {\
            dlist_del(&s->dlnode);\
            dlist_add_tail(&lts_conn_list, &s->dlnode);\
        } while (0)
#define lts_post_list_add(s)    do {\
            dlist_del(&s->dlnode);\
            dlist_add_tail(&lts_post_list, &s->dlnode);\
        } while (0)


static inline
void lts_init_socket(lts_socket_t *s)
{
    s->fd = -1;
    s->family = 0;
    s->ev_mask = 0;
    s->readable = 0;
    s->writable = 0;
    s->more = 0;
    s->timeoutable = 0;
    s->reset = 0;
    s->shutdown = 0;
    s->instance = (!s->instance);

    s->conn = NULL;
    dlist_init(&s->dlnode);

    s->timeout = 0;
    s->rb_node = RB_NODE;
    RB_CLEAR_NODE(&s->rb_node);

    s->do_read = NULL;
    s->do_write = NULL;
    s->do_timeout = NULL;

    s->app_ctx = NULL;
}


static inline
lts_socket_t *lts_alloc_socket(void)
{
    dlist_t *rslt;
    lts_socket_t *s;

    if (dlist_empty(&lts_sock_list)) {
        assert(0 == lts_sock_cache_n);
        errno = LTS_E_NOMEM;
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
    dlist_del(&s->dlnode);
    lts_sock_list_add(s);
    ++lts_sock_cache_n;
    --lts_sock_inuse_n;

    return;
}

#ifndef HAVE_FUNCTION_ACCEPT4
#define SOCK_NONBLOCK       (1U << 11)
extern int lts_accept4(int sockfd, struct sockaddr *addr,
                       socklen_t *addrlen, int flags);
#else
#define lts_accept4     accept4
#endif
#endif // __LATASIA__SOCKET_H__

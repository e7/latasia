/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#ifndef __LATASIA__SOCKET_H__
#define __LATASIA__SOCKET_H__


#include <sys/socket.h>
#include <netinet/in.h>

#include "extra_errno.h"
#include "list.h"
#include "rbtree.h"
#include "buffer.h"
#include "mem_pool.h"


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define LTS_SOCKADDRLEN         sizeof(struct sockaddr)
#define EVENT_READ              (1 << 1)
#define EVENT_WRITE             (1 << 2)

#define lts_set_nonblock(fd)    \
        fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK)


typedef struct lts_socket_s lts_socket_t;
typedef struct lts_conn_s lts_conn_t;
typedef void (*lts_handle_event_pt)(lts_socket_t *);


struct lts_conn_s {
    lts_pool_t *pool;
    lts_buffer_t *rbuf;
    lts_buffer_t *sbuf;
    void *app_data;
};


struct lts_socket_s {
    int fd;
    int family;
    struct sockaddr *peer_addr; // 对端地址信息，网络字节序
    uint32_t ev_mask;

    unsigned readable: 1;
    unsigned writable: 1;
    unsigned timeoutable: 1;
    unsigned instance: 1;

    int64_t born_time;
    lts_conn_t *conn;
    dlist_t dlnode;

    int64_t timeout; // 超时时间
    lts_rb_node_t timer_heap_node;

    lts_handle_event_pt do_read;
    lts_handle_event_pt do_write;
    lts_handle_event_pt do_timeout;
};


extern int lts_accept_disabled;
extern dlist_t lts_sock_list; // socket缓存列表
extern size_t lts_sock_cache_n; // socket缓存余量
extern size_t lts_sock_inuse_n; // socket缓存使用量
extern lts_socket_t **lts_listen_array; // 监听socket数组

extern dlist_t lts_watch_list; // 观察链表，与post链互为补链
extern dlist_t lts_post_list; // post链表，事件延迟处理

extern int64_t lts_current_time;

#define lts_sock_list_add(s)    dlist_add_tail(&lts_sock_list, &s->dlnode)
#define lts_addr_list_add(s)    do {\
            dlist_del(&s->dlnode);\
            dlist_add_tail(&lts_addr_list, &s->dlnode);\
        } while(0)
#define lts_listen_list_add(s)  do {\
            dlist_del(&s->dlnode);\
            dlist_add_tail(&lts_listen_list, &s->dlnode);\
        } while (0)
#define lts_watch_list_add(s)    do {\
            dlist_del(&s->dlnode);\
            dlist_add_tail(&lts_watch_list, &s->dlnode);\
        } while (0)
#define lts_post_list_add(s)    do {\
            dlist_del(&s->dlnode);\
            dlist_add_tail(&lts_post_list, &s->dlnode);\
        } while (0)


// 非线程安全，数字转ip
extern char *lts_inet_ntoa(struct in_addr in);


static inline
void lts_init_socket(lts_socket_t *s)
{
    s->fd = -1;
    s->family = -1;
    s->ev_mask = 0;
    s->readable = 0;
    s->writable = 0;
    s->timeoutable = 0;
    s->instance = (!s->instance);

    s->born_time = lts_current_time;
    s->conn = NULL;
    dlist_init(&s->dlnode);

    s->timeout = 0;
    s->timer_heap_node = RB_NODE;
    RB_CLEAR_NODE(&s->timer_heap_node);

    s->do_read = NULL;
    s->do_write = NULL;
    s->do_timeout = NULL;
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
    s->fd = -1;
    s->family = -1;
    s->ev_mask = 0;
    s->readable = 0;
    s->writable = 0;
    s->timeoutable = 0;

    dlist_del(&s->dlnode);
    lts_sock_list_add(s);
    ++lts_sock_cache_n;
    --lts_sock_inuse_n;

    return;
}


// 软事件，强行触发其它连接的事件
static inline
void lts_soft_event(lts_socket_t *other, int writable, int timeoutable)
{
    extern lts_rb_root_t lts_timer_heap;
    extern void lts_timer_heap_del(lts_rb_root_t *root, lts_socket_t *s);

    other->writable = writable;
    other->timeoutable = timeoutable;
    if (timeoutable) {
        other->timeout = 0;
        lts_timer_heap_del(&lts_timer_heap, other);
    }
    lts_post_list_add(other);
}


#ifndef HAVE_FUNCTION_ACCEPT4
#define SOCK_NONBLOCK       (1U << 11)
extern int lts_accept4(int sockfd, struct sockaddr *addr,
                       socklen_t *addrlen, int flags);
#else
#define lts_accept4     accept4
#endif

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // __LATASIA__SOCKET_H__

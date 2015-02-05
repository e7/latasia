#include <netdb.h>
#include <sys/epoll.h>

#include "latasia.h"
#include "socket.h"
#include "buffer.h"
#include "conf.h"
#include "logger.h"


void lts_close_conn(int fd, lts_pool_t *c, int reset)
{
    if (reset) {
        struct linger so_linger;

        so_linger.l_onoff = TRUE;
        so_linger.l_linger = 0;
        (void)setsockopt(fd, SOL_SOCKET, SO_LINGER,
                         &so_linger, sizeof(struct linger));
    }

    if (-1 == close(fd)) {
        (void)lts_write_logger(&lts_file_logger, LTS_ERROR, "close failed\n");
    }

    if (NULL != c) {
        lts_destroy_pool(c);
    }

    return;
}


static void handle_input(lts_socket_t *s)
{
    if (s->conn) {
        lts_sock_list_del(s);
    } else {
        lts_listen_sock_list_del(s);
    }
    lts_post_list_add(s);
    s->readable = 1;

    return;
}


static void handle_output(lts_socket_t *s)
{
    if (s->conn) {
        lts_sock_list_del(s);
    } else {
        lts_listen_sock_list_del(s);
    }
    lts_post_list_add(s);
    s->writable = 1;

    return;
}


static void lts_accept(lts_socket_t *s)
{
    int cmnct_fd;
    uint8_t clt[LTS_SOCKADDRLEN];
    socklen_t clt_len;
    lts_socket_t *cs;
    lts_conn_t *c;
    lts_pool_t *cpool;

    while (TRUE) {
        clt_len = sizeof(clt);

        if (0 == lts_sock_cache_n) {
            // 连接耗尽
            break;
        }

        cmnct_fd = accept4(s->fd,
                           (struct sockaddr *)clt, &clt_len, SOCK_NONBLOCK);
        if (-1 == cmnct_fd) {
            s->readable = 0;

            if (ECONNABORTED == errno) {
                continue;
            }

            lts_post_list_del(s);
            lts_listen_sock_list_add(s);
            if ((EAGAIN != errno) && (EWOULDBLOCK != errno)) {
                (void)lts_write_logger(
                    &lts_file_logger, LTS_ERROR,
                    "altset4() failed: %d\n", errno
                );
            }

            break;
        }

        // 新连接初始化
        cpool = lts_create_pool(CONN_POOL_SIZE);
        if (NULL == cpool) {
            lts_close_conn(cmnct_fd, cpool, TRUE);
            continue;
        }

        c = (lts_conn_t *)lts_palloc(cpool, sizeof(lts_conn_t));
        if (NULL == c) {
            lts_close_conn(cmnct_fd, cpool, TRUE);
            continue;
        }
        c->pool = cpool;
        c->rbuf = lts_create_buffer(cpool, CONN_BUFFER_SIZE);
        c->sbuf = lts_create_buffer(cpool, CONN_BUFFER_SIZE);
        if ((NULL == c->rbuf) || (NULL == c->sbuf)) {
            lts_close_conn(cmnct_fd, cpool, TRUE);
            continue;
        }

        cs = lts_alloc_socket();
        cs->fd = cmnct_fd;
        cs->ev_mask = (EPOLLET | EPOLLIN | EPOLLOUT);
        cs->conn = c;
        cs->on_readable = &handle_input;
        cs->do_read = &lts_recv;
        cs->on_writable = &handle_output;
        cs->do_write = &lts_send;

        if (LTS_E_OK != (*lts_event_itfc->event_add)(cs)) {
            lts_close_conn(cmnct_fd, cpool, TRUE);
            lts_free_socket(cs);
            continue;
        }

        lts_sock_list_add(cs);
    }

    return;
}


static int alloc_listen_sockets(lts_pool_t *pool)
{
    int rslt;
    size_t i;
    char const *conf_port;
    lts_socket_t *sock_cache;
    struct addrinfo hint, *records, *iter;

    rslt = LTS_E_OK;

    // 获取本地地址
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = AI_PASSIVE;
    conf_port = (char const *)lts_conf.port.data;
    if (0 != getaddrinfo(NULL, conf_port, &hint, &records)) {
        // log
        return LTS_E_SYS;
    }

    // 统计监听套接字数目
    lts_sock_cache_n = MAX_CONNECTIONS;
    for (iter = records; NULL != iter; iter = iter->ai_next) {
        ++lts_sock_cache_n;
    }

    // 建立socket缓存
    dlist_init(&lts_sock_list);
    dlist_init(&lts_post_list);
    sock_cache = (lts_socket_t *)(
        lts_palloc(pool, lts_sock_cache_n * sizeof(lts_socket_t))
    );
    for (i = 0; i < lts_sock_cache_n; ++i) {
        dlist_add_tail(&lts_sock_list, &sock_cache[i].dlnode);
    }

    // 监听套接字初始化
    dlist_init(&lts_listen_sock_list);
    for (iter = records; NULL != iter; iter = iter->ai_next) {
        struct sockaddr *a;
        lts_socket_t *ls;

        if ((AF_INET != iter->ai_family)
                && ((!ENABLE_IPV6) || (AF_INET6 != iter->ai_family)))
        {
            continue;
        }

        a = (struct sockaddr *)(
            lts_palloc(pool, iter->ai_addrlen)
        );
        ls = lts_alloc_socket();
        if ((NULL == a) || (NULL == ls)) {
            rslt = LTS_E_NO_MEM;
            break;
        }

        (void)memcpy(a, iter->ai_addr, iter->ai_addrlen);
        ls->fd = -1;
        ls->family = iter->ai_family;
        ls->local_addr = a;
        ls->addr_len = iter->ai_addrlen;
        ls->ev_mask = (EPOLLET | EPOLLIN);
        ls->on_readable = &handle_input;
        ls->do_read = &lts_accept;
        lts_listen_sock_list_add(ls);
    }

    freeaddrinfo(records);

    return rslt;
}


static void free_listen_sockets(void)
{
    dlist_for_each_f_safe(pos_node, cur_next, &lts_listen_sock_list) {
        lts_socket_t *ls;

        ls = CONTAINER_OF(pos_node, lts_socket_t, dlnode);
        if (-1 == close(ls->fd)) {
            // log
        }
        dlist_del(&ls->dlnode);
    }
}


static int init_event_core_master(lts_module_t *mod)
{
    int rslt, ipv6_only, reuseaddr;
    lts_pool_t *pool;

    rslt = LTS_E_OK;

    // 创建内存池
    pool = lts_create_pool(MODULE_POOL_SIZE);
    if (NULL == pool) {
        // log
        return LTS_E_NO_MEM;
    }
    mod->pool = pool;

    // 创建accept共享内存锁
    rslt = lts_shm_alloc(&lts_accept_lock);
    if (LTS_E_OK != rslt) {
        return rslt;
    }

    // 创建监听套接字
    rslt = alloc_listen_sockets(pool);
    if (LTS_E_OK != rslt) {
        return rslt;
    }

    // 打开监听套接字
    ipv6_only = 1;
    reuseaddr = 1;
    dlist_for_each_f_safe(pos_node, cur_next, &lts_listen_sock_list) {
        int fd;
        lts_socket_t *ls;

        ls = CONTAINER_OF(pos_node, lts_socket_t, dlnode);
        fd = socket(ls->family, SOCK_STREAM, 0);
        if (-1 == fd) {
            //log
            rslt = LTS_E_SYS;
            break;
        }

        if (ls->family == AF_INET6) { // 仅ipv6
            rslt = setsockopt(fd, IPPROTO_IPV6,
                              IPV6_V6ONLY, &ipv6_only, sizeof(ipv6_only));
            if (-1 == rslt) {
                // log
                if (-1 == close(fd)) {
                    // log
                }
                rslt = LTS_E_SYS;
                break;
            }
        }

        rslt = fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
        if (-1 == rslt) {
            // log
            if (-1 == close(fd)) {
                // log
            }
            rslt = LTS_E_SYS;
            break;
        }

        rslt = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                          (const void *) &reuseaddr, sizeof(int));
        if (-1 == rslt) {
            // log
            if (-1 == close(fd)) {
                // log
            }
            rslt = LTS_E_SYS;
            break;
        }

        rslt = bind(fd, ls->local_addr, ls->addr_len);
        if (-1 == rslt) {
            (void)lts_write_logger(&lts_stderr_logger, LTS_ERROR,
                                   "bind failed: %s\n", strerror(errno));
            if (-1 == close(fd)) {
                // log
            }
            rslt = LTS_E_SYS;
            break;
        }

        rslt = listen(fd, SOMAXCONN);
        if (-1 == rslt) {
            // log
            if (-1 == close(fd)) {
                // log
            }
            rslt = LTS_E_SYS;
            break;
        }

        ls->fd = fd;
    }

    return rslt;
}


static int init_event_core_worker(lts_module_t *mod)
{
    int i;

    // 关闭
    for (i = 0; i < lts_conf.workers; ++i) {
    }

    // 初始化master-worker通信管道
    lts_channel = lts_alloc_socket();
    lts_channel->fd = lts_processes[lts_ps_slot].channel[1];
    lts_channel->ev_mask = (EPOLLET | EPOLLIN);
    lts_channel->on_readable = &handle_input;
    lts_channel->do_read = NULL;
    lts_channel->on_writable = &handle_output;
    lts_channel->do_write = NULL;

    return 0;
}


static void exit_event_core_worker(lts_module_t *mod)
{
    if (-1 == close(lts_channel->fd)) {
        // log
    }
    lts_free_socket(lts_channel);

    return;
}


static void exit_event_core_master(lts_module_t *mod)
{
    // 关闭连接
    dlist_for_each_f_safe(pos_node, cur_next, &lts_sock_list) {
        lts_socket_t *s;

        s = CONTAINER_OF(pos_node, lts_socket_t, dlnode);
        if (-1 == close(s->fd)) {
            // log
        }
        dlist_del(&s->dlnode);
    }

    // 释放accept锁
    lts_shm_free(&lts_accept_lock);

    // 关闭监听套接字
    free_listen_sockets();

    // 释放模块内存池
    if (NULL != mod->pool) {
        lts_destroy_pool(mod->pool);
    }

    return;
}


void lts_recv(lts_socket_t *cs)
{
    ssize_t recv_sz;
    lts_buffer_t *buf;

    buf = cs->conn->rbuf;

    if ((buf->last == buf->end) && (buf->seek > buf->start)) {
        size_t cp_sz = (size_t)((uintptr_t)buf->last - (uintptr_t)buf->seek);

        (void)memmove(buf->start, buf->seek, cp_sz);
        buf->seek = buf->start;
        buf->last = (uint8_t *)((size_t)buf->start + cp_sz);
    }

    while ((uintptr_t)buf->last < (uintptr_t)buf->end) {
        recv_sz = recv(cs->fd, buf->last,
                      (uintptr_t)buf->end - (uintptr_t)buf->last, 0);
        if (-1 == recv_sz) {
            cs->readable = 0;
            if ((EAGAIN == errno) || (EWOULDBLOCK == errno)) {
                lts_post_list_del(cs);
                lts_sock_list_add(cs);
            } else {
                // 异常关闭
                (void)lts_write_logger(
                    &lts_file_logger, LTS_ERROR,
                    "recv failed(%d), reset connection\n", errno
                );
                cs->closing = ((1 << 0) | (1 << 1));
            }
            break;
        }

        if (0 == recv_sz) {
            // 正常关闭连接
            (void)lts_write_logger(
                &lts_file_logger, LTS_ERROR,
                "connection closed by peer\n", errno
            );
            cs->readable = 0;
            cs->closing = (1 << 0);
            break;
        }

        buf->last = (uint8_t *)((uintptr_t)buf->last + (uintptr_t)recv_sz);
    }

    return;
}


void lts_send(lts_socket_t *cs)
{
    ssize_t sent_sz;
    lts_buffer_t *buf;

    buf = cs->conn->sbuf;
    cs->writable = 0; // 不论成败，只发一次

    if ((uintptr_t)buf->seek < (uintptr_t)buf->last) {
        sent_sz = send(cs->fd, buf->seek,
                       (uintptr_t)buf->last - (uintptr_t)buf->seek, 0);

        // sent_sz won't be 0
        if (-1 == sent_sz) {
            if ((EAGAIN == errno) || (EWOULDBLOCK == errno)) {
                lts_post_list_del(cs);
                lts_sock_list_add(cs);
            } else {
                (void)lts_write_logger(
                    &lts_file_logger, LTS_ERROR,
                    "send failed(%d), reset connection\n", errno
                );
                cs->closing = ((1 << 0) | (1 << 1));
            }
        }

        buf->seek = (uint8_t *)((uintptr_t)buf->seek + (uintptr_t)sent_sz);
        if (buf->seek == buf->last) {
            // 数据已发完
            buf->seek = buf->start;
            buf->last = buf->start;
        }
    }

    return;
}


lts_module_t lts_event_core_module = {
    lts_string("lts_event_core_module"),
    LTS_CORE_MODULE,
    NULL,
    NULL,
    NULL,
    // interfaces
    &init_event_core_master,
    &init_event_core_worker,
    &exit_event_core_worker,
    &exit_event_core_master,
};


lts_event_module_itfc_t *lts_event_itfc;

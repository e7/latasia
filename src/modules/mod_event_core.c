#include <netdb.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <netinet/tcp.h>

#include "latasia.h"
#include "socket.h"
#include "buffer.h"
#include "conf.h"
#include "logger.h"
#include "rbt_timer.h"
#include "extra_errno.h"

#define __THIS_FILE__       "src/modules/mod_event_core.c"


void lts_close_conn_orig(int fd, int reset)
{
    if (reset) {
        struct linger so_linger;

        so_linger.l_onoff = TRUE;
        so_linger.l_linger = 0;
        (void)setsockopt(fd, SOL_SOCKET, SO_LINGER,
                         &so_linger, sizeof(struct linger));
    }

    if (-1 == close(fd)) {
        (void)lts_write_logger(&lts_file_logger, LTS_LOG_ERROR,
                               "%s:close() failed: %s\n",
                               STR_LOCATION, lts_errno_desc[errno]);
    }

    return;
}


void lts_close_conn(lts_socket_t *cs, int reset)
{
    // 移除定时器
    lts_timer_heap_del(&lts_timer_heap, cs);

    // 清事件标识移除事件监视
    cs->readable = 0;
    cs->writable = 0;
    cs->timeoutable = 0;
    (*lts_event_itfc->event_del)(cs);

    // 关闭连接
    lts_close_conn_orig(cs->fd, reset);
    if (cs->conn->pool) {
        lts_destroy_pool(cs->conn->pool);
        cs->conn = NULL;
    }

    // 回收套接字
    lts_free_socket(cs);

    return;
}


static void lts_accept(lts_socket_t *ls)
{
    int cmnct_fd, nodelay, count;
    uint8_t clt[LTS_SOCKADDRLEN];
    socklen_t clt_len;
    lts_socket_t *cs;
    lts_conn_t *c;
    lts_pool_t *cpool;

    nodelay = 1;
    for (count = 0; count < lts_conf.max_connections; ++count) {
        clt_len = sizeof(clt);

        if (0 == lts_sock_cache_n) {
            // 连接耗尽
            break;
        }

        cmnct_fd = lts_accept4(ls->fd, (struct sockaddr *)clt,
                               &clt_len, SOCK_NONBLOCK);
        if (-1 == cmnct_fd) {
            ls->readable = 0;
            lts_listen_list_add(ls); // post_list -> listen_list

            if (LTS_E_CONNABORTED == errno) {
                continue;
            }

            if ((LTS_E_AGAIN != errno) && (LTS_E_WOULDBLOCK != errno)) {
                (void)lts_write_logger(
                    &lts_file_logger, LTS_LOG_ERROR,
                    "%s:accept4() failed:%s\n",
                    STR_LOCATION, lts_errno_desc[errno]
                );
            }

            break;
        }
        if (-1 == setsockopt(cmnct_fd, IPPROTO_TCP,
                             TCP_NODELAY, &nodelay, sizeof(nodelay))) {
            (void)lts_write_logger(&lts_file_logger, LTS_LOG_ERROR,
                                   "%s:setsockopt() TCP_NODELAY failed:%s\n",
                                   STR_LOCATION, lts_errno_desc[errno]);
        }

        // 新连接初始化
        cpool = lts_create_pool(CONN_POOL_SIZE);
        if (NULL == cpool) {
            lts_close_conn_orig(cmnct_fd, TRUE);
            continue;
        }

        c = (lts_conn_t *)lts_palloc(cpool, sizeof(lts_conn_t));
        if (NULL == c) {
            lts_close_conn_orig(cmnct_fd, TRUE);
            lts_destroy_pool(cpool);
            continue;
        }
        c->pool = cpool; // 新连接的内存池
        c->rbuf = lts_create_buffer(cpool, CONN_BUFFER_SIZE, CONN_BUFFER_SIZE);
        c->sbuf = lts_create_buffer(cpool, CONN_BUFFER_SIZE, CONN_BUFFER_SIZE);
        c->app_data = NULL;
        assert(c->rbuf != c->sbuf);
        if ((NULL == c->rbuf) || (NULL == c->sbuf)) {
            lts_close_conn_orig(cmnct_fd, TRUE);
            lts_destroy_pool(cpool);
            continue;
        }

        cs = lts_alloc_socket();
        cs->fd = cmnct_fd;
        cs->ev_mask = (EPOLLET | EPOLLIN);
        cs->conn = c;
        cs->do_read = &lts_recv;
        cs->do_write = &lts_send;
        cs->do_timeout = &lts_timeout;
        cs->timeout = lts_current_time + lts_conf.keepalive * 10;

        // 加入事件监视
        (*lts_event_itfc->event_add)(cs);

        while (-1 == lts_timer_heap_add(&lts_timer_heap, cs)) {
            ++cs->timeout;
        }

        lts_conn_list_add(cs); // 纳入活动连接
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

    // 获取本地地址
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = AI_PASSIVE;
    conf_port = (char const *)lts_conf.port.data;
    rslt = getaddrinfo(NULL, conf_port, &hint, &records);
    if (rslt) {
        errno = LTS_E_SYS;
        return -1;
    }

    // 统计监听套接字数目
    lts_sock_cache_n = lts_conf.max_connections;
    for (iter = records; NULL != iter; iter = iter->ai_next) {
        ++lts_sock_cache_n;
    }

    // 建立socket缓存
    dlist_init(&lts_sock_list);
    sock_cache = (lts_socket_t *)(
        lts_palloc(pool, lts_sock_cache_n * sizeof(lts_socket_t))
    );
    for (i = 0; i < lts_sock_cache_n; ++i) {
        dlist_add_tail(&lts_sock_list, &sock_cache[i].dlnode);
    }

    // 地址列表初始化
    rslt = 0;
    dlist_init(&lts_addr_list);
    for (iter = records; NULL != iter; iter = iter->ai_next) {
        struct sockaddr *a;
        lts_socket_t *ls;

        if ((AF_INET != iter->ai_family)
                && ((!ENABLE_IPV6) || (AF_INET6 != iter->ai_family))) {
            continue;
        }

        a = (struct sockaddr *)(
            lts_palloc(pool, iter->ai_addrlen)
        );
        ls = lts_alloc_socket();
        if ((NULL == a) || (NULL == ls)) {
            rslt = -1;
            break;
        }

        (void)memcpy(a, iter->ai_addr, iter->ai_addrlen);
        lts_init_socket(ls);
        ls->family = iter->ai_family;
        ls->local_addr = a;
        ls->addr_len = iter->ai_addrlen;
        ls->ev_mask = (EPOLLET | EPOLLIN);
        ls->do_read = &lts_accept;
        lts_addr_list_add(ls); // 添加到地址列表
    }

    freeaddrinfo(records);

    return rslt;
}


static void free_listen_sockets(void)
{
    dlist_for_each_f_safe(pos_node, cur_next, &lts_listen_list) {
        lts_socket_t *ls;

        ls = CONTAINER_OF(pos_node, lts_socket_t, dlnode);
        if (-1 == close(ls->fd)) {
            (void)lts_write_logger(&lts_file_logger, LTS_LOG_ERROR,
                                   "%s:close() failed:%s\n",
                                   STR_LOCATION, lts_errno_desc[errno]);
        }
        dlist_del(&ls->dlnode);
    }
}


static int init_event_core_master(lts_module_t *mod)
{
    int rslt, ipv6_only, reuseaddr;
    lts_pool_t *pool;

    // 全局初始化
    if (NULL == getcwd((char *)lts_cwd.data, LTS_MAX_PATH_LEN)) {
        (void)lts_write_logger(&lts_file_logger, LTS_LOG_ERROR,
                               "%s:getcwd() failed:%s\n",
                               STR_LOCATION, lts_errno_desc[errno]);
        return -1;
    }
    for (lts_cwd.len = 0; lts_cwd.len < LTS_MAX_PATH_LEN; ++lts_cwd.len) {
        if (! lts_cwd.data[lts_cwd.len]) {
            break;
        }
    }

    // 创建内存池
    pool = lts_create_pool(MODULE_POOL_SIZE);
    if (NULL == pool) {
        return -1;
    }
    mod->pool = pool;

    // 创建accept共享内存锁
    rslt = lts_shm_alloc(&lts_accept_lock);
    if (0 != rslt) {
        return rslt;
    }

    // 创建监听套接字
    rslt = alloc_listen_sockets(pool);
    if (0 != rslt) {
        return rslt;
    }

    // 打开监听套接字
    rslt = 0;
    ipv6_only = 1;
    reuseaddr = 1;
    dlist_init(&lts_listen_list);
    dlist_for_each_f_safe(pos_node, cur_next, &lts_addr_list) {
        int fd;
        lts_socket_t *ls;

        ls = CONTAINER_OF(pos_node, lts_socket_t, dlnode);
        fd = socket(ls->family, SOCK_STREAM, 0);
        if (-1 == fd) {
            // log
            return -1;
        }

        if (ls->family == AF_INET6) { // 仅ipv6
            rslt = setsockopt(fd, IPPROTO_IPV6,
                              IPV6_V6ONLY, &ipv6_only, sizeof(ipv6_only));
            if (-1 == rslt) {
                // log
            }
        }

        rslt = lts_set_nonblock(fd);
        if (-1 == rslt) {
            // log
        }

        rslt = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                          (const void *) &reuseaddr, sizeof(int));
        if (-1 == rslt) {
            // log
        }

        rslt = bind(fd, ls->local_addr, ls->addr_len);
        if (-1 == rslt) {
            (void)lts_write_logger(&lts_file_logger, LTS_LOG_ERROR,
                                   "%s:bind() failed:%s\n",
                                   STR_LOCATION,
                                   lts_errno_desc[errno]);
            (void)close(fd);
            break;
        }

        rslt = listen(fd, SOMAXCONN);
        if (-1 == rslt) {
            (void)close(fd);
            break;
        }

        ls->fd = fd;

        lts_listen_list_add(ls); // 纳入监听列表
    }

    return rslt;
}

static void channel_do_read(lts_socket_t *s)
{
    uint32_t sig = 0;

    if (-1 == recv(s->fd, &sig, sizeof(sig), 0)) {
        (void)lts_write_logger(&lts_file_logger, LTS_LOG_ERROR,
                               "%s:recv() failed:%s\n",
                               STR_LOCATION, lts_errno_desc[errno]);
        return;
    }

    lts_global_sm.channel_signal = sig;

    return;
}
static int init_event_core_worker(lts_module_t *mod)
{
    struct itimerval timer_resolution = { // 晶振间隔0.1s
        {0, 1000 * 100},
        {0, 1000 * 100},
    };

    // 初始化各种列表
    dlist_init(&lts_conn_list);
    dlist_init(&lts_post_list);

    // 工作进程晶振
    if (-1 == setitimer(ITIMER_REAL, &timer_resolution, NULL)) {
        (void)lts_write_logger(&lts_file_logger, LTS_LOG_ERROR,
                               "%s:setitimer() failed:%s\n",
                               STR_LOCATION, lts_errno_desc[errno]);
        return -1;
    }

    // 分配域套接字连接
    lts_channels[lts_ps_slot] = lts_alloc_socket();
    lts_channels[lts_ps_slot]->fd = lts_processes[lts_ps_slot].channel[1];
    lts_channels[lts_ps_slot]->ev_mask = (EPOLLET | EPOLLIN);
    lts_channels[lts_ps_slot]->do_read = &channel_do_read;
    lts_channels[lts_ps_slot]->do_write = NULL;

    return 0;
}


static void exit_event_core_worker(lts_module_t *mod)
{
    // 释放域套接字连接
    lts_free_socket(lts_channels[lts_ps_slot]);

    return;
}


static void exit_event_core_master(lts_module_t *mod)
{
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


// 接收数据到连接缓冲
void lts_recv(lts_socket_t *cs)
{
    ssize_t recv_sz;
    lts_buffer_t *buf;
    lts_app_module_itfc_t *app_itfc = (lts_app_module_itfc_t *)(
        lts_module_app_cur->itfc
    );

    buf = cs->conn->rbuf;

    if (lts_buffer_full(buf)) {
        (void)lts_write_logger(&lts_file_logger, LTS_LOG_WARN,
                               "%s:recv buffer is full\n", STR_LOCATION);
        abort(); // 要求应用模块一定要处理缓冲
    }

    recv_sz = recv(cs->fd, buf->last,
                   (uintptr_t)buf->end - (uintptr_t)buf->last, 0);
    if (-1 == recv_sz) {
        if ((LTS_E_AGAIN == errno) || (LTS_E_WOULDBLOCK == errno)) {
            // 本次数据读完
            cs->readable = 0;

            // 应用模块处理
            (*app_itfc->service)(cs);

            if ((! lts_buffer_empty(cs->conn->sbuf))
                && (! (cs->ev_mask & EPOLLOUT))) {
                // 开启写事件监视
                cs->ev_mask |= EPOLLOUT;
                (*lts_event_itfc->event_mod)(cs);
            }
        } else {
            int lvl;

            // 异常关闭
            lvl = (
                (LTS_E_CONNRESET == errno) ? LTS_LOG_NOTICE : LTS_LOG_ERROR
            );
            (void)lts_write_logger(&lts_file_logger, lvl,
                                   "%s:recv() failed:%s, reset connection\n",
                                   STR_LOCATION, lts_errno_desc[errno]);
            lts_close_conn(cs, TRUE);
        }

        return;
    } else if (0 == recv_sz) {
        // 正常关闭连接
        lts_close_conn(cs, FALSE);

        return;
    } else {
        buf->last += recv_sz;
    }

    return;
}


void lts_send(lts_socket_t *cs)
{
    ssize_t sent_sz;
    lts_buffer_t *buf;
    lts_app_module_itfc_t *app_itfc = (lts_app_module_itfc_t *)(
        lts_module_app_cur->itfc
    );

    buf = cs->conn->sbuf;

    if (lts_buffer_empty(buf)) { // 无数据可发
        cs->writable = 0;
        return;
    }

    if ((uintptr_t)buf->seek >= (uintptr_t)buf->last) {
        abort();
    }

    // 发送数据
    sent_sz = send(cs->fd, buf->seek,
                   (uintptr_t)buf->last - (uintptr_t)buf->seek, 0);

    if (-1 == sent_sz) {
        if ((LTS_E_AGAIN == errno) || (LTS_E_WOULDBLOCK == errno)) {
            return;
        }

        (void)lts_write_logger(&lts_file_logger, LTS_LOG_ERROR,
                               "%s:send(%d) failed:%s, reset connection\n",
                               STR_LOCATION, cs->fd,
                               lts_errno_desc[errno]);
        lts_close_conn(cs, TRUE);

        return;
    }

    buf->seek += sent_sz;

    // 数据已发完
    if (buf->seek == buf->last) {
        lts_buffer_clear(cs->conn->sbuf);

        // 应用模块处理
        (*app_itfc->send_more)(cs);

        // 关闭写事件监视
        if (lts_buffer_empty(cs->conn->sbuf)) {
            cs->writable = 0;
            cs->ev_mask &= (~EPOLLOUT);
            (*lts_event_itfc->event_mod)(cs);
        }
    }

    return;
}


void lts_timeout(lts_socket_t *cs)
{
    cs->timeoutable = 0;

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

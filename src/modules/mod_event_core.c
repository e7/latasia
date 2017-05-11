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


typedef struct {
    int current_conns; // 当前连接数
} event_core_ctx_t;


static event_core_ctx_t s_event_core_ctx = {
    0,
};
static void lts_evt_timeout(lts_timer_node_t *cstn);


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
    lts_app_module_itfc_t *app_itfc = (lts_app_module_itfc_t *)(
        lts_module_app_cur->itfc
    );

    // 通知app模块
    if (app_itfc->on_closing) {
        (*app_itfc->on_closing)(cs);
    }

    // 移除事件监视
    cs->readable = 0;
    cs->writable = 0;
    (*lts_event_itfc->event_del)(cs);

    // 移除定时器
    if (lts_main_conf.keepalive > 0) {
        ASSERT(lts_timer_del(&lts_timer_heap, &cs->timer_node));
    }

    // 回收内存
    if (cs->conn->pool) {
        lts_destroy_pool(cs->conn->pool);
        cs->conn = NULL;
    }

    // 回收套接字
    lts_close_conn_orig(cs->fd, reset);
    lts_free_socket(cs);

    // 调整当前连接数
    if (s_event_core_ctx.current_conns > 0) {
        --s_event_core_ctx.current_conns;
    }

    return;
}


static void lts_accept(lts_socket_t *ls)
{
    int cmnct_fd, nodelay;
    uint8_t clt[LTS_SOCKADDRLEN] = {0};
    socklen_t clt_len = LTS_SOCKADDRLEN;
    lts_socket_t *cs;
    lts_conn_t *c;
    lts_pool_t *cpool;
    lts_app_module_itfc_t *app_itfc;

    if (s_event_core_ctx.current_conns >= lts_main_conf.max_connections) {
        // 达到最大连接数
        ls->readable = 0;
        return;
    }

    if (0 == lts_sock_cache_n) {
        abort();
    }

    nodelay = 1;
    cmnct_fd = lts_accept4(ls->fd, (struct sockaddr *)clt,
                           &clt_len, SOCK_NONBLOCK);
    if (-1 == cmnct_fd) {
        if (LTS_E_CONNABORTED == errno) {
            return; // hold readable and try again
        }

        if ((LTS_E_AGAIN != errno) && (LTS_E_WOULDBLOCK != errno)) {
            (void)lts_write_logger(
                &lts_file_logger, LTS_LOG_ERROR,
                "%s:accept4() failed:%s\n",
                STR_LOCATION, lts_errno_desc[errno]
            );
        }
        ls->readable = 0;

        return;
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
        return;
    }

    c = (lts_conn_t *)lts_palloc(cpool, sizeof(lts_conn_t));
    if (NULL == c) {
        lts_close_conn_orig(cmnct_fd, TRUE);
        lts_destroy_pool(cpool);
        return;
    }
    c->pool = cpool; // 新连接的内存池
    c->rbuf = lts_create_buffer(cpool, RBUF_SIZE, RBUF_SIZE);
    c->sbuf = lts_create_buffer(cpool, SBUF_SIZE, SBUF_SIZE);

    cs = lts_alloc_socket();
    cs->fd = cmnct_fd;
    cs->peer_addr = (struct sockaddr *)lts_palloc(cpool, LTS_SOCKADDRLEN);
    memmove(cs->peer_addr, clt, LTS_SOCKADDRLEN);
    cs->ev_mask = (EPOLLET | EPOLLIN | EPOLLOUT);
    cs->conn = c;
    cs->do_read = &lts_evt_recv;
    cs->do_write = &lts_evt_send;
    cs->do_error = &lts_evt_error;
    cs->app_ctx = NULL;

    // 加入事件监视
    (*lts_event_itfc->event_add)(cs);

    // 加入定时器
    lts_timer_node_init(&cs->timer_node, 0, &lts_evt_timeout);
    if (lts_main_conf.keepalive > 0) {
        int64_t reset = lts_current_time + lts_main_conf.keepalive * 10;

        while (-1 == lts_timer_reset(&lts_timer_heap, &cs->timer_node, reset)) {
            ++reset;
        }
    }

    lts_watch_list_add(cs); // 纳入观察列表

    ++s_event_core_ctx.current_conns;

    // 通知应用模块
    app_itfc = (lts_app_module_itfc_t *)lts_module_app_cur->itfc;
    if (NULL == app_itfc) {
        abort();
    }
    if (app_itfc->on_connected) {
        (*app_itfc->on_connected)(cs);
    }

    return;
}


static int alloc_listen_sockets(lts_pool_t *pool)
{
    int i;
    int nrecords;
    char const *conf_port;
    lts_socket_t *sock_cache;
    struct addrinfo hint, *records, *iter;

    // 获取本地地址
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = AI_PASSIVE;
    conf_port = (char const *)lts_main_conf.port.data;
    errno = getaddrinfo(NULL, conf_port, &hint, &records);
    if (errno) {
        return -1;
    }

    // 统计地址数
    nrecords = 0;
    for (iter = records; iter; iter = iter->ai_next) {
        ++nrecords;
    }

    // 创建监听地址数组
    lts_listen_array = (lts_socket_t **)lts_palloc(
        pool, sizeof(lts_socket_t *) * (nrecords + 1)
    );
    if (NULL == lts_listen_array) {
        // out of memory
        abort();
    }

    i = 0;
    lts_listen_array[0] = NULL;
    for (iter = records; iter; iter = iter->ai_next) {
        int fd;
        int tmp, ipv6_only, reuseaddr;
        lts_socket_t *ls;

        if ((AF_INET != iter->ai_family)
                && ((!ENABLE_IPV6) || (AF_INET6 != iter->ai_family))) {
            continue;
        }

        ls = (lts_socket_t *)lts_palloc(pool, sizeof(lts_socket_t));
        if (NULL == ls) {
            // out of memory
            abort();
        }
        lts_init_socket(ls);

        fd = socket(iter->ai_family, SOCK_STREAM, 0);
        if (-1 == fd) {
            break;
        }

        if (iter->ai_family == AF_INET6) { // 仅ipv6
            ipv6_only = 1;
            tmp = setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY,
                                 &ipv6_only, sizeof(ipv6_only));
            if (-1 == tmp) {
                // log
            }
        }

        tmp = lts_set_nonblock(fd);
        if (-1 == tmp) {
            // log
        }

        reuseaddr = 1;
        tmp = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                         (const void *) &reuseaddr, sizeof(int));
        if (-1 == tmp) {
            // log
        }

        if (-1 == bind(fd, iter->ai_addr, iter->ai_addrlen)) {
            (void)lts_write_logger(&lts_file_logger, LTS_LOG_ERROR,
                                   "%s:bind() failed:%s\n",
                                   STR_LOCATION,
                                   lts_errno_desc[errno]);
            (void)close(fd);
            break;
        }

        if (-1 == listen(fd, SOMAXCONN)) {
            (void)close(fd);
            break;
        }

        ls->fd = fd;
        ls->family = iter->ai_family;
        ls->ev_mask = (EPOLLET | EPOLLIN);
        ls->do_read = &lts_accept;

        lts_listen_array[i] = ls;
        lts_listen_array[++i] = NULL;
    }

    freeaddrinfo(records);

    if (NULL == lts_listen_array[0]) {
        // not one
        return -1;
    }

    // 建立socket缓存
    lts_sock_inuse_n = 0;
    lts_sock_cache_n = lts_main_conf.max_connections + 64;
    dlist_init(&lts_sock_list);

    sock_cache = (lts_socket_t *)(
        lts_palloc(pool, lts_sock_cache_n * sizeof(lts_socket_t))
    );
    for (i = 0; i < lts_sock_cache_n; ++i) {
        dlist_add_tail(&lts_sock_list, &sock_cache[i].dlnode);
    }

    return 0;
}


static void free_listen_sockets(void)
{
    for (int i = 0; lts_listen_array[i]; ++i) {
        lts_socket_t *ls = lts_listen_array[i];

        if (-1 == close(ls->fd)) {
            (void)lts_write_logger(&lts_file_logger, LTS_LOG_ERROR,
                                   "%s:close() failed:%s\n",
                                   STR_LOCATION, lts_errno_desc[errno]);
        }
    }
}


static int init_event_core_master(lts_module_t *mod)
{
    int rslt;
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
    dlist_init(&lts_watch_list);
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
void lts_evt_recv(lts_socket_t *cs)
{
    ssize_t recv_sz;
    lts_buffer_t *buf;
    lts_app_module_itfc_t *app_itfc = (lts_app_module_itfc_t *)(
        lts_module_app_cur->itfc
    );

    buf = cs->conn->rbuf;

    if (lts_buffer_full(buf)) {
        lts_buffer_drop_accessed(buf); // 试图腾出缓冲
    }
    if (lts_buffer_full(buf)) {
        // 应用模块繁忙
        (void)lts_write_logger(&lts_file_logger, LTS_LOG_WARN,
                               "%s:recv buffer is full\n", STR_LOCATION);
        cs->readable = 0;
        return;
    }

    recv_sz = recv(cs->fd, buf->last,
                   (uintptr_t)buf->end - (uintptr_t)buf->last, 0);
    if (-1 == recv_sz) {
        if ((LTS_E_AGAIN == errno) || (LTS_E_WOULDBLOCK == errno)) {
            // 本次数据读完
            cs->readable = 0;
        } else {
            int lvl;

            // 异常关闭
            lvl = (
                (LTS_E_CONNRESET == errno) ? LTS_LOG_NOTICE : LTS_LOG_ERROR
            );
            (void)lts_write_logger(&lts_file_logger, lvl,
                                   "%s:recv() failed:%s, reset connection\n",
                                   STR_LOCATION, lts_errno_desc[errno]);

            // 直接重置连接
            lts_close_conn(cs, TRUE);
        }

        return;
    } else if (0 == recv_sz) {
        // 正常关闭连接
        lts_close_conn(cs, FALSE);

        return;
    } else {
        buf->last += recv_sz;

        // 通知应用模块处理
        (*app_itfc->on_received)(cs);
    }

    return;
}


void lts_evt_send(lts_socket_t *cs)
{
    ssize_t sent_sz;
    lts_buffer_t *buf;
    lts_app_module_itfc_t *app_itfc = (lts_app_module_itfc_t *)(
        lts_module_app_cur->itfc
    );

    buf = cs->conn->sbuf;

    if (0 == lts_buffer_pending(buf)) { // no more sending
        cs->writable = 0;
        return;
    }
    ASSERT(lts_buffer_pending(buf) > 0);

    // 发送数据
    sent_sz = send(cs->fd, buf->seek, lts_buffer_pending(buf), 0);

    if (-1 == sent_sz) {
        if ((LTS_E_AGAIN == errno) || (LTS_E_WOULDBLOCK == errno)) {
            return;
        }

        (void)lts_write_logger(&lts_file_logger, LTS_LOG_ERROR,
                               "%s:send(%d) failed:%s, reset connection\n",
                               STR_LOCATION, cs->fd,
                               lts_errno_desc[errno]);

        // 直接重置连接
        lts_close_conn(cs, TRUE);

        return;
    }

    buf->seek += sent_sz;
    (*app_itfc->on_sendable)(cs); // 通知应用层可写

    return;
}


void lts_evt_error(lts_socket_t *cs)
{
    int tmperr;
    socklen_t len = sizeof(tmperr);

    (void)getsockopt(cs->fd, SOL_SOCKET, SO_ERROR, &tmperr, &len);
    (void)lts_write_logger(
        &lts_file_logger, LTS_LOG_ERROR, "%s:socket[%d] error:%s\n",
        STR_LOCATION, cs->fd, lts_errno_desc[tmperr]
    );

    // 直接重置连接
    lts_close_conn(cs, TRUE);

    return;
}


void lts_evt_timeout(lts_timer_node_t *cstn)
{
    lts_socket_t *cs = CONTAINER_OF(cstn, lts_socket_t, timer_node);
    lts_close_conn(cs, FALSE);

    return;
}


lts_module_t lts_event_core_module = {
    lts_string("lts_event_core_module"),
    LTS_CORE_MODULE,
    NULL,
    NULL,
    &s_event_core_ctx,

    // interfaces
    &init_event_core_master,
    &init_event_core_worker,
    &exit_event_core_worker,
    &exit_event_core_master,
};


lts_event_module_itfc_t *lts_event_itfc;

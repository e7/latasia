#include <sys/epoll.h>

#include <stdio.h>

#include "conf.h"
#include "latasia.h"
#include "logger.h"
#include "vsignal.h"
#include "rbt_timer.h"


static int epfd;
static struct epoll_event *buf_epevs;
static int nbuf_epevs;


static int epoll_event_add(lts_socket_t *s)
{
    struct epoll_event ee;

    ee.events = s->ev_mask;
    ee.data.ptr = (void *)((uintptr_t)s | s->instance);

    if (-1 == epoll_ctl(epfd, EPOLL_CTL_ADD, s->fd, &ee)) {
        // log
        return LTS_E_SYS;
    }

    return 0;
}


static int epoll_event_del(lts_socket_t *s)
{
    struct epoll_event ee;

    if (-1 == epoll_ctl(epfd, EPOLL_CTL_DEL, s->fd, &ee)) {
        // log
        return LTS_E_SYS;
    }

    return 0;
}


static int epoll_process_events(void)
{
    int i, nevents, timeout, tmp_err;
    lts_socket_t *s;
    uintptr_t instance;
    uint32_t revents;
    sigset_t sig_mask;

    (void)sigfillset(&sig_mask);
    (void)sigdelset(&sig_mask, SIGALRM); // 允许时钟信号
    timeout = dlist_empty(&lts_post_list) ? -1 : 0;
    nevents = epoll_pwait(epfd, buf_epevs, nbuf_epevs, timeout, &sig_mask);
    tmp_err = (-1 == nevents) ? errno : 0;

    // 更新时间
    if (lts_signals_mask & LTS_MASK_SIGALRM) {
        lts_signals_mask &= ~LTS_MASK_SIGALRM;
        lts_update_time();
    }

    if (tmp_err) {
        if (EINTR == tmp_err) { // 信号中断
            return 0;
        } else {
            return LTS_E_SYS;
        }
    }

    for (i = 0; i < nevents; ++i) {
        s = (lts_socket_t *)buf_epevs[i].data.ptr;
        instance = ((uintptr_t)s & (uintptr_t)1);
        s = (lts_socket_t *)((uintptr_t)s & (uintptr_t)~1);

        if ((-1 == s->fd) || (instance != s->instance)) {
            // 过期事件
            continue;
        }

        revents = buf_epevs[i].events;
        if ((revents & (EPOLLERR | EPOLLHUP)
            && (revents & (EPOLLIN | EPOLLOUT)) == 0))
        {
            revents |= (EPOLLIN | EPOLLOUT);
        }

        if ((revents & EPOLLIN) && (NULL != s->on_readable)) {
            (*s->on_readable)(s);
        }

        if ((revents & EPOLLOUT) && (NULL != s->on_writable)) {
            (*s->on_writable)(s);
        }
    }

    return 0;
}


static int init_event_epoll_worker(lts_module_t *mod)
{
    int max_conns;
    lts_pool_t *pool;

    // 全局变量初始化
    lts_timer_heap = RB_ROOT;
    lts_event_itfc = (lts_event_module_itfc_t *)mod->itfc;

    // 创建内存池
    pool = lts_create_pool(MODULE_POOL_SIZE);
    if (NULL == pool) {
        // log
        return -1;
    }
    mod->pool = pool;

    // 创建epoll对象
    epfd = epoll_create1(0);
    if (-1 == epfd) {
        return LTS_E_SYS;
    }

    // 创建epoll_event缓存
    max_conns = lts_conf.max_connections;
    buf_epevs = (struct epoll_event *)(
        lts_palloc(pool, max_conns * sizeof(struct epoll_event))
    );
    if (NULL == buf_epevs) {
        return -1;
    }
    nbuf_epevs = max_conns;

    // 添加channel事件监视
    epoll_event_add(lts_channels[lts_ps_slot]);

    return 0;
}


static void exit_event_epoll_worker(lts_module_t *mod)
{
    // 删除channel事件监视
    epoll_event_del(lts_channels[lts_ps_slot]);

    if (-1 == close(epfd)) {
        (void)lts_write_logger(&lts_file_logger, LTS_LOG_ERROR, "close failed");
    }
    lts_destroy_pool(mod->pool);

    return;
}


static lts_event_module_itfc_t event_epoll_itfc = {
    &epoll_event_add,
    &epoll_event_del,
    &epoll_process_events,
};


lts_module_t lts_event_epoll_module = {
    lts_string("lts_event_epoll_module"),
    LTS_EVENT_MODULE,
    &event_epoll_itfc,
    NULL,
    NULL,
    // interfaces
    NULL,
    &init_event_epoll_worker,
    &exit_event_epoll_worker,
    NULL,
};

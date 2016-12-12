#include <sys/epoll.h>

#include <stdio.h>

#include "conf.h"
#include "latasia.h"
#include "logger.h"
#include "vsignal.h"
#include "rbt_timer.h"

#define __THIS_FILE__       "src/modules/mod_event_epoll.c"


static int epfd;
static struct epoll_event *buf_epevs;
static int nbuf_epevs;


static void epoll_event_add(lts_socket_t *s)
{
    struct epoll_event ee;

    ee.events = s->ev_mask;
    ee.data.ptr = (void *)((uintptr_t)s | s->instance);

    if (-1 == epoll_ctl(epfd, EPOLL_CTL_ADD, s->fd, &ee)) {
        (void)lts_write_logger(&lts_file_logger, LTS_LOG_ERROR,
                               "%s:epoll_ctl_add(%d) failed:%s\n",
                               STR_LOCATION, s->fd, lts_errno_desc[errno]);
        ASSERT(0);
    }

    return;
}


static void epoll_event_mod(lts_socket_t *s)
{
    struct epoll_event ee;

    ee.events = s->ev_mask;
    ee.data.ptr = (void *)((uintptr_t)s | s->instance);

    if (-1 == epoll_ctl(epfd, EPOLL_CTL_MOD, s->fd, &ee)) {
        (void)lts_write_logger(&lts_file_logger, LTS_LOG_ERROR,
                               "%s:epoll_ctl_mod(%d) failed:%s\n",
                               STR_LOCATION, s->fd, lts_errno_desc[errno]);
        ASSERT(0);
    }

    return;
}


static void epoll_event_del(lts_socket_t *s)
{
    struct epoll_event ee;

    if (-1 == epoll_ctl(epfd, EPOLL_CTL_DEL, s->fd, &ee)) {
        (void)lts_write_logger(&lts_file_logger, LTS_LOG_ERROR,
                               "%s:epoll_ctl_del(%d) failed:%s\n",
                               STR_LOCATION, s->fd, lts_errno_desc[errno]);
        ASSERT(0);
    }

    return;
}


static int epoll_process_events(void)
{
    int i, nevents, timeout, tmp_err;
    lts_socket_t *cs;
    lts_timer_node_t *min;
    uintptr_t instance;
    uint32_t revents;
#ifndef HAVE_FUNCTION_EPOLL_PWAIT
    sigset_t orig_mask;
#endif
    sigset_t sig_mask;

    (void)sigfillset(&sig_mask);
    (void)sigdelset(&sig_mask, SIGALRM); // 允许时钟信号
    (void)sigdelset(&sig_mask, SIGPIPE);
    min = lts_timer_min(&lts_timer_heap);
    if (! dlist_empty(&lts_post_list)) {
        timeout = 0;
    } else if (min) {
        timeout = (int)((min->mapnode.key - lts_current_time) * 100); // ms
        if (timeout <= 0) {
            timeout = 0;
        }
    } else {
        timeout = -1;
    }
#ifndef HAVE_FUNCTION_EPOLL_PWAIT
    sigprocmask(SIG_SETMASK, &sig_mask, &orig_mask);
    nevents = epoll_wait(epfd, buf_epevs, nbuf_epevs, timeout);
    sigprocmask(SIG_SETMASK, &orig_mask, NULL);
#else
    nevents = epoll_pwait(epfd, buf_epevs, nbuf_epevs, timeout, &sig_mask);
#endif
    tmp_err = (-1 == nevents) ? errno : 0;

    // 更新时间
    if (lts_signals_mask & LTS_MASK_SIGALRM) {
        lts_signals_mask &= ~LTS_MASK_SIGALRM;
        lts_update_time();
    }

    // 错误处理
    if (tmp_err) {
        if (EINTR == tmp_err) { // 信号中断
            return 0;
        } else {
            return -1;
        }
    }

    // 事件处理
    for (i = 0; i < nevents; ++i) {
        cs = (lts_socket_t *)buf_epevs[i].data.ptr;
        instance = ((uintptr_t)cs & (uintptr_t)1);
        cs = (lts_socket_t *)((uintptr_t)cs & (uintptr_t)~1);

        if (instance != cs->instance) {
            // 忽略过期事件
            continue;
        }

        revents = buf_epevs[i].events;
        if (revents & (EPOLLERR | EPOLLHUP)) {
            if (cs->do_error) {(*cs->do_error)(cs);}
            continue;
        }

        if (revents & EPOLLIN) {
            cs->readable = 1;
            lts_post_list_add(cs);
        }

        if (revents & EPOLLOUT) {
            cs->writable = 1;
            lts_post_list_add(cs);
        }
    }

    return 0;
}


static int init_event_epoll_worker(lts_module_t *mod)
{
    lts_pool_t *pool;

    // 全局变量初始化
    lts_event_itfc = (lts_event_module_itfc_t *)mod->itfc;

    // 创建内存池
    pool = lts_create_pool(MODULE_POOL_SIZE);
    if (NULL == pool) {
        // log
        return -1;
    }
    mod->pool = pool;

    // 创建epoll对象
#ifndef HAVE_FUNCTION_EPOLL_CREATE1
    epfd = epoll_create(1024);
#else
    epfd = epoll_create1(0);
#endif
    if (-1 == epfd) {
        (void)lts_write_logger(&lts_file_logger, LTS_LOG_ERROR,
                               "%s:create epoll inst failed:%s\n",
                               STR_LOCATION, lts_errno_desc[errno]);
        return -1;
    }

    // 创建epoll_event缓存
    nbuf_epevs = lts_sock_cache_n + lts_sock_inuse_n;
    buf_epevs = (struct epoll_event *)(
        lts_palloc(pool, nbuf_epevs * sizeof(struct epoll_event))
    );
    if (NULL == buf_epevs) {
        return -1;
    }

    // 添加channel事件监视
    epoll_event_add(lts_channels[lts_ps_slot]);

    return 0;
}


static void exit_event_epoll_worker(lts_module_t *mod)
{
    // 删除channel事件监视
    epoll_event_del(lts_channels[lts_ps_slot]);

    if (-1 == close(epfd)) {
        (void)lts_write_logger(&lts_file_logger, LTS_LOG_ERROR,
                               "%s:close() failed:%s\n",
                               STR_LOCATION, lts_errno_desc[errno]);
    }
    lts_destroy_pool(mod->pool);

    return;
}


static lts_event_module_itfc_t event_epoll_itfc = {
    &epoll_event_add,
    &epoll_event_mod,
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

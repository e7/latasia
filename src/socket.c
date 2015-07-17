/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#include "socket.h"

#define __THIS_FILE__       "src/socket.c"


int lts_accept_disabled;
dlist_t lts_sock_list; // socket缓存列表
size_t lts_sock_cache_n;
size_t lts_sock_inuse_n;
dlist_t lts_addr_list;
dlist_t lts_listen_list;
dlist_t lts_conn_list;
dlist_t lts_post_list;
dlist_t lts_timeout_list; // 超时列表


#ifndef HAVE_FUNCTION_ACCEPT4
int lts_accept4(int sockfd, struct sockaddr *addr,
                socklen_t *addrlen, int flags)
{
    int fd;

    fd = accept(sockfd, addr, addrlen);
    if (-1 == fd) {
        return -1;
    }
    (void)lts_set_nonblock(fd);

    return fd;
}
#endif

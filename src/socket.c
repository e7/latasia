/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#include "socket.h"
#include "adv_string.h"

#define __THIS_FILE__       "src/socket.c"


int lts_accept_disabled;
dlist_t lts_sock_list; // socket缓存列表
size_t lts_sock_cache_n;
size_t lts_sock_inuse_n;
lts_socket_t **lts_listen_array; // 监听socket数组
dlist_t lts_watch_list;
dlist_t lts_post_list;
dlist_t lts_task_list = DLIST_NODE(lts_task_list); // 请求列表


char *lts_inet_ntoa(struct in_addr in)
{
    static char rslt[16];

    union {
        uint8_t u8[4];
        uint32_t u32;
    } uin;

    uin.u32 = in.s_addr;
    memset(rslt, 0, sizeof(rslt)); // 清空
    strcat(rslt, lts_uint322cstr(uin.u8[0])); // 保证不越界可使用该函数
    strcat(rslt, "."); // 保证不越界可使用该函数
    strcat(rslt, lts_uint322cstr(uin.u8[1])); // 保证不越界可使用该函数
    strcat(rslt, "."); // 保证不越界可使用该函数
    strcat(rslt, lts_uint322cstr(uin.u8[2])); // 保证不越界可使用该函数
    strcat(rslt, "."); // 保证不越界可使用该函数
    strcat(rslt, lts_uint322cstr(uin.u8[3])); // 保证不越界可使用该函数

    return rslt;
}


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

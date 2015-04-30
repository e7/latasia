/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#include "socket.h"


int lts_accept_disabled;
dlist_t lts_sock_list; // socket缓存列表
size_t lts_sock_cache_n;
size_t lts_sock_inuse_n;
dlist_t lts_listen_sock_list;
dlist_t lts_conn_list;
dlist_t lts_post_list;
dlist_t lts_timeout_list; // 超时列表

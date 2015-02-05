#include "socket.h"


int lts_accept_disabled;
dlist_t lts_sock_list; // socket缓存列表
size_t lts_sock_cache_n;
size_t lts_sock_inuse_n;
dlist_t lts_listen_sock_list;
dlist_t lts_post_list;

/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#include "adv_string.h"
#include "mem_pool.h"
#include "conf.h"


// 默认配置
lts_conf_asyn_t lts_asyn_conf = {
    lts_string("127.0.0.1"),
    6666,
};


static void cb_host_match(void *c,
                          lts_str_t *k,
                          lts_str_t *v,
                          lts_pool_t *pool)
{
    return;
}


static void cb_port_match(void *c,
                          lts_str_t *k,
                          lts_str_t *v,
                          lts_pool_t *pool)
{
    return;
}


lts_conf_item_t *lts_conf_asyn_items[] = {
    &(lts_conf_item_t){lts_string("host"), &cb_host_match},
    &(lts_conf_item_t){lts_string("port"), &cb_port_match},
    NULL,
};

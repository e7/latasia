/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#include "adv_string.h"
#include "mem_pool.h"
#include "conf.h"


// 默认配置
lts_conf_lua_t lts_lua_conf = {
    lts_string("lua_app/service.lua"),
};


static void cb_entry_match(void *c,
                           lts_str_t *k,
                           lts_str_t *v,
                           lts_pool_t *pool)
{
    return;
}


lts_conf_item_t *lts_conf_lua_items[] = {
    &(lts_conf_item_t){lts_string("entry"), &cb_entry_match},
    NULL,
};

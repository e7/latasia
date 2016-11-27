#include <zlib.h>
#include <arpa/inet.h>
#include <dirent.h>

#include <strings.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "latasia.h"
#include "rbtree.h"
#include "logger.h"
#include "conf.h"

#define __THIS_FILE__       "src/modules/mod_app_lua.c"


static lua_State *s_state;
static struct {
    lts_socket_t *s;
} s_lua_ctx;


static int init_lua_module(lts_module_t *module)
{
    s_state = luaL_newstate();
    if (NULL == s_state) {
        return -1;
    }
    luaL_openlibs(s_state);

    return 0;
}


static void exit_lua_module(lts_module_t *module)
{
    lua_close(s_state);
    return;
}


static void lua_on_connected(lts_socket_t *s)
{
    return;
}


static int api_push_sbuf(lua_State *s)
{
    uint8_t *data = (uint8_t *)lua_tostring(s, 1);
    lts_buffer_t *sb = s_lua_ctx.s->conn->sbuf;

    if (NULL == data) {
        return 0;
    }

    lua_pop(s, 1);
    lts_buffer_append(sb, data, strlen((char const *)data));

    return 0;
}


static int set_context_table(lua_State *s)
{
    int table_idx;
    lts_buffer_t *rb = s_lua_ctx.s->conn->rbuf;

    lua_newtable(s);
    table_idx = lua_gettop(s);

    lua_pushlstring(s, (char const *)rb->seek, rb->last - rb->seek);
    lua_setfield(s, table_idx, "rbuf");
    lua_pushcfunction(s, &api_push_sbuf);
    lua_setfield(s, table_idx, "push_sbuf");

    return 1; // lua调该函数时返回值数目
}


static void lua_service(lts_socket_t *s)
{
    lts_buffer_t *rb = s->conn->rbuf;
    lts_buffer_t *sb = s->conn->sbuf;

    s_lua_ctx.s = s;

    if (luaL_loadfile(s_state, "service.lua")) {
        // LUA_ERRFILE
        fprintf(stderr, "%s\n", lua_tostring(s_state, -1));
        return;
    }

    // 注册api
    lua_register(s_state, "lts_context", &set_context_table);

    // 执行初始化
    if (lua_pcall(s_state, 0, 0, 0)) {
        fprintf(stderr, "%s\n", lua_tostring(s_state, -1));
        return;
    }

    // 执行main函数
    lua_getglobal(s_state, "main");
    if (lua_pcall(s_state, 0, 0, 0)) {
        fprintf(stderr, "call function `main` faile:%s\n", lua_tostring(s_state, -1));
        return;
    }

    return;
}


static void lua_send_more(lts_socket_t *s)
{
    return;
}


static lts_app_module_itfc_t lua_itfc = {
    &lua_on_connected,
    &lua_service,
    &lua_send_more,
    NULL,
};

lts_module_t lts_app_lua_module = {
    lts_string("lts_app_lua_module"),
    LTS_APP_MODULE,
    &lua_itfc,
    NULL,
    NULL,
    // interfaces
    NULL,
    &init_lua_module,
    &exit_lua_module,
    NULL,
};

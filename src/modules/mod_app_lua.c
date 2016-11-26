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


static void lua_service(lts_socket_t *s)
{
    lts_buffer_t *rb = s->conn->rbuf;
    lts_buffer_t *sb = s->conn->sbuf;

    if (luaL_loadfile(s_state, "service.lua")) {
        // LUA_ERRFILE
        return;
    }
    lua_pcall(s_state, 0, 0, 0);
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

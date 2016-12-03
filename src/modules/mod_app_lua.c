#include <zlib.h>
#include <arpa/inet.h>
#include <dirent.h>

#include <strings.h>

#include "luajit/lua.h"
#include "luajit/lualib.h"
#include "luajit/lauxlib.h"

#include "latasia.h"
#include "rbtree.h"
#include "logger.h"
#include "conf.h"

#define __THIS_FILE__       "src/modules/mod_app_lua.c"

#define CONF_FILE           "conf/mod_app.conf"


static lua_State *s_state;
static struct {
    lts_socket_t *s;
} s_lua_ctx;
static int load_lua_config(lts_conf_lua_t *conf, lts_pool_t *pool);


// 加载模块配置
int load_lua_config(lts_conf_lua_t *conf, lts_pool_t *pool)
{
    extern lts_conf_item_t *lts_conf_lua_items[];

    off_t sz;
    uint8_t *addr;
    int rslt;
    lts_file_t lts_conf_file = {
        -1, {
            (uint8_t *)CONF_FILE, sizeof(CONF_FILE) - 1,
        },
    };

    if (-1 == load_conf_file(&lts_conf_file, &addr, &sz)) {
        return -1;
    }
    rslt = parse_conf(addr, sz, lts_conf_lua_items, pool, conf);
    close_conf_file(&lts_conf_file, addr, sz);

    return rslt;
}


static int init_lua_module(lts_module_t *module)
{
    lts_pool_t *pool;

    // 创建模块内存池
    pool = lts_create_pool(MODULE_POOL_SIZE);
    if (NULL == pool) {
        // log
        return -1;
    }
    module->pool = pool;

    // 读取模块配置
    if (-1 == load_lua_config(&lts_lua_conf, module->pool)) {
        (void)lts_write_logger(&lts_stderr_logger, LTS_LOG_WARN,
                               "%s:load lua config failed, using default\n",
                               STR_LOCATION);
    }

    // 初始化lua运行环境
    // setenv("LUA_PATH", (char const *)lts_lua_conf.search_path.data, TRUE);
    // setenv("LUA_CPATH", (char const *)lts_lua_conf.search_cpath.data, TRUE);
    s_state = luaL_newstate();
    if (NULL == s_state) {
        return -1;
    }
    luaL_openlibs(s_state);

    lua_getglobal(s_state, "package");
    if (! lua_istable(s_state, -1)) {
        return -1;
    }

    lua_pushlstring(
        s_state, (char const *)lts_lua_conf.search_path.data,
        lts_lua_conf.search_path.len
    );
    lua_setfield(s_state, -2, "path");
    lua_pushlstring(
        s_state, (char const *)lts_lua_conf.search_cpath.data,
        lts_lua_conf.search_cpath.len
    );
    lua_setfield(s_state, -2, "cpath");
    lua_pop(s_state, 1);

    return 0;
}


static void exit_lua_module(lts_module_t *module)
{
    lua_close(s_state);

    // 销毁模块内存池
    if (module->pool) {
        lts_destroy_pool(module->pool);
        module->pool = NULL;
    }

    return;
}


static void lua_on_connected(lts_socket_t *s)
{
    return;
}


static int api_tcp_socket(lua_State *s)
{
    extern lts_event_module_itfc_t *lts_event_itfc;
    int sockfd;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sockfd) {
        lua_pushstring(s_state, "create socket failed");
        lua_pushnil(s_state);
        return 2;
    }

    lua_newtable(s_state);
        lua_pushstring(s_state, "fd");
        lua_pushinteger(s_state, sockfd);
    lua_pushnil(s_state);

    return 2;
}


static int api_pop_rbuf(lua_State *s)
{
    lts_buffer_t *rb = s_lua_ctx.s->conn->rbuf;
    lua_pushlstring(s_state, (char const *)rb->seek, rb->last - rb->seek);
    return 1;
}


static int api_push_sbuf(lua_State *s)
{
    uint8_t const *data;
    size_t dlen;
    data = (uint8_t const *)lua_tolstring(s, 1, &dlen);
    lts_buffer_t *sb = s_lua_ctx.s->conn->sbuf;

    if (NULL == data) {
        return 0;
    }

    lua_pop(s, 1);
    lts_buffer_append(sb, data, dlen);

    return 0;
}


//static int set_context_table(lua_State *s)
//{
//    int table_idx;
//    lts_buffer_t *rb = s_lua_ctx.s->conn->rbuf;
//
//    lua_newtable(s);
//    table_idx = lua_gettop(s);
//
//    lua_pushlstring(s, (char const *)rb->seek, rb->last - rb->seek);
//    lua_setfield(s, table_idx, "rbuf");
//    lua_pushcfunction(s, &api_push_sbuf);
//    lua_setfield(s, table_idx, "push_sbuf");
//
//    return 1; // lua调该函数时返回值数目
//}


static void lua_service(lts_socket_t *s)
{
    lts_buffer_t *rb = s->conn->rbuf;
    // lts_buffer_t *sb = s->conn->sbuf;

    s_lua_ctx.s = s;

    if (luaL_loadfile(s_state, (char const *)lts_lua_conf.entry.data)) {
        // LUA_ERRFILE
        fprintf(stderr, "%s\n", lua_tostring(s_state, -1));
        return;
    }


    // 注册api
    lua_newtable(s_state);
        // ctx
        lua_pushstring(s_state, "ctx");
        lua_newtable(s_state);
            lua_pushstring(s_state, "pop_rbuf");
            lua_pushcfunction(s_state, &api_pop_rbuf);
            lua_settable(s_state, -3);
            lua_pushstring(s_state, "push_sbuf");
            lua_pushcfunction(s_state, &api_push_sbuf);
            lua_settable(s_state, -3);
        lua_settable(s_state, -3);

        // socket
        lua_pushstring(s_state, "socket");
        lua_newtable(s_state);
            lua_pushstring(s_state, "tcp");
            lua_pushcfunction(s_state, &api_tcp_socket);
            lua_settable(s_state, -3);
        lua_settable(s_state, -3);
    lua_setglobal(s_state, "lts");

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

    lts_buffer_clear(rb);

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

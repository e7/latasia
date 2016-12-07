#include <zlib.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/epoll.h>

#include <strings.h>

#include "luajit/lua.h"
#include "luajit/lualib.h"
#include "luajit/lauxlib.h"

#include "obj_pool.h"
#include "latasia.h"
#include "rbtree.h"
#include "logger.h"
#include "conf.h"

#define __THIS_FILE__       "src/modules/mod_app_lua.c"

#define CONF_FILE           "conf/mod_app.conf"


enum {
    E_SUCCESS = 200,
    E_FAILED = 400,
    E_OUTOFRESOURCE, // 401
    E_INVALIDARG, // 402
    E_INPROGRESS, // 403
};


extern lts_event_module_itfc_t *lts_event_itfc;

static lua_State *s_state, *s_rt_state;

static struct {
    lts_socket_t *s;
} s_lua_ctx;
static lts_obj_pool_t s_sock_pool;

static int load_lua_config(lts_conf_lua_t *conf, lts_pool_t *pool);
static void connect_on_write(lts_socket_t *s);
static void connect_on_error(lts_socket_t *s);

static int api_pop_rbuf(lua_State *s);
static int api_push_sbuf(lua_State *s);
static int api_tcp_socket_connect(lua_State *s);
static int api_tcp_socket_send(lua_State *s);
static int api_tcp_socket_recv(lua_State *s);
static int api_tcp_socket_close(lua_State *s);
static int api_tcp_socket(lua_State *s);

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
    lts_socket_t *cache;
    ssize_t cache_len;

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

    // 创建后端socket对象池
    cache_len = sizeof(lts_socket_t) * 1;
    cache = (lts_socket_t *)lts_palloc(pool, cache_len);
    lts_init_opool(&s_sock_pool, cache, cache_len,
                   sizeof(lts_socket_t), OFFSET_OF(lts_socket_t, dlnode));

    // 初始化lua运行环境
    s_state = luaL_newstate();
    if (NULL == s_state) {
        return -1;
    }
    luaL_openlibs(s_state);

    // setenv("LUA_PATH", (char const *)lts_lua_conf.search_path.data, TRUE);
    // setenv("LUA_CPATH", (char const *)lts_lua_conf.search_cpath.data, TRUE);
    lua_getglobal(s_state, "package");
    if (! lua_istable(s_state, -1)) {
        return -1;
    }

    lua_pushstring(s_state, "path");
    lua_pushlstring(
        s_state, (char const *)lts_lua_conf.search_path.data,
        lts_lua_conf.search_path.len
    );
    lua_settable(s_state, -3);

    lua_pushstring(s_state, "cpath");
    lua_pushlstring(
        s_state, (char const *)lts_lua_conf.search_cpath.data,
        lts_lua_conf.search_cpath.len
    );
    lua_settable(s_state, -3);
    lua_pop(s_state, 1);

    // 注册API
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
    lua_setglobal(s_state, "lts"); // pop 1

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


void connect_on_write(lts_socket_t *s)
{
    s->writable = 0;
    (*lts_event_itfc->event_del)(s);

    // 连接已建立
    lua_pushinteger(s_rt_state, E_SUCCESS);
    (void)lua_resume(s_rt_state, 1);

    return;
}


void connect_on_error(lts_socket_t *s)
{
    int tmperr;
    socklen_t len = sizeof(tmperr);

    (void)getsockopt(s->fd, SOL_SOCKET, SO_ERROR, &tmperr, &len);
    fprintf(stderr, "connect_on_error: %s\n", lts_errno_desc[tmperr]);

    (*lts_event_itfc->event_del)(s);
    (void)close(s->fd);
    lts_op_release(&s_sock_pool, s);

    // 连接失败
    lua_pushinteger(s_rt_state, E_FAILED);
    (void)lua_resume(s_rt_state, 1);

    return;
}


// lts.socket.tcp {{
int api_tcp_socket(lua_State *s)
{
    int sockfd;
    lts_socket_t *conn;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sockfd) {
        fprintf(stderr, "socket() failed: %s\n", lts_errno_desc[errno]);
        lua_pushnil(s);
        lua_pushinteger(s, E_FAILED);
        return 2;
    }
    lts_set_nonblock(sockfd);

    conn = lts_op_instance(&s_sock_pool);
    if (NULL == conn) {
        fprintf(stderr, "out of pool\n");
        lua_pushnil(s);
        lua_pushinteger(s, E_OUTOFRESOURCE);
        return 2;
    }
    lts_init_socket(conn);
    conn->fd = sockfd;

    lua_newtable(s);
        lua_pushstring(s, "_sock");
        lua_pushlightuserdata(s, conn);
        lua_settable(s, -3);

        lua_pushstring(s, "connect");
        lua_pushcfunction(s, &api_tcp_socket_connect);
        lua_settable(s, -3);

        lua_pushstring(s, "close");
        lua_pushcfunction(s, &api_tcp_socket_close);
        lua_settable(s, -3);
    lua_pushinteger(s, E_SUCCESS);

    return 2;
}


int api_tcp_socket_connect(lua_State *s)
{
    int ok;
    struct sockaddr_in svr = {0};
    lts_socket_t *conn;
    //lts_str_t target_addr = lts_string();

    fprintf(stderr, "request connect\n");
    svr.sin_family = AF_INET;
    svr.sin_port = htons(luaL_checkint(s, -1));
    if (! inet_aton(luaL_checkstring(s, -2), &svr.sin_addr)) {
        lua_pushinteger(s, E_INVALIDARG);
        return 1;
    }

    lua_pop(s, 2); // 释放用户参数

    luaL_checktype(s, -1, LUA_TTABLE);
    lua_getfield(s, -1, "_sock");
    conn = (lts_socket_t *)lua_topointer(s, -1);

    ok = connect(conn->fd, (struct sockaddr *)&svr, sizeof(svr));
    if (-1 == ok) {
        if (errno != EINPROGRESS) {
            fprintf(stderr, "connect failed: %d\n", errno);
            (void)close(conn->fd);
            lts_op_release(&s_sock_pool, conn);

            lua_pushinteger(s, E_FAILED);
            return 1;
        }

        // 事件监视
        conn->ev_mask = EPOLLOUT | EPOLLET;
        conn->do_write = &connect_on_write;
        conn->do_error = &connect_on_error;
        (*lts_event_itfc->event_add)(conn);

        lua_pushinteger(s_rt_state, LUA_YIELD);
        return lua_yield(s_rt_state, 1);
    }

    lua_pushinteger(s, E_SUCCESS);
    return 1;
}


int api_tcp_socket_close(lua_State *s)
{
    lts_socket_t *conn;

    luaL_checktype(s, -1, LUA_TTABLE);
    lua_getfield(s, -1, "_sock");
    conn = (lts_socket_t *)lua_topointer(s, -1);

    (void)(*lts_event_itfc->event_del)(conn);
    (void)close(conn->fd);
    lts_op_release(&s_sock_pool, conn);

    return 0;
}
// }} lts.socket.tcp


int api_pop_rbuf(lua_State *s)
{
    lts_buffer_t *rb;

    rb = s_lua_ctx.s->conn->rbuf;
    lua_pushlstring(s, (char const *)rb->seek, rb->last - rb->seek);
    lts_buffer_clear(rb); // 清空读缓冲

    return 1;
}


int api_push_sbuf(lua_State *s)
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
    s_lua_ctx.s = s;

    // 执行
    s_rt_state = lua_newthread(s_state);
    if (luaL_loadfile(s_rt_state, (char const *)lts_lua_conf.entry.data)) {
        // LUA_ERRFILE
        fprintf(stderr, "%s\n", lua_tostring(s_rt_state, -1));
        return;
    }

    (void)lua_resume(s_rt_state, 0);

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

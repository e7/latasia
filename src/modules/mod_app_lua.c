#include <zlib.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/epoll.h>

#include <strings.h>

#ifdef LUAJIT
#include "luajit/lua.h"
#include "luajit/lualib.h"
#include "luajit/lauxlib.h"
#else
#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"
#endif

#include "obj_pool.h"
#include "latasia.h"
#include "rbtree.h"
#include "logger.h"
#include "conf.h"
#include "protocol_sjsonb.h"

#define __THIS_FILE__       "src/modules/mod_app_lua.c"

#define CONF_FILE           "conf/mod_app.conf"
#define NTASK_RSC           4


enum {
    E_SUCCESS = 200,
    E_FAILED = 400,
    E_OUTOFRESOURCE, // 401
    E_INVALIDARG, // 402
    E_INPROGRESS, // 403
};

enum {
    Y_NOTHING = 0,
    Y_FRONT_SENT,
    Y_BACK_CONNECT,
    Y_BACK_RECEIVE,
    Y_BACK_SEND,
};


typedef struct {
    uint16_t contype;
    lts_str_t content;
    dlist_t node; // 任务链节点
    lts_socket_t *cs;
} task_t;

typedef struct {
    uint32_t yield_for;
    lua_State *rt_state;
    lts_socket_t *front; // 前端连接
} lua_ctx_t;


extern lts_event_module_itfc_t *lts_event_itfc;

static lua_State *s_state;
static lua_ctx_t *curr_ctx; // 当前上下文

static lts_obj_pool_t s_sock_pool;

static int load_lua_config(lts_conf_lua_t *conf, lts_pool_t *pool);

static int api_push_sbuf(lua_State *s);
static int api_tcp_socket_connect(lua_State *s);
static void tcp_on_connected(lts_socket_t *s);
static void tcp_on_read(lts_socket_t *s);
static void tcp_on_write(lts_socket_t *s);
static void tcp_on_error(lts_socket_t *s);
static int api_tcp_socket_send(lua_State *s);
static int api_tcp_socket_receive(lua_State *s);
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


void tcp_on_read(lts_socket_t *s)
{
    lua_State *L;
    int bufsz, moresz;
    ssize_t recvsz;
    lts_buffer_t *rbuf = s->conn->rbuf;

    s->readable = 0;
    L = curr_ctx->rt_state;

    ASSERT(Y_BACK_RECEIVE == curr_ctx->yield_for);

    lts_buffer_drop_accessed(rbuf);

    bufsz = lua_tointeger(L, -1);
    lua_pop(L, 1);
    if (bufsz > lts_buffer_space(rbuf)) {
        fprintf(stderr, "lua: receive too large\n");
        lua_pushnil(L);
        lua_pushinteger(L, E_FAILED);
        (void)lua_resume(L, 2);
    }

    moresz = bufsz - lts_buffer_pending(rbuf);

    recvsz = recv(s->fd, rbuf->last, moresz, 0);
    if (recvsz > 0) {rbuf->last += recvsz;}
    if (recvsz == moresz) { // 足量
        (*lts_event_itfc->event_del)(s); // 删除事件监视

        curr_ctx->yield_for = Y_NOTHING;
        lua_pushlstring(L, (char const *)rbuf->seek, bufsz);
        lua_pushinteger(L, E_SUCCESS);
        rbuf->seek += bufsz;

        (void)lua_resume(L, 2);
    } else if (
        (recvsz > 0)
        || ((-1 == recvsz) && ((EAGAIN == errno) || (EWOULDBLOCK == errno)))
    ) {
        // continue block
        // 数据量压栈下次再读
        lua_pushinteger(L, bufsz);
    } else {
        (*lts_event_itfc->event_del)(s);

        lua_pushnil(L);
        lua_pushinteger(L, E_FAILED);
        (void)lua_resume(L, 2);
    }

    return;
}


void tcp_on_write(lts_socket_t *s)
{
    ssize_t sendsz;
    lts_buffer_t *sbuf = s->conn->sbuf;

    ASSERT(Y_BACK_SEND == curr_ctx->yield_for); // 只可能send阻塞时被调
    if (0 == lts_buffer_pending(sbuf)) {
        // no more sending
        s->writable = 0;
        (*lts_event_itfc->event_del)(s);

        curr_ctx->yield_for = Y_NOTHING;
        lua_pushinteger(curr_ctx->rt_state, E_SUCCESS);
        (void)lua_resume(curr_ctx->rt_state, 1);
        return;
    }

    sendsz = send(s->fd, sbuf->seek, lts_buffer_pending(sbuf), 0);
    if (-1 == sendsz) {
        if (EAGAIN == errno || EWOULDBLOCK == errno) {
            s->writable = 0;
        } else {
            (*lts_event_itfc->event_del)(s);
            lua_pushinteger(curr_ctx->rt_state, E_FAILED);
            (void)lua_resume(curr_ctx->rt_state, 1);
        }
    } else { // sendsz > 0
        sbuf->seek += sendsz;
    }

    return;
}


void tcp_on_connected(lts_socket_t *s)
{
    lts_pool_t *pool;

    ASSERT(Y_BACK_CONNECT == curr_ctx->yield_for); // 只可能connect阻塞时被调

    s->writable = 0;
    s->do_write = &tcp_on_write; // 切换回调
    (*lts_event_itfc->event_del)(s); // 删除事件监视

    // 初始化连接
    pool = lts_create_pool(4096 * 1024);
    s->conn = lts_palloc(pool, sizeof(lts_conn_t));
    s->conn->pool = pool;
    s->conn->sbuf = lts_create_buffer(pool, 4096, 4096);
    s->conn->rbuf = lts_create_buffer(pool, 4096, 4096);

    // 唤醒
    curr_ctx->yield_for = Y_NOTHING;
    lua_pushinteger(curr_ctx->rt_state, E_SUCCESS);
    (void)lua_resume(curr_ctx->rt_state, 1);

    return;
}


void tcp_on_error(lts_socket_t *s)
{
    int tmperr;
    socklen_t len = sizeof(tmperr);

    ASSERT(Y_BACK_CONNECT == curr_ctx->yield_for); // 只可能connect阻塞时被调
    (void)getsockopt(s->fd, SOL_SOCKET, SO_ERROR, &tmperr, &len);
    fprintf(stderr, "tcp_on_error: %s\n", lts_errno_desc[tmperr]);

    // close connection
    (*lts_event_itfc->event_del)(s);
    if (s->conn) {
        lts_destroy_pool(s->conn->pool);
        s->conn = NULL;
    }
    (void)close(s->fd);
    lts_op_release(&s_sock_pool, s);

    // 连接失败
    curr_ctx->yield_for = Y_NOTHING;
    lua_pushinteger(curr_ctx->rt_state, E_FAILED);
    (void)lua_resume(curr_ctx->rt_state, 1);

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
        lua_pushstring(s, "connect");
        lua_pushlightuserdata(s, conn);
        lua_pushcclosure(s, &api_tcp_socket_connect, 1);
        lua_settable(s, -3);

        lua_pushstring(s, "close");
        lua_pushlightuserdata(s, conn);
        lua_pushcclosure(s, &api_tcp_socket_close, 1);
        lua_settable(s, -3);

        lua_pushstring(s, "receive");
        lua_pushlightuserdata(s, conn);
        lua_pushcclosure(s, &api_tcp_socket_receive, 1);
        lua_settable(s, -3);

        lua_pushstring(s, "send");
        lua_pushlightuserdata(s, conn);
        lua_pushcclosure(s, &api_tcp_socket_send, 1);
        lua_settable(s, -3);
    lua_pushinteger(s, E_SUCCESS);

    return 2;
}


int api_tcp_socket_connect(lua_State *s)
{
    int ok;
    struct sockaddr_in svr = {0};
    lts_socket_t *conn;

    luaL_checktype(s, lua_upvalueindex(1), LUA_TLIGHTUSERDATA);
    conn = (lts_socket_t *)lua_topointer(s, lua_upvalueindex(1));
    svr.sin_family = AF_INET;
    svr.sin_port = htons(luaL_checkint(s, -1));
    if (! inet_aton(luaL_checkstring(s, -2), &svr.sin_addr)) {
        lua_pushinteger(s, E_INVALIDARG);
        return 1;
    }
    lua_pop(s, 2); // 释放用户参数
    lua_pop(s, 1); // 释放self参数

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
        conn->do_write = &tcp_on_connected;
        conn->do_error = &tcp_on_error;
        (*lts_event_itfc->event_add)(conn);

        curr_ctx->yield_for = Y_BACK_CONNECT;
        return lua_yield(s, 0);
    } else {
        ASSERT(0); // imporssible if non-block
    }

    lua_pushinteger(s, E_SUCCESS);
    return 1;
}


int api_tcp_socket_send(lua_State *s)
{
    size_t datalen = 0;
    char const *data;
    lts_socket_t *conn;
    lts_buffer_t *sbuf;

    luaL_checktype(s, lua_upvalueindex(1), LUA_TLIGHTUSERDATA);
    conn = (lts_socket_t *)lua_topointer(s, lua_upvalueindex(1));
    sbuf = conn->conn->sbuf;

    data = luaL_checklstring(s, -1, &datalen);
    if (lts_buffer_space(sbuf) < datalen) {
        // 发送的数据过大
        lua_pop(s, 2); // 释放所有参数

        lua_pushinteger(s, E_FAILED);
        return 1;
    }

    curr_ctx->yield_for = Y_BACK_SEND;
    ASSERT(0 == lts_buffer_append(sbuf, (uint8_t *)data, datalen));
    lua_pop(s, 2); // 释放所有参数

    // 写事件监视
    conn->ev_mask = EPOLLET | EPOLLOUT;
    conn->do_read = &tcp_on_write;
    (*lts_event_itfc->event_add)(conn);

    return lua_yield(s, 0);
}


int api_tcp_socket_receive(lua_State *s)
{
    int need_sz;
    lts_socket_t *conn;
    lts_buffer_t *rbuf;

    need_sz = luaL_checkint(s, -1);
    lua_pop(s, 1); // 释放用户参数
    lua_pop(s, 1); // 释放self参数

    luaL_checktype(s, lua_upvalueindex(1), LUA_TLIGHTUSERDATA);
    conn = (lts_socket_t *)lua_topointer(s, lua_upvalueindex(1));

    // 如果缓冲区足量直接返回
    rbuf = conn->conn->rbuf;
    if (lts_buffer_pending(rbuf) >= need_sz) {
        lua_pushlstring(s, (char const *)rbuf->seek, need_sz);
        lua_pushinteger(s, E_SUCCESS);
        rbuf->seek += need_sz;
        return 2;
    }

    curr_ctx->yield_for = Y_BACK_RECEIVE;

    // 读事件监视
    conn->ev_mask = EPOLLET | EPOLLIN;
    conn->do_read = &tcp_on_read;
    (*lts_event_itfc->event_add)(conn);

    lua_pushinteger(s, need_sz);
    return lua_yield(s, 1);
}


int api_tcp_socket_close(lua_State *s)
{
    lts_socket_t *conn;

    luaL_checktype(s, lua_upvalueindex(1), LUA_TLIGHTUSERDATA);
    conn = (lts_socket_t *)lua_topointer(s, lua_upvalueindex(1));

    if (conn->conn) {
        lts_destroy_pool(conn->conn->pool);
        conn->conn = NULL;
    }
    (void)shutdown(conn->fd, SHUT_WR);
    (void)close(conn->fd);
    lts_op_release(&s_sock_pool, conn);

    return 0;
}
// }} lts.socket.tcp


int api_push_sbuf(lua_State *s)
{
    uint8_t const *data;
    size_t dlen;
    lts_buffer_t *sb;

    data = (uint8_t const *)luaL_checklstring(s, -1, &dlen);
    sb = curr_ctx->front->conn->sbuf;
    if (dlen > sb->limit) {
        lua_pop(s, 1);
        lua_pushstring(s, "too large to sent");
        return 1;
    }

    if (lts_buffer_space(sb) < dlen) {
        lts_buffer_drop_accessed(sb);
    }
    if (lts_buffer_space(sb) < dlen) {
        // 缓冲不足，只能等待on_sent通知
        curr_ctx->yield_for = Y_FRONT_SENT;
        return lua_yield(s, 0);
    }
    lts_buffer_append(sb, data, dlen);
    lua_pop(s, 1);

    lts_soft_event(curr_ctx->front, TRUE);

    lua_pushnil(s);
    return 1;
}


static void mod_on_connected(lts_socket_t *s)
{
    lua_State *L;
    void *rsc_buf;
    ssize_t task_rsc_sz = sizeof(task_t) * NTASK_RSC;

    // 分配任务资源池
    rsc_buf = lts_palloc(s->conn->pool, task_rsc_sz);
    ASSERT(rsc_buf);
    lts_init_opool(&s->task_rsc, rsc_buf, task_rsc_sz,
                   sizeof(task_t), OFFSET_OF(task_t, node));

    // 初始化context
    s->app_ctx = lts_palloc(s->conn->pool, sizeof(lua_ctx_t));
    curr_ctx = (lua_ctx_t *)s->app_ctx;
    curr_ctx->yield_for = Y_NOTHING;
    curr_ctx->rt_state = lua_newthread(s_state);
    curr_ctx->front = s;

    // 创建协程
    L = curr_ctx->rt_state;
    lua_getglobal(L, "lts");
        // front
        lua_pushstring(L, "front");
        lua_newtable(L);
            lua_pushstring(L, "closed");
            lua_pushboolean(L, FALSE);
            lua_settable(L, -3);
            lua_pushstring(L, "push_sbuf");
            lua_pushcclosure(L, &api_push_sbuf, 0);
            lua_settable(L, -3);
        lua_settable(L, -3);
    lua_setglobal(L, "lts");

    return;
}


static void mod_on_received(lts_socket_t *s)
{
    task_t *tsk;
    uint16_t contype;
    lts_str_t content = lts_null_string;

    // 解码
    while ((tsk = lts_op_instance(&s->task_rsc))) {
        if (-1 == lts_proto_sjsonb_decode(s->conn->rbuf, &contype, &content)) {
            lts_op_release(&s->task_rsc, tsk);
            break;
        }

        // 追加任务
        tsk->contype = contype;
        lts_str_share(&tsk->content, &content);
        tsk->cs = s;
        dlist_add_tail(&lts_task_list, &tsk->node);
    }

    return;
}


static void mod_on_service(dlist_t *nd)
{
    int r;
    lua_State *L;
    task_t *tsk = CONTAINER_OF(nd, task_t, node);
    lts_socket_t *cs = tsk->cs;

    curr_ctx = (lua_ctx_t *)cs->app_ctx;
    ASSERT(curr_ctx);
    L = curr_ctx->rt_state;

    ASSERT(tsk > 0);

    lua_getglobal(L, "lts");
        lua_getfield(L, -1, "front");
            lua_pushstring(L, "contype");
            lua_pushinteger(L, tsk->contype);
            lua_settable(L, -3);
            lua_pushstring(L, "content");
            lua_pushlstring(L, (char const *)tsk->content.data, tsk->content.len);
            lua_settable(L, -3);
        lua_setfield(L, -2, "front");
    lua_setglobal(L, "lts");

    // run coroutine
    if (luaL_loadfile(L, (char const *)lts_lua_conf.entry.data)) {
        // LUA_ERRFILE
        lts_op_release(&cs->task_rsc, tsk); // 记得归还
        fprintf(stderr, "load file failed:%s\n", lua_tostring(L, -1));
        return;
    }

    r = lua_resume(L, 0);

    if (r && (LUA_YIELD != r)) {
        fprintf(stderr, "resume failed: %s\n", lua_tostring(L, -1));
    }
    lts_op_release(&cs->task_rsc, tsk); // 记得归还

    return;
}


static void mod_on_sent(lts_socket_t *s)
{
    lua_State *L;

    curr_ctx = (lua_ctx_t *)s->app_ctx;
    ASSERT(curr_ctx);
    L = curr_ctx->rt_state;

    if (Y_FRONT_SENT == curr_ctx->yield_for) {
        int r;
        uint8_t const *data;
        size_t dlen;
        lts_buffer_t *sbuf = curr_ctx->front->conn->sbuf;

        data = (uint8_t const *)luaL_checklstring(L, 1, &dlen);
        if (lts_buffer_space(sbuf) < dlen) {
            lts_buffer_drop_accessed(sbuf);
        }
        if (lts_buffer_space(sbuf) < dlen) {
            // keep block
            return;
        }

        (void)lts_buffer_append(sbuf, data, dlen);
        lua_pop(L, 1);

        curr_ctx->yield_for = Y_NOTHING;
        lua_pushnil(L);
        r = lua_resume(L, 0);
        if (r && (LUA_YIELD != r)) {
            fprintf(stderr, "resume failed: %s\n", lua_tostring(L, -1));
        }
    }

    return;
}


static void mod_on_closing(lts_socket_t *s)
{
    lua_State *L;

    curr_ctx = (lua_ctx_t *)s->app_ctx;
    ASSERT(curr_ctx);
    L = curr_ctx->rt_state;

    lua_getglobal(L, "lts");
        lua_getfield(L, -1, "front");
            lua_pushstring(L, "closed");
            lua_pushboolean(L, TRUE);
            lua_settable(L, -3);
        lua_setfield(L, -2, "front");
    lua_setglobal(L, "lts");

    return;
}


static lts_app_module_itfc_t lua_itfc = {
    &mod_on_connected,
    &mod_on_received,
    &mod_on_service,
    &mod_on_sent,
    &mod_on_closing,
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

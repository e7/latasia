#include <arpa/inet.h>

#include "protocol_sjsonb.h"

#define MAGIC_NO                0xE78F8A9DU
#define PROTO_VER               1000


typedef struct {
    uint32_t magic_no;
    uint32_t proto_ver;
    uint32_t content_ofst; // 内容偏移
    uint32_t content_len; // 内容长度
    uint32_t content_checksum; // 内容校验码
} sjsonb_header_t __attribute__ ((aligned (1)));
#define LEN_HEADER              sizeof(sjsonb_header_t)
// 最短json串长度
#define LEN_MIN_CONTENT         2


int lts_proto_sjsonb_encode(lts_sjson_t *sjson, lts_buffer_t *buf)
{
    lts_str_t str_sjson;

    if (-1 == lts_sjson_encode(sjson, &str_sjson)) {
        return -1;
    }

    if (lts_buffer_pending(buf) < str_sjson.len) {
        lts_buffer_drop_accessed(buf);
    }

    if (lts_buffer_pending(buf) < str_sjson.len) {
        return -1;
    }

    (void)lts_buffer_append(buf, str_sjson.data, str_sjson.len);

    return 0;
}


lts_sjson_t *lts_proto_sjsonb_decode(lts_buffer_t *buf, lts_pool_t *pool)
{
    union {
        char c[4];
        uint32_t ui;
    } uni_magic;
    sjsonb_header_t header, *header_be;
    ssize_t idx_pack_start;
    lts_str_t str_magic, str_buf;
    int valid_header_found;
    lts_sjson_t *rslt;

    // 包头标识的字符串形式
    uni_magic.ui = htonl(MAGIC_NO);
    str_magic = (lts_str_t)lts_string(uni_magic.c);

    // 寻找有效包头
    valid_header_found = FALSE;
    while (! valid_header_found) {
        // 缓冲的字符串形式
        str_buf.data = buf->seek;
        str_buf.len = lts_buffer_pending(buf);

        idx_pack_start = lts_str_find(&str_buf, &str_magic, 0);
        if (-1 == idx_pack_start) {
            // 无效数据，丢弃缓冲数据
            lts_buffer_clear(buf);
            break;
        }
        buf->seek += idx_pack_start; // 丢弃部分垃圾数据

        // 解析包头
        if (lts_buffer_pending(buf) < (LEN_HEADER + LEN_MIN_CONTENT)) {
            // 数据长度不足以解析
            break;
        }

        header_be = (sjsonb_header_t *)buf->seek;

        // 转换为本地字节序
        header.magic_no = ntohl(header_be->magic_no);
        header.proto_ver= ntohl(header_be->proto_ver);
        header.content_ofst = ntohl(header_be->content_ofst);
        header.content_len = ntohl(header_be->content_len);
        header.content_checksum = ntohl(header_be->content_checksum);

        // 包头数据检查
        if ((header.proto_ver != PROTO_VER)
            || (header.content_ofst < LEN_HEADER)
            || ((header.content_len < LEN_MIN_CONTENT)
                || header.content_len > 4096)) {
            buf->seek += sizeof(uint32_t); // 丢弃该包头标识
            continue;
        }

        valid_header_found = TRUE;
    }
    if (! valid_header_found) {
        return NULL;
    }

    if (lts_buffer_pending(buf) < header.content_ofst + header.content_len) {
        // 数据长度不足以解析
        return NULL;
    }

    rslt = (lts_sjson_t *)lts_palloc(pool, sizeof(lts_sjson_t));
    *rslt = lts_empty_sjson(pool); // 初始化

    str_buf.data = &buf->seek[header.content_ofst];
    str_buf.len = header.content_len;

    if (-1 == lts_sjson_decode(&str_buf, rslt)) {
        return NULL;
    }

    return rslt;
}
